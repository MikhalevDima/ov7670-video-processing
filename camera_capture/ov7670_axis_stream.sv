module ov7670_axis_stream #(
    parameter WIDTH = 640,
    parameter HEIGHT = 480
)
(
    input logic clk,
    input logic reset, // Активный high (1 - сброс).

    input logic [31:0] ControlRegister,

    // OV7670 INPUT
    input logic VSYNC,                      
    input logic HREF,                       
    input logic PCLK,                       
    input logic [7:0] Din,                  

    // AXIS STREAM INTERFACE
    input logic         m_axis_tready,
    output logic        m_axis_tvalid,
    output logic [23:0] m_axis_tdata,
    output logic        m_axis_tlast,                
    output logic        m_axis_tuser                
);

// Управление включением модуля
logic ENABLE;
logic MONOCHROME;
logic fifo_rd_en;
logic fifo_empty_int;

// ИСПРАВЛЕНО: Декларация и правильное подключение шин данных конвертера
logic [23:0] convert_pix1;
logic [23:0] convert_pix2;

assign ENABLE = ControlRegister[0];
assign MONOCHROME = ControlRegister[1];

(*mark_debug = "true", keep = "true"*) logic ControlRegister_dbg;
assign ControlRegister_dbg = ControlRegister;

// Синхронизация VSYNC для безопасного сброса счетчиков кадров
logic vsync_clk_d1, vsync_clk_d2;
always_ff @(posedge clk) begin
    if (reset) begin
        vsync_clk_d1 <= 1'b0;
        vsync_clk_d2 <= 1'b0;
    end else begin
        vsync_clk_d1 <= VSYNC;
        vsync_clk_d2 <= vsync_clk_d1;
    end
end
logic frame_start_detect;
assign frame_start_detect = vsync_clk_d1 && !vsync_clk_d2;

// Флаг четности: 0 - отправляем 1-й пиксель из FIFO, 1 - отправляем 2-й пиксель (буферный)
logic cnt_pix;

// Главное правило потока: запрашиваем чтение из FIFO ТОЛЬКО тогда, когда:
// 1. В FIFO реально есть данные (!fifo_empty_int)
// 2. Video Mixer готов принимать пиксели (m_axis_tready == 1)
// 3. МЫ НАХОДИМСЯ НА ВТОРОМ ПИКСЕЛЕ (cnt_pix == 1). Это значит, что текущая пара пикселей уходит, 
//    и на следующем такте нам нужно получить из Standard FIFO уже новое слово!
assign fifo_rd_en = ENABLE && !fifo_empty_int && (cnt_pix == 1'b1 && m_axis_tready);

/* Экземпляр модуля конвертера */
// ВНИМАНИЕ: Подключаем fifo_rd_en напрямую. Внутри конвертера выходы RGB888_1pix/2pix 
// должны быть сделаны через комбинаторный assign (без always_ff), как мы договаривались!
ov7670_YUV_rgb888 ov7670_YUV_rgb888_inst(
    .clk(clk),                              
    .reset(reset), 

    .VSYNC(VSYNC),                          
    .HREF(HREF),                            
    .PCLK(PCLK),                            
    .Din(Din),                              

    .enable(ENABLE),
    .monohrome(MONOCHROME),                           
    .RGB888_1pix(convert_pix1),                  
    .RGB888_2pix(convert_pix2),                 

    .fifo_rd(fifo_rd_en), // Управляем чтением потоком
    .fifo_empty(fifo_empty_int),
    .fifo_full(),
    .fifo_data_count()
);

// Буфер для сохранения второго пикселя
logic [23:0] r_pix2_buf;

// Логика переключения пикселей (Handshake)
logic axis_handshake;
assign axis_handshake = m_axis_tvalid && m_axis_tready;

always_ff @(posedge clk or posedge reset) begin
    if (reset || frame_start_detect) begin
        cnt_pix    <= 1'b0;
        r_pix2_buf <= 24'b0;
    end 
    else begin
        // Так как FIFO работает в режиме Standard, данные на шине convert_pix2 
        // появляются через 1 такт после того, как fifo_rd_en отработал (когда cnt_pix был равен 1).
        // Поэтому фиксируем второй пиксель строго в тот момент, когда завершился handshake первого пикселя!
        if (axis_handshake && (cnt_pix == 1'b0)) begin
            r_pix2_buf <= convert_pix2;
        end

        // Переключаем триггер пикселей строго по факту их успешного ухода в Mixer
        if (axis_handshake) begin
            cnt_pix <= cnt_pix + 1'b1;
        end
    end
end

// ВЫХОДНЫЕ ДАННЫЕ:
// Если cnt_pix == 0, берем первый пиксель напрямую с выхода конвертера.
// Если cnt_pix == 1, берем второй пиксель из нашего стабильного буфера.
assign m_axis_tdata = (cnt_pix == 1'b0) ? convert_pix1 : r_pix2_buf;

// ВАЛИДНОСТЬ: выставляем в 1, если в FIFO есть данные и модуль включен.
// Для Standard FIFO: на первом пикселе (cnt_pix == 0) данные валидны, если FIFO не пусто.
// На втором пикселе (cnt_pix == 1) данные берутся из буфера, они валидны всегда, пока не уйдут.
assign m_axis_tvalid = ENABLE && !fifo_empty_int;


// --- СЧЕТЧИКИ КООРДИНАТ КАДРА (СТРОГО ПО HANDSHAKE) ---
logic [10:0] width_reg;
logic [10:0] height_reg;

always_ff @(posedge clk) begin
    if (reset || frame_start_detect) begin
        width_reg  <= 11'd0;
        height_reg <= 11'd0;
    end
    else if (axis_handshake) begin
        if (width_reg == (WIDTH - 1)) begin
            width_reg <= 11'd0;
            if (height_reg == (HEIGHT - 1)) begin
                height_reg <= 11'd0;
            end else begin
                height_reg <= height_reg + 1'b1;
            end
        end else begin
            width_reg <= width_reg + 1'b1;
        end
    end
end

// Формирование маркеров AXI4-Stream Video
assign m_axis_tuser = (width_reg == 11'd0) && (height_reg == 11'd0) && m_axis_tvalid;
assign m_axis_tlast = (width_reg == (WIDTH - 1)) && m_axis_tvalid;


endmodule
