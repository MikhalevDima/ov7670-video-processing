#include "hls_stream.h"
#include "ap_axi_sdata.h"
#include "ap_int.h"
#include "ap_fixed.h"
#include "hls_math.h"

int const WIDTH = 640;
int const HEIGHT = 480;
int const R = 3;                                    // Радиус окна (7х7)
int const N_PIXELS = 49;                            // Строго 49 пикселей для 7х7

typedef ap_axiu<24, 1, 1, 1> video_pixel;
typedef hls::stream<video_pixel> video_stream;

// ТИПЫ ДЛЯ КОЭФФИЦИЕНТОВ (Расширяем дробную часть для плавной работы 7х7)
typedef ap_fixed<26, 4> coeff_a;                    // 14 бит на дробь, точность идеальная
typedef ap_fixed<32, 12> coeff_b;                   // До 1023.0, 10 бит на дробь

// Потоки для выходов
typedef hls::stream<coeff_a> stream_a_t;
typedef hls::stream<coeff_b> stream_b_t;

// ВНУТРЕННИЕ АККУМУЛЯТОРЫ СТАТИСТИКИ 7х7 (Беззнаковые, чтобы исключить потерю бит)
typedef ap_uint<14> sum_t;                          // Вмещает 255 * 49 = 12495 (требует 14 бит)
typedef ap_uint<22> sum_sq_t;                       // Вмещает 255*255 * 49 = 3186225 (требует 22 бита)
typedef ap_uint<28> var_scale_t;                    // Для масштабированной дисперсии (до 156 млн)

// Типы для финальной fixed-point математики
typedef ap_ufixed<32, 16> var_fixed_t;
typedef ap_ufixed<24, 8> mean_fixed_t;

// Внутреннее FIFO для 8 бит пикселя + 2 бит синхронизации (user, last)
typedef hls::stream<ap_uint<10>> video_fifo_t;

// ВАЖНО: Точный пересчет глубины FIFO задержки под R=3!
// 2 * 3 * 640 + 2 * 3 + 20 = 3840 + 6 + 20 = 3866 пикселей.
// Если оставить 1302, FIFO переполнится на 3-й строке кадра, и ПЛИС зависнет (будет черный экран).
int const DELAY_FIFO_DEPTH = (2 * R * WIDTH) + (2 * R) + 20;

// Вместо деления на 49 используем точную константу INV_N в целых числах.
// 1/49 * 2^14 = 0.02040816 * 16384 = 334.37 -> округляем до 334
// Чтобы вернуть масштаб на место, после умножения сдвигаем вправо на 14 бит (>> 14)
ap_uint<32> const INV_N_INT = 334;


