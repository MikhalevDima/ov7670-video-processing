module Gaussian_Filter_3x3 #(
    parameter WIDTH = 640,
    parameter HEIGHT = 480
)
(

    // -------------------------------------------------------------------------
    // Системные сигналы
    // -------------------------------------------------------------------------
    input logic         clk,
    input logic         reset,

    // -------------------------------------------------------------------------
    // Интерфейс AXI-Stream Slave (Вход видеопотока)
    // -------------------------------------------------------------------------
    output logic        s_axis_tready_top, // Готовность принимать данные
    input logic         s_axis_tvalid_top, // Валидность входных данных
    input logic [23:0]  s_axis_tdata_top,  // Входной пиксель (RGB)
    input logic         s_axis_tlast_top,  // Конец строки (End of Line)
    input logic         s_axis_tuser_top,  // Начало кадра (Start of Frame)

    // -------------------------------------------------------------------------
    // Интерфейс AXI-Stream Master (Выход задержанного потока)
    // -------------------------------------------------------------------------
    output logic        m_axis_tvalid_top, // Валидность выходных данных
    output logic [23:0] m_axis_tdata_top,  // Выходной пиксель (RGB / только зеленый)
    output logic        m_axis_tlast_top,  // Задержанный конец строки
    output logic        m_axis_tuser_top,  // Задержанное начало кадра
    input logic         m_axis_tready_top  // Готовность следующего модуля принимать данные
);

    // Экземпляры модуля LineBuffer

    // Линии для соединения модулей LineBuffer
    logic tvalid_l1;
    logic [23:0] tdata_l1;
    logic tlast_l1;
    logic tuser_l1;
    logic tready_l1;

    logic tvalid_l2;
    logic [23:0] tdata_l2;
    logic tlast_l2;
    logic tuser_l2;

    LineBuffer #(.WIDTH(WIDTH), .HEIGHT(HEIGHT))
    LineBuffer_1 (
    // -------------------------------------------------------------------------
    // Системные сигналы
    // -------------------------------------------------------------------------
    .clk(clk),
    .reset(reset),

    // -------------------------------------------------------------------------
    // Интерфейс AXI-Stream Slave (Вход видеопотока)
    // -------------------------------------------------------------------------
    .s_axis_tready(s_axis_tready_top), // Готовность принимать данные
    .s_axis_tvalid(s_axis_tvalid_top), // Валидность входных данных
    .s_axis_tdata(s_axis_tdata_top),  // Входной пиксель (RGB)
    .s_axis_tlast(s_axis_tlast_top),  // Конец строки (End of Line)
    .s_axis_tuser(s_axis_tuser_top),  // Начало кадра (Start of Frame)

    // -------------------------------------------------------------------------
    // Интерфейс AXI-Stream Master (Выход задержанного потока)
    // -------------------------------------------------------------------------
    .m_axis_tvalid(tvalid_l1), // Валидность выходных данных
    .m_axis_tdata(tdata_l1),  // Выходной пиксель (RGB / только зеленый)
    .m_axis_tlast(tlast_l1),  // Задержанный конец строки
    .m_axis_tuser(tuser_l1),  // Задержанное начало кадра
    .m_axis_tready(tready_l1)  // Готовность следующего модуля принимать данные
    );

    LineBuffer  #(.WIDTH(WIDTH), .HEIGHT(HEIGHT))
    LineBuffer_2 (
    // -------------------------------------------------------------------------
    // Системные сигналы
    // -------------------------------------------------------------------------
    .clk(clk),
    .reset(reset),

    // -------------------------------------------------------------------------
    // Интерфейс AXI-Stream Slave (Вход видеопотока)
    // -------------------------------------------------------------------------
    .s_axis_tready(tready_l1), // Готовность принимать данные
    .s_axis_tvalid(tvalid_l1), // Валидность входных данных
    .s_axis_tdata(tdata_l1),   // Входной пиксель (RGB)
    .s_axis_tlast(tlast_l1),   // Конец строки (End of Line)
    .s_axis_tuser(tuser_l1),   // Начало кадра (Start of Frame)

    // -------------------------------------------------------------------------
    // Интерфейс AXI-Stream Master (Выход задержанного потока)
    // -------------------------------------------------------------------------
    .m_axis_tvalid(tvalid_l2),          // Валидность выходных данных
    .m_axis_tdata(tdata_l2),            // Выходной пиксель (RGB / только зеленый)
    .m_axis_tlast(tlast_l2),            // Задержанный конец строки
    .m_axis_tuser(tuser_l2),            // Задержанное начало кадра
    .m_axis_tready(m_axis_tready_top)   // Готовность следующего модуля принимать данные
    );

