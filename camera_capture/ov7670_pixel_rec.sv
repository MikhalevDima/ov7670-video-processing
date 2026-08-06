`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Приём пикселя формата YUV
module ov7670_pixel_rec (
    input logic reset,

    output logic [31:0] Dout,
    output logic pixel_valid,

    // OV7670
    input logic VSYNC,                      
    input logic HREF,                       
    input logic PCLK,                       
    input logic [7:0] Din                   
);

// Для YUV422 (Y0, U, Y1, V) нужно 4 байта = 2 пикселя
logic [7:0] Y0, U, Y1;
logic [1:0] step;
logic       href_last; // Регистр для отслеживания фронта HREF

// Тактуем строго по спаду PCLK, как рекомендует Omnivision
always_ff @(negedge PCLK) begin
    if (reset) begin
        step        <= 2'd0;
        Y0          <= 8'h0;
        U           <= 8'h0;
        Y1          <= 8'h0;
        Dout        <= 32'h0;
        pixel_valid <= 1'b0;
        href_last   <= 1'b0;
    end 
    else begin
        pixel_valid <= 1'b0;  // Сброс флага по умолчанию
        href_last   <= HREF;  // Запоминаем предыдущее состояние HREF

        // Жесткий сброс автомата между кадрами
        if (VSYNC) begin             
            step <= 2'd0;
        end 
        // Ловим момент СТАРТА новой строки: HREF стал 1, а на прошлом такте был 0
        else if (HREF && !href_last) begin
            Y0   <= Din;      // Сразу надежно защелкиваем самый первый байт (Y0)
            step <= 2'd1;     // Переходим к ожиданию U
        end
        // Обычный прием байт внутри строки
        else if (HREF) begin 
            case (step)
                2'd1: begin
                    U    <= Din;
                    step <= 2'd2;
                end
                2'd2: begin
                    Y1   <= Din;
                    step <= 2'd3;
                end
                2'd3: begin
                    Dout        <= {Y0, U, Y1, Din}; // Собираем [Y0, U, Y1, V]
                    pixel_valid <= 1'b1;             
                    step        <= 2'd0;             
                end
                default: begin
                    // Если автомат оказался в step 0 в середине строки, 
                    // значит принимаем текущий байт как Y0
                    Y0   <= Din;
                    step <= 2'd1;
                end
            endcase
        end 
        else begin
            // Если HREF равен 0 (пауза между строками), удерживаем автомат в готовности
            step <= 2'd0;
        end
    end
end

/* Отладка */
//(*mark_debug = "true", keep = "true"*) logic [31:0] Dout_dbg; 
//assign Dout_dbg = Dout;

endmodule