void calc_ab_coefficients(
    video_stream& stream_in,
    video_fifo_t& stream_I_delayed,
    hls::stream<ap_uint<15>>& stream_a, // Выдаем чистое целое 'a' (0..16384)
    hls::stream<ap_uint<22>>& stream_b, // Выдаем чистое целое 'b' (0..417792)
    int EPS
) {

    // Линейный буфер для строк
    ap_uint<8> linebuffer[2*R][WIDTH];
    #pragma HLS ARRAY_PARTITION variable=linebuffer complete dim=1

    // Буфер для окна
    ap_uint<8> window[2*R+1][2*R+1];
    #pragma HLS ARRAY_PARTITION variable=window complete dim=0

    // Читаем кадр
    video_row: for(int r = 0; r < HEIGHT; r++){
        video_col: for(int c = 0; c < WIDTH; c++){
            #pragma HLS PIPELINE II=1

            // 1. Читаем входной пиксель
            video_pixel pixel_of_stream_in = stream_in.read();
            ap_uint<8> pixel_in = pixel_of_stream_in.data.range(15, 8); // Зелёный цвет OV7670

            // 2. Упаковка пикселей и сигналов синхронизации для FIFO задержки
            ap_uint<10> fifo_packet;
            fifo_packet.range(7, 0) = pixel_in;
            fifo_packet.range(8, 8) = pixel_of_stream_in.user;
            fifo_packet.range(9, 9) = pixel_of_stream_in.last;
            stream_I_delayed.write(fifo_packet);

            // 3. Сдвигаем linebuffer вверх
            for(int i = 0; i < 2*R-1; i++){
                #pragma HLS UNROLL
                linebuffer[i][c] = linebuffer[i+1][c];
            }
            linebuffer[2*R-1][c] = pixel_in;

            // Сдвигаем окно влево
            for(int i = 0; i < 2*R+1; i++){
                #pragma HLS UNROLL
                for(int j = 0; j < 2*R; j++){
                    #pragma HLS UNROLL
                    window[i][j] = window[i][j+1];
                }
            }

            // Заполняем правый столбец окна
            for(int i = 0; i < 2*R; i++){
                #pragma HLS UNROLL
                window[i][2*R] = linebuffer[i][c];
            }
            window[2*R][2*R] = pixel_in;

            // 4. Расчёт статистики внутри окна
            if(r >= R && c >= R){
                ap_uint<14> sum = 0;
                ap_uint<22> sum_sq = 0;

                for(int i = 0; i < 2*R+1; i++){
                    #pragma HLS UNROLL
                    for(int j = 0; j < 2*R+1; j++){
                        ap_uint<8> val = window[i][j];
                        sum += val;
                        sum_sq += (ap_uint<22>)val * val;
                    }
                }

                ap_uint<32> const INV_N_INT = 334; // Константа 1/49 * 2^14

                // 1. Считаем среднее КВАДРАТОВ в масштабе 14 бит дроби (НЕ сдвигаем пока!)
                // Максимум: 3186225 * 334 = 1 064 199 150 (отлично влезает в uint32)
                ap_uint<32> mean_sq_scaled = sum_sq * INV_N_INT;

                // 2. Считаем КВАДРАТ СРЕДНЕГО.
                // Сначала находим просто среднее, но сохраняем его в масштабе 14 бит дроби
                ap_uint<32> mean_scaled = sum * INV_N_INT;
                // Теперь возводим его в квадрат. Масштаб дроби становится 14 + 14 = 28 бит!
                ap_uint<64> mean_squared_high = (ap_uint<64>)mean_scaled * (ap_uint<64>)mean_scaled;
                // Чтобы вернуть масштаб к 14 битам дроби (для вычитания), сдвигаем этот квадрат вправо на 14 бит
                ap_uint<32> mean_squared_scaled = mean_squared_high >> 14;

                // 3. Вычисляем ЧЕСТНУЮ дисперсию в масштабе 14 бит дроби
                ap_uint<32> var_scaled = 0;
                if(mean_sq_scaled > mean_squared_scaled) {
                    var_scaled = mean_sq_scaled - mean_squared_scaled;
                }

                // Теперь наша дисперсия var_scaled имеет масштаб 14 бит дроби.
                // Чтобы прибавить к ней EPS (который приходит как обычное целое число),
                // мы обязаны перевести EPS в ТАКОЙ ЖЕ масштаб — сдвинуть его влево на 14 бит!
                ap_uint<48> eps_scaled = (ap_uint<48>)EPS << 14;
                ap_uint<48> denominator = var_scaled + eps_scaled;

                // 4. Вычисляем коэффициент 'a' (в масштабе 14 бит дроби)
                ap_uint<15> a_int;
                if (denominator == 0) {
                    a_int = 16384;
                } else {
                    // Чтобы при делении получить масштаб 14 бит, числитель сдвигаем еще на 14 бит влево
                    ap_uint<48> num_scaled = (ap_uint<48>)var_scaled << 14;
                    a_int = num_scaled / denominator;
                    if(a_int > 16384) a_int = 16384;
                }

                // 5. Вычисляем коэффициент 'b' с абсолютной защитой от переполнения снизу
                ap_uint<8> mean_pure = mean_scaled >> 14;
                ap_uint<32> a_times_mean = (ap_uint<32>)a_int * mean_pure;

                // Используем знаковый тип ap_int для безопасного вычитания
                ap_int<34> b_diff = (ap_int<34>)mean_scaled - (ap_int<34>)a_times_mean;

                ap_uint<22> b_int;
                if(b_diff <= 0) {
                    b_int = 0; // Если округление ушло в минус или ноль — жестко глушим в 0
                } else {
                    // Защита сверху: коэффициент 'b' в нашем масштабе не может быть больше, чем (255 << 14) = 4177920
                    if(b_diff > 4177920) {
                        b_int = 4177920;
                    } else {
                        b_int = (ap_uint<22>)b_diff;
                    }
                }
                // Отправляем в потоки целые числа
                stream_a.write(a_int);
                stream_b.write(b_int);

            } else {
                stream_a.write((ap_uint<15>)16384);
                stream_b.write((ap_uint<22>)0);
            }
        }
    }
}