logic [7:0] window [2:0][2:0];  // Матрица 3х3, где каждая ячейка по 8 бит


wire top_hdsk = s_axis_tvalid_top && s_axis_tready_top;     // Внутренний флаг общего сдвига
wire [7:0] pix_row2 = s_axis_tdata_top[23:16];              // Текущая строка
wire [7:0] pix_row1 = tdata_l1[23:16];                      // Строка с выхода 1-го буфера
wire [7:0] pix_row0 = tdata_l2[23:16];                      // Строка с выхода 2-го буфера

// Заполнение окна
always_ff @(posedge clk) begin
    if (reset) begin
        for (int r = 0; r < 3; r++) begin
            for (int c = 0; c < 3; c++ ) begin
                window[r][c] <= 'b0;            // Обнуляем значение окна
            end
        end
    end else if (top_hdsk) begin
        // Сдвигаем окно
        for (int i = 0; i < 3; i++) begin
            window[i][0] <= window[i][1];
            window[i][1] <= window[i][2];
        end
        // Заполняем окно
        window[0][2] <= pix_row0;
        window[1][2] <= pix_row1;
        window[2][2] <= pix_row2;
    end
end

// Задержка сигнала валидности, пока окно не заполнится. Окно заполнится на 3 такт, на 4 такт будет подсчитан Гаусс
localparam GAUSS_DELAY = 4; // Задержка: 3 такта на заполнение окна + 1 на вычисление
    
    logic [GAUSS_DELAY-1:0] valid_shifter;
    logic [GAUSS_DELAY-1:0] user_shifter;
    logic [GAUSS_DELAY-1:0] last_shifter;
    
    // Сигнал, что окно заполнено (после 3-х тактов)
    logic window_ready;

     always_ff @(posedge clk) begin
        if (reset) begin
            valid_shifter <= 'b0;
            user_shifter  <= 'b0;
            last_shifter  <= 'b0;
            window_ready  <= 1'b0;
        end else if (top_hdsk) begin
            // Сдвигаем сигналы с правильным размером
            valid_shifter <= {valid_shifter[GAUSS_DELAY-2:0], tvalid_l2};
            user_shifter  <= {user_shifter[GAUSS_DELAY-2:0], tuser_l2};
            last_shifter  <= {last_shifter[GAUSS_DELAY-2:0], tlast_l2};
            
            // Флаг готовности окна (после 3-х тактов заполнения)
            if (valid_shifter[2]) begin
                window_ready <= 1'b1;
            end
        end
    end


// Свёртка с матрицей Гаусса
localparam [7:0] GAUSSE [2:0][2:0] = '{ '{1, 2, 1},
                                        '{2, 4, 2},
                                        '{1, 2, 1} };
logic [11:0] gauss_sum;

// Временный комбинаторный провод для сборки суммы внутри такта
logic [11:0] next_sum;

always_comb begin
    next_sum = 'b0; // Обязательно обнуляем в начале комбинаторного блока!
    for(int row = 0; row < 3; row++) begin
        for(int col = 0; col < 3; col++) begin
            next_sum = next_sum + (window[row][col] * GAUSSE[row][col]);
        end
    end
end

// А в тактовом блоке мы просто защелкиваем готовый результат
always_ff @(posedge clk) begin
    if (reset) begin
        gauss_sum <= 'b0;
    end else if (top_hdsk && window_ready) begin
        gauss_sum <= next_sum;
    end 
end

 // Выходные данные (деление на 16)
    assign m_axis_tdata_top = {{3{gauss_sum[11:4]}}};
    
// Выходная валидность с задержкой
assign m_axis_tvalid_top = valid_shifter[GAUSS_DELAY-1];
assign m_axis_tuser_top  = user_shifter[GAUSS_DELAY-1];
assign m_axis_tlast_top  = last_shifter[GAUSS_DELAY-1];

endmodule