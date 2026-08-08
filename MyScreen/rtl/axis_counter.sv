module axis_counter #(
    parameter WMAX = 640,       // ширина
    parameter HMAX = 480        // высота
)
(
    input logic clk,
    input logic reset,

    /* Сигнал разрешения */
    input logic tready,
    /* Выходные счётчики hc и vc */
    output logic [10:0] width,
    output logic [10:0] height,

    /* Начало и конец строки */
    output logic StartOfFrame,
    output logic EndOfLine
);
    
/* Декларация внутренних сигналов */
logic [10:0] width_reg, width_next;
logic [10:0] height_reg, height_next;

logic StartOfFrame_reg, StartOfFrame_next, EndOfLine_reg, EndOfLine_next;

always_ff @(posedge clk) begin
    if(!reset) begin
        width_reg <= 0;
        height_reg <= 0;
        StartOfFrame_reg <= 0;
        EndOfLine_reg <= 0;
    end
    else if(tready) begin
        width_reg <= width_next;
        height_reg <= height_next;
        StartOfFrame_reg <= StartOfFrame_next;
        EndOfLine_reg <= EndOfLine_next;
    end
    else begin
        StartOfFrame_reg <= 0;
        EndOfLine_reg <= 0;
    end
end

/* Условия начало строки и конца */
assign StartOfFrame_next = ((width_reg == 0 && height_reg == 0));
assign EndOfLine_next = (width_reg == (WMAX - 1)); 

/* Усовия перехода widht */
always_comb begin : widgt_count
    if (width_reg == (WMAX - 1)) begin
        width_next = 0;
    end
    else width_next = width_reg + 1;
end

/* Условия перехода height */
always_comb begin : height_count
    if (width_reg == (WMAX - 1)) begin
        if (height_reg == (HMAX - 1)) begin
            height_next = 0;
        end
        else height_next = height_reg + 1;
    end
    else height_next = height_reg;
end

/* Назначение выходов */
assign width = width_reg;
assign height = height_reg;
assign StartOfFrame = StartOfFrame_reg;
assign EndOfLine = EndOfLine_reg;

endmodule