void mean_ab_smoothing(
    hls::stream<ap_uint<15>>& stream_a, // Входной поток целого 'a' (0..16384)
    hls::stream<ap_uint<22>>& stream_b, // Входной поток целого 'b' (0..417792)
    hls::stream<ap_uint<15>>& mean_a,   // Выходной сглаженный 'mean_a'
    hls::stream<ap_uint<22>>& mean_b    // Выходной сглаженный 'mean_b'
) {
    // ЛИНЕЙНЫЕ БУФЕРЫ на чистых целых числах
    ap_uint<15> linebuffer_a[2*R][WIDTH];
    #pragma HLS ARRAY_PARTITION variable=linebuffer_a complete dim=1

    ap_uint<22> linebuffer_b[2*R][WIDTH];
    #pragma HLS ARRAY_PARTITION variable=linebuffer_b complete dim=1

    // БУФЕРЫ ДЛЯ ОКОН
    ap_uint<15> window_a[2*R+1][2*R+1];
    #pragma HLS ARRAY_PARTITION variable=window_a complete dim=0

    ap_uint<22> window_b[2*R+1][2*R+1];
    #pragma HLS ARRAY_PARTITION variable=window_b complete dim=0

    ap_uint<32> const INV_N_INT = 334; // Константа 1/49 * 2^14

    // Читаем кадр
    video_row_coeff: for(int r = 0; r < HEIGHT; r++){
        video_col_coeff: for(int c = 0; c < WIDTH; c++){
            #pragma HLS PIPELINE II=1

            // 1. Читаем входные целочисленные коэффициенты
            ap_uint<15> cur_a = stream_a.read();
            ap_uint<22> cur_b = stream_b.read();

            // 2. Обновляем linebuffer
            for(int i = 0; i < 2*R-1; i++){
                #pragma HLS UNROLL
                linebuffer_a[i][c] = linebuffer_a[i+1][c];
                linebuffer_b[i][c] = linebuffer_b[i+1][c];
            }
            linebuffer_a[2*R-1][c] = cur_a;
            linebuffer_b[2*R-1][c] = cur_b;

            // Сдвигаем окно влево
            for(int i = 0; i < 2*R+1; i++){
                #pragma HLS UNROLL
                for(int j = 0; j < 2*R; j++){
                    #pragma HLS UNROLL
                    window_a[i][j] = window_a[i][j+1];
                    window_b[i][j] = window_b[i][j+1];
                }
            }

            // Заполняем правый столбец окна
            for(int i = 0; i < 2*R; i++){
                #pragma HLS UNROLL
                window_a[i][2*R] = linebuffer_a[i][c];
                window_b[i][2*R] = linebuffer_b[i][c];
            }
            window_a[2*R][2*R]  = cur_a;
            window_b[2*R][2*R]  = cur_b;

            // 3. Логика расчета средних значений
            // ВАЖНО: Убираем условие "if (r >= R && c >= R)".
            // Поток коэффициентов уже идет с учетом кадра. Нам нельзя сдвигать его второй раз!

            // Расширяем аккумуляторы, чтобы 49 элементов гарантированно не переполнили сумму
            // 16384 * 49 = 802816 (требует 20 бит)
            ap_uint<21> sum_a = 0;
            // 417792 * 49 = 20471808 (требует 25 бит)
            ap_uint<26> sum_b = 0;

            for(int i = 0; i < 2*R+1; i++){
                #pragma HLS UNROLL
                for(int j = 0; j < 2*R+1; j++){
                    #pragma HLS UNROLL
                    sum_a += window_a[i][j];
                    sum_b += window_b[i][j];
                }
            }

            // Умножаем на инверсию (334) и возвращаем масштаб сдвигом >> 14
            ap_uint<15> out_mean_a = (sum_a * INV_N_INT) >> 14;
            ap_uint<22> out_mean_b = (sum_b * INV_N_INT) >> 14;

            // Дополнительная аппаратная защита от переполнения «сверху»
            if(out_mean_a > 16384) out_mean_a = 16384;

            // Пишем чистые целые числа в выходные потоки
            mean_a.write(out_mean_a);
            mean_b.write(out_mean_b);
        }
    }
}

