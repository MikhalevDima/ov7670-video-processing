`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////

module axis_stream #(
    parameter CD = 24,
    parameter WMAX = 640,
    parameter HMAX = 480
)
(

    input logic clk,
    input logic reset,

    /* CONTROL REGISTR */
    input logic [31:0] control,
    /* AXIS VIDEO STREAM */
    input logic m_axis_tready,
    output logic m_axis_tvalid,
    output logic m_axis_tuser,          // начало строки
    output logic m_axis_tlast,          // конец строки
    output logic [CD-1:0] m_axis_tdata
);

/* Декларация внутренних сигналов */
logic [10:0] X, Y;
logic StartOfLine, frame_started;
logic enable;
logic [CD-1:0] tdata_reg, tdata_next;

assign enable = control[0];             // включение если первый регистр установлен в 1

always_ff @( posedge clk ) begin
    if (!reset) m_axis_tvalid <= 0;
    else begin 
        m_axis_tvalid <= enable && (X < WMAX) && (Y < HMAX);
    end    
end

always_ff @( posedge clk ) begin
    if (!reset) begin
        tdata_reg <= 0;
    end
    else tdata_reg <= tdata_next;
end

/*Выход данных*/
assign m_axis_tdata = tdata_reg;

/* Экземпляр модуля axis_counter */
axis_counter #(
    .WMAX(640),       // ширина
    .HMAX(480)        // высота
)
axis_counter_inst
(
    .clk(clk),
    .reset(reset),

    /* Сигнал разрешения */
    .tready(m_axis_tready && enable),
    /* Выходные счётчики hc и vc */
    .width(X),
    .height(Y),

    /* Начало и конец строки */
    .StartOfFrame(m_axis_tuser),
    .EndOfLine(m_axis_tlast)
);

/* Экземпляр модуля bar_demo */
bar_demo bar_demo_inst(
    .x(X), 
    .y(Y),    
    .bar_rgb(tdata_next)
    );


endmodule
