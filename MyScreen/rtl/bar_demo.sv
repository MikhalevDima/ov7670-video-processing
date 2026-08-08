`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////

module bar_demo(
    input logic [10:0] x, y,        // x/y axis
    output logic [23:0] bar_rgb
    );

/* declaration */

logic [7:0] r, g, b;

/*
always_comb begin : test_generator
    r = {x[9:5], 3'b000};
    g = {x[9:5], 3'b000};
    b = {x[9:5], 3'b000};
end
*/

always_comb begin : checkerboard_generator
    // Используем 5-й бит (вес 32 пикселя) для X и Y
    // Операция XOR (^) создаст шахматный порядок
    if (x[5] ^ y[5]) begin
        // Белый квадрат
        r = 8'hFF;
        g = 8'hFF;
        b = 8'hFF;
    end else begin
        // Черный квадрат
        r = 8'h00;
        g = 8'h00;
        b = 8'h00;
    end
end

assign bar_rgb = {r, g, b};            // расширяем чтобы получить 24 бита для преобразования в ip блоке rgb2dvi

endmodule
