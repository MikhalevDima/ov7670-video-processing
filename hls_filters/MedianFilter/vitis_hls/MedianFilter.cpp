#include "hls_stream.h"
#include "ap_axi_sdata.h"
#include "ap_int.h"

int const WIDTH = 640;
int const HEIGHT = 480;


typedef ap_axiu<24, 1, 1, 1> video_pixel;
typedef hls::stream<video_pixel> video_stream;

// Структура для хранения данных пикселя в формате RGB
struct rgb_data {
	ap_uint<8> r;
	ap_uint<8> g;
	ap_uint<8> b;
};

// Вспомогательный макрос/функция для обмена элементов (Compare and Swap)
#define CAS(a, b) if (a > b) { ap_uint<8> tmp = a; a = b; b = tmp; }

// Функция сортировки
rgb_data window_sort(rgb_data window[3][3]){
    #pragma HLS INLINE

    // 1. Копируем данные из окна в плоские массивы для каждого канала отдельно,
    // чтобы HLS мог развернуть операции в параллельные компараторы.
    ap_uint<8> r[9], g[9], b[9];
    #pragma HLS ARRAY_PARTITION variable=r complete
    #pragma HLS ARRAY_PARTITION variable=g complete
    #pragma HLS ARRAY_PARTITION variable=b complete

    int idx = 0;
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            #pragma HLS UNROLL
            r[idx] = window[i][j].r;
            g[idx] = window[i][j].g;
            b[idx] = window[i][j].b;
            idx++;
        }
    }

    // 2. Оптимальная сортирующая сеть для 9 элементов (всего 25 шагов сравнения).
    // Все три канала выполняют одинаковые перестановки параллельно в логике FPGA.
    CAS(r[0], r[1]); CAS(g[0], g[1]); CAS(b[0], b[1]);
    CAS(r[3], r[4]); CAS(g[3], g[4]); CAS(b[3], b[4]);
    CAS(r[6], r[7]); CAS(g[6], g[7]); CAS(b[6], b[7]);

    CAS(r[1], r[2]); CAS(g[1], g[2]); CAS(b[1], b[2]);
    CAS(r[4], r[5]); CAS(g[4], g[5]); CAS(b[4], b[5]);
    CAS(r[7], r[8]); CAS(g[7], g[8]); CAS(b[7], b[8]);

    CAS(r[0], r[1]); CAS(g[0], g[1]); CAS(b[0], b[1]);
    CAS(r[3], r[4]); CAS(g[3], g[4]); CAS(b[3], b[4]);
    CAS(r[6], r[7]); CAS(g[6], g[7]); CAS(b[6], b[7]);

    CAS(r[0], r[3]); CAS(g[0], g[3]); CAS(b[0], b[3]);
    CAS(r[1], r[4]); CAS(g[1], g[4]); CAS(b[1], b[4]);
    CAS(r[2], r[5]); CAS(g[2], g[5]); CAS(b[2], b[5]);

    CAS(r[3], r[6]); CAS(g[3], g[6]); CAS(b[3], b[6]);
    CAS(r[4], r[7]); CAS(g[4], g[7]); CAS(b[4], b[7]);
    CAS(r[5], r[8]); CAS(g[5], g[8]); CAS(b[5], b[8]);

    CAS(r[0], r[3]); CAS(g[0], g[3]); CAS(b[0], b[3]);
    CAS(r[1], r[4]); CAS(g[1], g[4]); CAS(b[1], b[4]);
    CAS(r[2], r[5]); CAS(g[2], g[5]); CAS(b[2], b[5]);

    CAS(r[1], r[3]); CAS(g[1], g[3]); CAS(b[1], b[3]);
    CAS(r[5], r[7]); CAS(g[5], g[7]); CAS(b[5], b[7]);

    CAS(r[2], r[6]); CAS(g[2], g[6]); CAS(b[2], b[6]);

    CAS(r[2], r[4]); CAS(g[2], g[4]); CAS(b[2], b[4]);
    CAS(r[4], r[6]); CAS(g[4], g[6]); CAS(b[4], b[6]);

    CAS(r[2], r[3]); CAS(g[2], g[3]); CAS(b[2], b[3]);
    CAS(r[5], r[6]); CAS(g[5], g[6]); CAS(b[5], b[6]);

    // 3. После полной сортировки медианой является центральный (4-й) элемент.
    rgb_data output_pixel;
    output_pixel.r = r[4];
    output_pixel.g = g[4];
    output_pixel.b = b[4];

    return output_pixel;
}

