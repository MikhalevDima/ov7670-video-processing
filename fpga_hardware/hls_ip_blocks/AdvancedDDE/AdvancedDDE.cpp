#include "hls_stream.h"
#include "ap_axi_sdata.h"
#include "ap_int.h"
#include "ap_fixed.h"
#include "hls_math.h"

int const WIDTH = 640;
int const HEIGHT = 480;

typedef ap_axiu<24, 1, 1, 1> video_pixel;
typedef hls::stream<video_pixel> video_stream;

// Переводим згачение int в float
typedef ap_fixed<8, 4> coeff_K;


// Структура для хранения пикселя
struct rgb_data {
	ap_uint<8> r;
	ap_uint<8> g;
	ap_uint<8> b;
};

// AdvancedDDE
void AdvancedDDE (video_stream& stream_in, video_stream& stream_out, ap_uint<8> base_lut[256], ap_uint<8>K_lut[256]){

#pragma HLS INTERFACE mode=axis port=stream_in
#pragma HLS INTERFACE mode=axis port=stream_out
#pragma HLS INTERFACE mode=s_axilite port=base_lut bundle=CTRL_BUS
#pragma HLS INTERFACE mode=s_axilite port=K_lut bundle=CTRL_BUS
#pragma HLS INTERFACE mode=s_axilite port=return bundle=CTRL_BUS

// Говорим, что память должна быть расположена в LUT и доступна за один такт
#pragma HLS RESOURCE variable=base_lut core=RAM_1P_LUTRAM
#pragma HLS RESOURCE variable=K_lut core=RAM_1P_LUTRAM


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
				// 4. Ограничение локальной контрастности до 255
				ap_uint<8> var_index = (local_variance > 255) ? 255 : local_variance;

				// 5. По индексу из K_lut читаем коэффициент, и делем на 16 (т.к. в Vitis *16)
				int K_raw = K_lut[var_index];
				coeff_K K;
				K.range(7, 0) = K_raw; // Превращаем в дробное число деля на 16

				// 6. Читаем динамический диапазон из base_lut по индексу blur
				ap_uint<8> compress_base = base_lut[blur];


				// 7. Усиление деталей
				int enhanced = (int)compress_base + (int)(K * detalis);

				// 8. Ограничение диапазона (Clipping)
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



















