#include "hls_stream.h"
#include "ap_axi_sdata.h"
#include "ap_int.h"
#include "ap_fixed.h"
#include "hls_math.h"

int const WIDTH = 640;
int const HEIGHT = 480;

typedef ap_axiu<24, 1, 1, 1> video_pixel;
typedef hls::stream<video_pixel> video_stream;


// Структура для хранения пикселя
struct rgb_data {
	ap_uint<8> r;
	ap_uint<8> g;
	ap_uint<8> b;
};

// SimpleDDE
void SimpleDDE (video_stream& stream_in, video_stream& stream_out, int K){

#pragma HLS INTERFACE mode=axis port=stream_in
#pragma HLS INTERFACE mode=axis port=stream_out
#pragma HLS INTERFACE mode=s_axilite port=K bundle=CTRL_BUS
#pragma HLS INTERFACE mode=s_axilite port=return bundle=CTRL_BUS


	// Переводим значение int в float
	ap_fixed<8, 4> K_fixed;

	// Строчный буфер 2 строки
	ap_uint<8> linebuffer[2][WIDTH];
	#pragma HLS ARRAY_PARTITION variable=linebuffer complete dim=1

	// Буфер для окна
	ap_uint<8> window[3][3];
	#pragma HLS ARRAY_PARTITION variable=window complete dim=0

	// Читаем кадр
	video_row: for(int r = 0; r < HEIGHT; r++){
		video_col: for(int c = 0; c < WIDTH; c++){
			#pragma HLS PIPELINE II=1

			// Выходной пиксель
			video_pixel pixel_of_stream_out;

			// Читаем пиксель из входного потока
			ap_uint<8> pixel_in;
			video_pixel pixel_of_stream_in;
			stream_in.read(pixel_of_stream_in);
			pixel_in = pixel_of_stream_in.data.range(23, 16);	// Читаем только зелёный канал


			// Сохраняем принятый пиксель в линейный буфер
			ap_uint<8> r0;
			ap_uint<8> r1;

			r0 = linebuffer[0][c];	// Сохраняем значение первого линейного буфера
			r1 = linebuffer[1][c];	// Сохраняем значение второго линейного буфера

			// Обновляем буфер
			linebuffer[0][c] = r1;
			linebuffer[1][c] = pixel_in;

			// Заполняем окно пикселями
			// 1. Сдвигаем значение в окне в право
			for(int i = 0; i < 3; i++){
				#pragma HLS UNROLL
				window[i][0] = window[i][1];
				window[i][1] = window[i][2];
			}

			// 2. Заполняем окно
			window[0][2] = r0;
			window[1][2] = r1;
			window[2][2] = pixel_in;


			// TO DO сделать обработку краёв
			if(r >= 2 && c >= 2) {

				// 1. Простое размытие (3х3 boxing фильтр)
				int sum = 0;
				for(int i = 0; i < 3; i++) {
					for(int j = 0; j < 3; j++){
						sum += window[i][j];
					}
				}
				ap_uint<8> blur = sum / 9;

				// 2. Выделение деталей (оригинал находится в центре она)
				ap_uint<8> orig = window[1][1];
				int detalis = (int)orig - (int)blur;

				// 3. Считаем локальную контрасность в окне 3х3
				int local_variance = 0;
				for(int i = 0; i < 3; i++){
					for(int j = 0; j < 3; j++){
						local_variance += hls::abs((int)orig - (int)window[i][j]);
					}
				}
				if(local_variance < 40) {
					// Вариация очень мала — это либо ровный тон, либо мелкий шум матрицы
					K_fixed = 0;
				} else if(local_variance < 150) {
					// Средняя вариация — плавные переходы или текстуры, слегка усиливаем
					K_fixed = 1;
				} else if(local_variance < 600) {
					// Высокая вариация — четкие границы объектов, усиливаем на максимум
					K_fixed.range(7, 0) = K;
				} else {
					// Экстремальная вариация — резкий контрастный стык (черное/белое)
					K_fixed = 1;
				}


				// 4. Сглаживание динамического диапазона через плавную интерполяцию
				int compress_base = 0;

				if (blur < 64) {
				    // Зона 1: от 0 до 64. Расстояние = 64 (сдвиг >> 6)
				    // Плавно идем от выставочного значения 0 до 128
				    int x = (int)blur;
				    compress_base = (x * 128) >> 6;
				}
				else if (blur < 192) {
				    // Зона 2: от 64 до 192. Расстояние = 128 (сдвиг >> 7)
				    // Плавно переходим от значения 128 к значению 200
				    int x = (int)blur - 64;
				    // Формула: Start_Y + (Delta_Y * x) / Delta_X
				    // 128 + ((200 - 128) * x) / 128  =>  128 + (72 * x) >> 7
				    compress_base = 128 + ((72 * x) >> 7);
				}
				else {
				    // Зона 3: от 192 до 255. Расстояние ~ 64 (для простоты сдвига берем >> 6)
				    // Плавно переходим от 200 к 255
				    int x = (int)blur - 192;
				    // 200 + ((255 - 200) * x) / 63. Заменим деление на 63 быстрым сдвигом на 64 (>> 6):
				    // 200 + (55 * x) >> 6
				    compress_base = 200 + ((55 * x) >> 6);
				}

				// 5. Усиление деталей
				int enhanced = (int)compress_base + (K_fixed * detalis);

				// 6. Ограничение диапазона (Clipping)
				if(enhanced > 255) enhanced = 255;
				if(enhanced < 0) enhanced = 0;


				// Собираем обратно поток RGB888
				ap_uint<24> pixel_rgb;
				pixel_rgb.range(23, 16) = enhanced;
				pixel_rgb.range(15, 8) = enhanced;
				pixel_rgb.range(7, 0) = enhanced;
				pixel_of_stream_out.data = pixel_rgb;

			} else {
				// На краях оставляем пиксели без изменений
				pixel_of_stream_out.data = pixel_of_stream_in.data;
			}

			// Формирование выходного пакета
			pixel_of_stream_out.keep = pixel_of_stream_in.keep;
			pixel_of_stream_out.strb = pixel_of_stream_in.strb;
			pixel_of_stream_out.user = pixel_of_stream_in.user;
			pixel_of_stream_out.last = pixel_of_stream_in.last;

			stream_out.write(pixel_of_stream_out);
		}
	}

}



