// Функция фильтрации
void MedianFilter(video_stream& stream_in, video_stream& stream_out, unsigned int threshold){

#pragma HLS INTERFACE mode=axis port=stream_in
#pragma HLS INTERFACE mode=axis port=stream_out
#pragma HLS INTERFACE mode=s_axilite port=threshold bundle=CTRL_BUS
#pragma HLS INTERFACE mode=s_axilite port=return bundle=CTRL_BUS

	// Строчный буфер 2 строки
	rgb_data linebuffer[2][WIDTH];
	#pragma HLS ARRAY_PARTITION variable=linebuffer complete dim=1

	// Буфер для окна
	rgb_data window[3][3];
	#pragma HLS ARRAY_PARTITION variable=window complete dim=0

	// Читаем кадр
	video_row: for(int r = 0; r < HEIGHT; r ++){
		video_col: for(int c = 0; c < WIDTH; c ++){
			#pragma HLS PIPELINE II=1

			rgb_data pixel_in;
			video_pixel pixel_of_stream_in;
			// Читаем пиксель из входного потока
			stream_in.read(pixel_of_stream_in);
			pixel_in.r = pixel_of_stream_in.data.range(23, 16);
			pixel_in.g = pixel_of_stream_in.data.range(15, 8);
			pixel_in.b = pixel_of_stream_in.data.range(7, 0);

			// Сохраняем принятые пиксели в линейный буфер
			rgb_data r0;
			rgb_data r1;
			r0 = linebuffer[0][c];
			r1 = linebuffer[1][c];

			linebuffer[0][c] = r1;
			linebuffer[1][c] = pixel_in;

			// Сдвигаем окно в право
			for (int i = 0; i < 3; i++) {
				#pragma HLS UNROLL
				window[i][0] = window[i][1];
				window[i][1] = window[i][2];
			}

			// Заполняем окно
			window[0][2] = r0;
			window[1][2] = r1;
			window[2][2] = pixel_in;


			// Логика центра окна
			int center_r = r - 1;
			int center_c = c - 1;

			// Обработка с учётом краёв кадра
			rgb_data filtered_pixel;
			rgb_data pixel_out;
			// Ждём, когда запишем одну строку и один пиксель. Чтобы на след. пиксели, первый пиксель
			// был в центре окна
			if(r >= 1 && c >= 1) {

				// Учитываем края
				if(center_r == 0 || center_c == 0 || center_r == HEIGHT-1 || center_c == WIDTH-1){

					pixel_out.r = window[1][1].r;
					pixel_out.g = window[1][1].g;
					pixel_out.b = window[1][1].b;

				} else {

				filtered_pixel = window_sort(window);

				// Сравниваем с порогом
				pixel_out.r = (window[1][1].r < (ap_uint<8>)threshold) ? (window[1][1].r) : (filtered_pixel.r);
				pixel_out.g = (window[1][1].g < (ap_uint<8>)threshold) ? (window[1][1].g) : (filtered_pixel.g);
				pixel_out.b = (window[1][1].b < (ap_uint<8>)threshold) ? (window[1][1].b) : (filtered_pixel.b);

				}

			} else {

				pixel_out = pixel_in;

			}

			// Формируем выходной пакет
			video_pixel pixel_of_stream_out;
			pixel_of_stream_out.data.range(23, 16) = pixel_out.r;
			pixel_of_stream_out.data.range(15, 8)  = pixel_out.g;
			pixel_of_stream_out.data.range(7, 0)   = pixel_out.b;

			// Компилятор Vitis HLS сам автоматически построит внутренний конвейер задержки
			// для этих сигналов ровно на столько тактов, сколько занимает обработка цвета.
			pixel_of_stream_out.keep = pixel_of_stream_in.keep;
			pixel_of_stream_out.strb = pixel_of_stream_in.strb;
			pixel_of_stream_out.user = pixel_of_stream_in.user;
			pixel_of_stream_out.last = pixel_of_stream_in.last;

			stream_out.write(pixel_of_stream_out);
		}
	}

}











