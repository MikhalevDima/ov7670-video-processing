`timescale 1ns / 1ps

module Gaussian_Filter_3x3_tb ();

// Параметры изображения
parameter WIDTH = 640;
parameter HEIGHT = 512;
parameter INPUT_FILE = "C:/Users/mikha/Desktop/Xilinx/SystemVerilog/GausseFilter/test.txt";
parameter OUTPUT_FILE = "C:/Users/mikha/Desktop/Xilinx/SystemVerilog/GausseFilter/test_f.txt";
parameter FRAME_SIZE = WIDTH * HEIGHT;

// Для хранения файлов
integer file_in, file_out;

// Переменные для хранения прочитанных пикселей из файла и для записи в файл
logic [7:0] pixel_in;
logic [7:0] pixel_out;

// Переменный для хранения координат пикселей
integer row, col;

// Тактовый сигнал
logic clk;
parameter PERIOD = 10;
always begin
    clk = 1'b0;
    #(PERIOD/2) clk = 1'b1;
    #(PERIOD/2);
end

// Объявляем сигнал reset в модуле тестбенча
logic reset;
// Задача сброса
task automatic do_reset();
    begin
        reset = 1'b1;              // Включаем сброс (активная единица)
        repeat (3) @(posedge clk); // Ждем ровно 3 положительных фронта clk
        reset = 1'b0;              // Снимаем сброс
        @(posedge clk);            // Ждем еще один такт для стабилизации схемы
    end
endtask

// -----------------------------------------------------------------------------
// Подключаем мой top модуль для тестирования
// -----------------------------------------------------------------------------

// Интерфейс AXI-Stream Slave (Вход видеопотока)
logic s_axis_tready_dut;
logic s_axis_tvalid_dut;
logic [23:0] s_axis_tdata_dut;
logic s_axis_tlast_dut;
logic s_axis_tuser_dut;

// Интерфейс AXI-Stream Master (Выход задержанного потока)
logic m_axis_tready_dut;
logic m_axis_tvalid_dut;
logic [23:0] m_axis_tdata_dut;
logic m_axis_tlast_dut;
logic m_axis_tuser_dut;

Gaussian_Filter_3x3 my_dut(

    // -------------------------------------------------------------------------
    // Системные сигналы
    // -------------------------------------------------------------------------
    .clk(clk),
    .reset(reset),

    // -------------------------------------------------------------------------
    // Интерфейс AXI-Stream Slave (Вход видеопотока)
    // -------------------------------------------------------------------------
    .s_axis_tready_top(s_axis_tready_dut),  // Готовность принимать данные
    .s_axis_tvalid_top(s_axis_tvalid_dut),  // Валидность входных данных
    .s_axis_tdata_top(s_axis_tdata_dut),    // Входной пиксель (RGB)
    .s_axis_tlast_top(s_axis_tlast_dut),    // Конец строки (End of Line)
    .s_axis_tuser_top(s_axis_tuser_dut),    // Начало кадра (Start of Frame)

    // -------------------------------------------------------------------------
    // Интерфейс AXI-Stream Master (Выход задержанного потока)
    // -------------------------------------------------------------------------
    .m_axis_tvalid_top(m_axis_tvalid_dut),  // Валидность выходных данных
    .m_axis_tdata_top(m_axis_tdata_dut),    // Выходной пиксель (RGB / только зеленый)
    .m_axis_tlast_top(m_axis_tlast_dut),    // Задержанный конец строки
    .m_axis_tuser_top(m_axis_tuser_dut),    // Задержанное начало кадра
    .m_axis_tready_top(m_axis_tready_dut)   // Готовность следующего модуля принимать данные
);

initial begin 

// 1. Открытие файлов
    file_in = $fopen(INPUT_FILE, "r");
    file_out = $fopen(OUTPUT_FILE, "w");

    if (file_in == 0) begin
        $display("Ошибка: не удалось открыть файл %s", INPUT_FILE);
        $finish; 
    end

// 2. Инициализируем входные сигналы (чтобы не было неопределенности 'X')
    reset = 1'b0;

// 3. Сразу убираем состояние 'X' со всех входных линий AXI
    s_axis_tvalid_dut = 1'b0;
    s_axis_tdata_dut  = 24'b0;
    s_axis_tlast_dut  = 1'b0;
    s_axis_tuser_dut  = 1'b0;
    
    // Показываем фильтру, что мир вокруг готов забирать его вычисления
    m_axis_tready_dut = 1'b1; 

// 4. Ждем пару наносекунд и делаем аппаратный сброс
    #10;
    do_reset(); // Ваша задача сброса, которую мы обсудили

    // ХОЛОСТАЯ ПАУЗА (Даем схеме стабилизироваться после сброса)
    s_axis_tvalid_dut = 1'b0; // Убеждаемся, что валид пока выключен
    repeat (10) @(posedge clk); // Просто крутим тактовую частоту 10 тактов вхолостую

// 5. Подача изображения в тестируемый модуль
    for(row = 0; row < HEIGHT; row = row + 1) begin 
        for(col = 0; col < WIDTH; col = col + 1) begin 

            // Чтение пикселя из файла
            if($fscanf(file_in, "%d", pixel_in) != 1) begin
                $error("Ошибка чтения в row=%0d, col=%0d", row, col);
                $finish;
            end
            // Сначала выставляем ВСЕ сигналы для текущего такта
                s_axis_tvalid_dut = 1'b1;
                s_axis_tdata_dut  = {pixel_in, pixel_in, pixel_in};
                
                // Условия флагов (чистые комбинаторные присваивания)
                s_axis_tuser_dut = (row == 0 && col == 0) ? 1'b1 : 1'b0;
                s_axis_tlast_dut     = (col == WIDTH - 1)     ? 1'b1 : 1'b0;

                // И только ТЕПЕРЬ ждем, пока DUT защелкнет эти выставленные данные по фронту такта!
                do begin
                    @(posedge clk);
                end while (!s_axis_tready_dut);
        end
    end

    s_axis_tvalid_dut = 1'b0;
    s_axis_tlast_dut  = 1'b0;

    // Ждем, пока из фильтра выйдет самый последний пиксель.
    // Мы можем просто подождать, пока выходной валид снова не упадет в 0
    repeat (10) @(posedge clk); // Даем конвейеру сделать еще несколько тактов
    while (m_axis_tvalid_dut) @(posedge clk); 

    // Финал симуляции
    $fclose(file_in);
    $fclose(file_out);
    $display("Симуляция успешно завершена! Проверяйте файл результатов.");
    $finish;
    
end

// Записываем пиксели в файл
assign pixel_out = m_axis_tdata_dut[23:16];

always_ff @(posedge clk) begin
    if (m_axis_tvalid_dut ) begin
        $fwrite(file_out, "%d\n", pixel_out);
    end
end

endmodule