void GuideFilter(video_stream& stream_in, video_stream& stream_out, int EPS){

#pragma HLS INTERFACE mode=axis port=stream_in
#pragma HLS INTERFACE mode=axis port=stream_out
#pragma HLS INTERFACE mode=s_axilite port=EPS bundle=CTRL_BUS
#pragma HLS INTERFACE mode=s_axilite port=return bundle=CTRL_BUS

    // Указываем компилятору, что EPS гарантированно стабилен во время обработки кадра
    #pragma HLS stable variable=EPS

    // 1. Промежуточные потоки для соединения функций на ЧИСТЫХ ЦЕЛЫХ ЧИСЛАХ
    hls::stream<ap_uint<15>> stream_a; // 0..16384
    hls::stream<ap_uint<22>> stream_b; // 0..417792
    hls::stream<ap_uint<15>> mean_a;
    hls::stream<ap_uint<22>> mean_b;

    // FIFO для передачи оригинального задержанного изображения
    video_fifo_t stream_I_delayed;
    // Задаем жесткую глубину буфера в BRAM памяти ПЛИС
    #pragma HLS STREAM variable=stream_I_delayed depth=DELAY_FIFO_DEPTH

    // Активируем параллельный запуск всех подпроцессов кадра
    #pragma HLS DATAFLOW

    // 2. Вызов конвейерных методов расчета
    calc_ab_coefficients(stream_in, stream_I_delayed, stream_a, stream_b, EPS);

    mean_ab_smoothing(stream_a, stream_b, mean_a, mean_b);

    // 3. Цикл финальной сборки базового слоя
    recon_row: for(int r = 0; r < HEIGHT; r++) {
        recon_col: for(int c = 0; c < WIDTH; c++) {
            #pragma HLS PIPELINE II=1

            // Читаем синхронизированные целочисленные коэффициенты (масштаб 2^14)
            ap_uint<15> a = mean_a.read();
            ap_uint<22> b = mean_b.read();

            // Распаковка FIFO
            ap_uint<10> fifo_packet = stream_I_delayed.read();
            ap_uint<8> I_pixel = fifo_packet.range(7, 0);
            ap_uint<1> sync_user = fifo_packet.range(8, 8);
            ap_uint<1> sync_last = fifo_packet.range(9, 9);

            // Вычисляем базовый слой (масштабированный пиксель)
            // Максимальное значение выражения: 16384 * 255 + 417792 = 4177920 + 417792 = 4595712.
            // Для этого числа нам достаточно 23 бит (ap_uint<23>).
            ap_uint<23> base_layer_scaled = (ap_uint<23>)a * I_pixel + b;

            // Возвращаем масштаб обратно: делим на 16384 с помощью сдвига на 14 бит.
            // Чтобы реализовать математическое округление до ближайшего целого (вместо отсечения),
            // перед сдвигом прибавляем половину делителя (2^13 = 8192). Это аналог AP_RND!
            ap_uint<16> base_layer = (base_layer_scaled + 8192) >> 14;

            // Жесткая аппаратная защита диапазона 0-255 (насыщение/saturation)
            ap_uint<8> out_gray;
            if (base_layer > 255) {
                out_gray = 255;
            } else {
                out_gray = (ap_uint<8>)base_layer;
            }

            // Формируем выходной видео-пиксель AXI-Stream
            video_pixel OUT;

            // Выводим монохромное сглаженное изображение во все RGB каналы:
            OUT.data.range(7, 0)   = out_gray; // Синий
            OUT.data.range(15, 8)  = out_gray; // Зеленый
            OUT.data.range(23, 16) = out_gray; // Красный

            // СЛУЖЕБНЫЕ СИГНАЛЫ:
            OUT.keep = 0x7; // 3 байта валидны (24 бита)
            OUT.strb = 0x7;

            // user (Start of Frame) взводится строго на первом пикселе кадра
            OUT.user = sync_user;

            // last (End of Line) взводится на последнем пикселе каждой строки
            OUT.last = sync_last;

            // Отправляем пиксель на выход IP-ядра
            stream_out.write(OUT);
        }
    }
}















