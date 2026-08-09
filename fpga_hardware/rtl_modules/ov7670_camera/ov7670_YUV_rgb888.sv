`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////

module ov7670_rgb565_rgb888(

    input logic clk,                        // Системная частота
    input logic reset,                      // Общий сброс


    // Интерфейс OV7670
    input logic VSYNC,                      //Вход кадровой сигнализации
    input logic HREF,                       //Вход строчной синхронизации
    input logic PCLK,                       //Вход тактового сигнала пикселей
    input logic [7:0] Din,                  //8-битный параллельный вход

    // Интерфейс для управления и чтения RGB888
    input logic enable,                     // Разрешаем преобразование
    input logic monohrome,                  // Чёрно-белый режим работы камеры
    output logic [23:0] RGB888_1pix,        // Выходной пиксель в формате RGB888 1 пиксель
    output logic [23:0] RGB888_2pix,        // Выходной пиксель в формате RGB888 2 пиксель

    // Сигналы статуса из фифо
    input logic fifo_rd,
    output logic fifo_empty,
    output logic fifo_full,
    output logic [9:0] fifo_data_count

    );
   

/* Локальные параметры для преобразования в RGB888 (теперь строго знаковые!) */

localparam signed [17:0] AW = 18'sd350;   // 1.3707 * 256
localparam signed [17:0] BW = 18'sd86;    // 0.3376 * 256
localparam signed [17:0] DW = 18'sd178;   // 0.698  * 256
localparam signed [17:0] FW = 18'sd443;   // 1.7324 * 256

/* Декларация внутренних сигналов */
logic [31:0] YUV_in;
logic rgb888_valid;
logic fifo_empty_int;
logic [9:0] fifo_data_count_int;

/* Экземпляр внешних сигналов */
fifo_buffer fifo_buffer_inst(
    .clk(clk),                          // Системная частота (для чтения)
    .reset(reset),                      // Общий сброс

    // Интерфейс OV7670
    .VSYNC(VSYNC),                      // Вход кадровой синхронизации
    .HREF(HREF),                        // Вход строчной синхронизации
    .PCLK(PCLK),                        // Вход тактового сигнала пикселей (для записи)
    .Din(Din),                          // 8-битный вход с камеры

    // Интерфейс чтения (выходной домен)
    .D_out(YUV_in),                     // 32-бит (YUV)
    .rd_en(fifo_rd),                      // Сигнал чтения из FIFO
    .full(fifo_full),                   // Флаг заполнения
    .empty(fifo_empty_int),             // Флаг пустого фифо
    .overflow(),                        // Флаг переполнения
    .underflow(),                       // Флаг опустошения
    .write_count(),                     // Доступное число слов для записи
    .read_count(fifo_data_count_int),   // Доступное число слов для чтения

    
    .enable(enable)                     // Сигнал включения ip модуля

    );

/* Экземплярв внешних модулей Конец */

// Проброс сигналов на выход
assign fifo_empty = fifo_empty_int;
assign fifo_data_count = fifo_data_count_int;


logic [7:0] y0_val, u_val, y1_val, v_val;

assign y0_val = YUV_in[31:24];
assign v_val  = YUV_in[23:16];
assign y1_val = YUV_in[15:8];
assign u_val  = YUV_in[7:0];  

// Переводим хроминанту в честный знаковый вид (-128...127) путем расширения до 9 бит
logic signed [8:0] u_chroma, v_chroma;

always_comb begin
    if(!monohrome) begin
        u_chroma = $signed({1'b0, u_val}) - 9'sd128;
        v_chroma = $signed({1'b0, v_val}) - 9'sd128;
    end
    else begin
        u_chroma = 9'sd0;
        v_chroma = 9'sd0;
    end
end


// Знаковые промежуточные результаты (24 бита исключают любое переполнение при умножении)
logic signed [25:0] r1_sign, g1_sign, b1_sign;
logic signed [25:0] r2_sign, g2_sign, b2_sign;

// Создаем промежуточные переменные для сдвинутого значения
logic signed [25:0] r1_shifted, g1_shifted, b1_shifted;
logic signed [25:0] r2_shifted, g2_shifted, b2_shifted;

// Внутренние 8-битные шины после ограничения (clipping)
logic [7:0] r1_clip, g1_clip, b1_clip;
logic [7:0] r2_clip, g2_clip, b2_clip;

assign r1_shifted = r1_sign >>> 8;
assign g1_shifted = g1_sign >>> 8;
assign b1_shifted = b1_sign >>> 8;

assign r2_shifted = r2_sign >>> 8;
assign g2_shifted = g2_sign >>> 8;
assign b2_shifted = b2_sign >>> 8;

// Математика для Первого пикселя (Y0, U, V)
always_comb begin
    // Y расширяем нулем и сдвигаем (умножаем на 256). Все участники уравнения теперь SIGNED.
    r1_sign = ($signed({1'b0, y0_val}) << 8) + (AW * v_chroma);
    g1_sign = ($signed({1'b0, y0_val}) << 8) - (BW * u_chroma) - (DW * v_chroma);
    b1_sign = ($signed({1'b0, y0_val}) << 8) + (FW * u_chroma);

    // Сравнение выполняем строго со знаковыми константами 24'sd...
    r1_clip = (r1_shifted > 24'sd255) ? 8'd255 : ((r1_shifted < 24'sd0) ? 8'd0 : r1_shifted[7:0]);
    g1_clip = (g1_shifted > 24'sd255) ? 8'd255 : ((g1_shifted < 24'sd0) ? 8'd0 : g1_shifted[7:0]);
    b1_clip = (b1_shifted > 24'sd255) ? 8'd255 : ((b1_shifted < 24'sd0) ? 8'd0 : b1_shifted[7:0]);
end

// Математика для Второго пикселя (Y1, U, V)
always_comb begin
    r2_sign = ($signed({1'b0, y1_val}) << 8) + (AW * v_chroma);
    g2_sign = ($signed({1'b0, y1_val}) << 8) - (BW * u_chroma) - (DW * v_chroma);
    b2_sign = ($signed({1'b0, y1_val}) << 8) + (FW * u_chroma);

    r2_clip = (r2_shifted > 24'sd255) ? 8'd255 : ((r2_shifted < 24'sd0) ? 8'd0 : r2_shifted[7:0]);
    g2_clip = (g2_shifted > 24'sd255) ? 8'd255 : ((g2_shifted < 24'sd0) ? 8'd0 : g2_shifted[7:0]);
    b2_clip = (b2_shifted > 24'sd255) ? 8'd255 : ((b2_shifted < 24'sd0) ? 8'd0 : b2_shifted[7:0]);
end

assign RGB888_1pix = {g1_clip, b1_clip, r1_clip}; 
assign RGB888_2pix = {g2_clip, b2_clip, r2_clip};


/* Отладка */
/*
(*mark_debug = "true", keep = "true"*) logic [23:0] rgb888_1pix_dbg;
(*mark_debug = "true", keep = "true"*) logic [23:0] rgb888_2pix_dbg;
assign rgb888_1pix_dbg = RGB888_1pix;
assign rgb888_2pix_dbg = RGB888_2pix;
*/

endmodule
