`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
/* Передаём принятый байт в фифо буфер для для перехода в другой домен частоты */

module fifo_buffer(
    input  logic        clk,        // Системная частота (для чтения)
    input  logic        reset,      // Общий сброс

    // Интерфейс OV7670
    input  logic        VSYNC,      // Вход кадровой синхронизации
    input  logic        HREF,       // Вход строчной синхронизации
    input  logic        PCLK,       // Вход тактового сигнала пикселей (для записи)
    input  logic [7:0]  Din,        // 8-битный вход с камеры

    // Интерфейс чтения (выходной домен)
    output logic [31:0] D_out,      // 32-битное значение YUV
    input  logic        rd_en,      // Сигнал чтения из FIFO
    output logic        full,       // Флаг заполнения
    output logic        empty,      // Флаг пустоты

    // Дополнительные сигналы из фифо
    output logic overflow,          // Переполнение фифо
    output logic underflow,         // Опустошение фифо
    output logic[9:0] write_count,  // Доступное число слов для записи
    output logic[9:0] read_count,   // Доступное число слов для чтения 

    
    input logic enable              // Сигнал включения ip модуля
    

    );


// Объявление промежуточных сигналов для соединения модулей
logic [31:0] internal_pixel_data;
logic        internal_pixel_valid;
assign wr_valid = internal_pixel_valid & enable;

ov7670_pixel_rec ov7670_pixel_rec_inst (
    .reset       (reset),
    .Dout        (internal_pixel_data),  // Соединяем с шиной данных FIFO
    .pixel_valid (internal_pixel_valid), // Соединяем с wr_en FIFO

    .VSYNC       (VSYNC),
    .HREF        (HREF),
    .PCLK        (PCLK),
    .Din         (Din)
);

// Сборка отладочной шины
// DATA IN
/*
(*mark_debug = "true", keep = "true"*) logic [7:0] Din_dbg;
(*mark_debug = "true", keep = "true"*) logic VSYNC_dbg;
(*mark_debug = "true", keep = "true"*) logic HREF_dbg;
(*mark_debug = "true", keep = "true"*) logic PCLK_dbg;
(*mark_debug = "true", keep = "true"*) logic clk_dbg;
(*mark_debug = "true", keep = "true"*) logic reset_dbg;

assign Din_dbg = Din;
assign VSYNC_dbg = VSYNC;
assign HREF_dbg = HREF;
assign PCLK_dbg = PCLK;
assign clk_dbg = clk;
assign reset_dbg = reset;

// DATa IN END
*/
// DATA OUT
/*
(*mark_debug = "true", keep = "true"*) logic [31:0] D_out_dbg;
assign D_out_dbg = D_out;
*/
// DATA OUT END

// 1. Синхронизируем VSYNC в домен системной частоты чтения (clk)
logic vsync_clk_d1, vsync_clk_d2;                               // Надо сбросить FIFO по VSYNC, но сбрасываем в домене clk
always_ff @(posedge clk or posedge reset) begin
    if (reset) begin
        vsync_clk_d1 <= 1'b0;
        vsync_clk_d2 <= 1'b0;
    end else begin
        vsync_clk_d1 <= VSYNC;
        vsync_clk_d2 <= vsync_clk_d1;
    end
end

// 2. Выделяем ОДИН короткий импульс (длиной в 1 такт clk) в самом начале кадра
logic frame_rst_pulse;
assign frame_rst_pulse = vsync_clk_d1 && !vsync_clk_d2;

// 3. Безопасный счетчик сброса в стабильном домене clk
logic [3:0] cnt_res;
localparam FIFO_RESET = 15;

always_ff @(posedge clk or posedge reset) begin
    if (reset || frame_rst_pulse) begin
        cnt_res <= 4'h0;
    end 
    else if (cnt_res < FIFO_RESET) begin
        cnt_res <= cnt_res + 1'b1;
    end
end

logic fifo_res;
assign fifo_res = (cnt_res < FIFO_RESET);

// Логика задержки wr_clk, rd_clk на 10 тактов после reset
logic wr_rst_busy;
logic rd_rst_busy;


assign wr_en_fifo = (wr_valid && !wr_rst_busy);
assign rd_en_fifo = (rd_en && !rd_rst_busy);

fifo_generator_1 your_instance_name (
  .rst    (fifo_res),                   // Сброс FIFO
  .wr_clk (PCLK),                       // Частота записи — PCLK камеры
  .rd_clk (clk),                        // Частота чтения — моей системы
  .din    (internal_pixel_data),        // 32-битные данные
  .wr_en  (wr_en_fifo),                 // Пишем только когда пиксель готов
  .rd_en  (rd_en_fifo),                 // Сигнал чтения из фифо
  .dout   (D_out),                      // Выходные данные
  .full   (full),                       // Флаг полного фифо
  .empty  (empty),                      // Флаг пустого фифо
  .overflow   (overflow),               // Флаг переполнения
  .underflow  (underflow),              // Флаг опустошения
  .rd_data_count (read_count),          // Доступное число слов для записи
  .wr_data_count (write_count),         // Доступное число слов для чтения  
  .wr_rst_busy(wr_rst_busy),            // output wire wr_rst_busy
  .rd_rst_busy(rd_rst_busy)             // output wire rd_rst_busy
);

// FIFO END

endmodule
