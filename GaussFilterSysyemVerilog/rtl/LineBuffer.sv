module LineBuffer #(
    parameter WIDTH  = 640, // Ширина кадра в пикселях
    parameter HEIGHT = 480 // Высота кадра в пикселях
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
    output logic        s_axis_tready, // Готовность принимать данные
    input logic         s_axis_tvalid, // Валидность входных данных
    input logic [23:0]  s_axis_tdata,  // Входной пиксель (RGB)
    input logic         s_axis_tlast,  // Конец строки (End of Line)
    input logic         s_axis_tuser,  // Начало кадра (Start of Frame)

    // -------------------------------------------------------------------------
    // Интерфейс AXI-Stream Master (Выход задержанного потока)
    // -------------------------------------------------------------------------
    output logic        m_axis_tvalid, // Валидность выходных данных
    output logic [23:0] m_axis_tdata,  // Выходной пиксель (RGB / только зеленый)
    output logic        m_axis_tlast,  // Задержанный конец строки
    output logic        m_axis_tuser,  // Задержанное начало кадра
    input logic         m_axis_tready  // Готовность следующего модуля принимать данные

);

    // -------------------------------------------------------------------------
    // Локальные параметры (Константы)
    // -------------------------------------------------------------------------
    localparam WIDTH_PIXEL = 8;                  // Разрядность одного канала (зеленого)
    localparam WIDTH_BIT   = $clog2(WIDTH);      // Разрядность счетчика пикселей в строке
    localparam HEIGHT_BIT  = $clog2(HEIGHT);     // Разрядность счетчика строк в кадре

    // -------------------------------------------------------------------------
    // Внутренние сигналы и линии связи
    // -------------------------------------------------------------------------
    // Вырезаем только зелёный канал из входной шины RGB
    logic [WIDTH_PIXEL-1:0] input_data;
    assign input_data = s_axis_tdata[23:16];

    // Объявление памяти Block RAM. 
    // Ширина: 10 бит (8 бит пиксель + 1 бит tuser + 1 бит tlast)
    // Глубина: WIDTH ячеек (одна строка кадра)
    (* ram_style="block" *)
    logic [9:0] linebuffer [WIDTH-1:0];         
    
    logic [WIDTH_PIXEL-1:0] output_pixel;  // Прочитанный из памяти пиксель
    logic [WIDTH_BIT-1:0]   read_address;  // Адрес чтения памяти
    logic [WIDTH_BIT-1:0]   write_address; // Адрес записи памяти

    // -------------------------------------------------------------------------
    // Логика управления потоком (AXI-Stream Handshake)
    // -------------------------------------------------------------------------
    logic axis_handshake;
    
    // Готовность входа жестко привязываем к готовности следующего за нами модуля
    assign s_axis_tready  = m_axis_tready;
    
    // Передача данных происходит только при одновременной готовности источника и приемника
    assign axis_handshake = s_axis_tvalid && s_axis_tready;

// -------------------------------------------------------------------------
    // Счётчик координат кадра (Администратор потока)
    // -------------------------------------------------------------------------
    logic [WIDTH_BIT-1:0]  width_reg;
    logic [HEIGHT_BIT-1:0] height_reg;
    logic first_line_done;                                      // Флаг, что первая строка полностью записана

    always_ff @(posedge clk) begin
            if (reset) begin
                width_reg        <= 'b0;
                height_reg       <= 'b0;
                first_line_done  <= 1'b0;
            end
            else if (axis_handshake) begin
                // Если дошли до конца строки
                if (width_reg == (WIDTH - 1)) begin
                    width_reg <= 'd0;
                    // Если дошли до конца кадра
                    if (height_reg == (HEIGHT - 1)) begin
                        height_reg <= 'd0;
                        first_line_done <= 1'b0;    // Сбрасываем флаг, при начале нового кадра
                    // Иначе переходим на следующую строку
                    end else begin
                        height_reg <= height_reg + 1'b1;
                        if(height_reg == 0) first_line_done <= 1'b1;    // Первая строка принята
                    end
                end 
                // Иначе двигаемся по строке вправо
                else begin
                    width_reg <= width_reg + 1'b1;
                end
            end
        end
  

    // -------------------------------------------------------------------------
    // Управление адресацией памяти
    // -------------------------------------------------------------------------

    // Для задержки на 1 такт используем регистр адреса
    logic [WIDTH_BIT-1:0] read_address_delayed;

    always_ff @(posedge clk) begin
        if (reset) begin
            write_address <= 'b0;
            read_address  <= 'b0;
            read_address_delayed <= 1'b0;
        end else if (axis_handshake) begin
            write_address <= width_reg;
            read_address  <= width_reg;
            // Задерживаем адрес чтения на один такт для синхронизации с ram_output_reg
            read_address_delayed <= read_address;
        end
    end

    // -------------------------------------------------------------------------
    // Синхронная работа Block RAM (Чтение и запись по такту)
    // -------------------------------------------------------------------------
    // Переменная для защелкивания полного слова из памяти на такте
    logic [9:0] ram_output_reg;

    always_ff @(posedge clk) begin
        if (axis_handshake) begin
            // Запись: упаковываем служебные биты и пиксель в ячейку
            linebuffer[write_address] <= {s_axis_tuser, s_axis_tlast, input_data};
            
            // Чтение: защелкиваем данные в регистр (Read-Before-Write)
            // Физически прочитается старое значение до того, как перезапишется новым
            ram_output_reg <= linebuffer[read_address_delayed];
        end
    end

    // Распаковка считанного из BRAM слова на целевые сигналы
   // Распаковываем из жестких 15 и 14 битов
    assign m_axis_tuser = ram_output_reg[9];
    assign m_axis_tlast = ram_output_reg[8];
    assign output_pixel = ram_output_reg[7:0];

    // -------------------------------------------------------------------------
    // Выходная логика и управление валидностью
    // -------------------------------------------------------------------------
    // Формируем черно-белую RGB шину (дублируем зеленый канал)
    assign m_axis_tdata = {output_pixel, output_pixel, output_pixel};

    // Синхронный триггер для выходного валида (чтобы не отставать от данных из BRAM)
  
    logic m_axis_tvalid_reg;

    always_ff @(posedge clk) begin
        if (reset) begin
            m_axis_tvalid_reg <= 1'b0;
        end else begin
            // Данные валидны, когда:
            // 1. Прошла хотя бы одна строка (first_line_done)
            // 2. Был handshake на прошлом такте (данные прочитаны из BRAM)
            m_axis_tvalid_reg <= first_line_done && axis_handshake;
        end
    end

    assign m_axis_tvalid = m_axis_tvalid_reg;

endmodule