#include <ap_axi_sdata.h>
#include <hls_stream.h>

int const WIDTH = 640;
int const HEIGHT = 480;
int const HIST = 256;
int const TOTAL_PIXELS = WIDTH * HEIGHT; // 307200 пикселей

typedef ap_axiu<24, 1, 1, 1> video_pixel;
typedef hls::stream<video_pixel> video_stream;

// Гистограмма
void Histogram(video_stream& stream_in, unsigned int hist[HIST])
{
	#pragma HLS INTERFACE axis port=stream_in
    #pragma HLS INTERFACE s_axilite port=hist bundle=CTRL
    #pragma HLS INTERFACE s_axilite port=return bundle=CTRL

	// Локальная копия для работы 
	ap_uint<32> local_hist[256];
	#pragma HLS BIND_STORAGE variable=local_hist type=ram_2p impl=bram

	// Регистры конвейера
	ap_uint<8> addr_d1 = 0, addr_d2 = 0;
	ap_uint<32> data_d1 = 0, data_d2 = 0;
	bool valid_d1 = 0, valid_d2 = 0;

	// 1. Очистка hist_mem
	clear_loop: for(int i = 0; i < HIST; i++) {
		#pragma HLS PIPELINE II=1
		local_hist[i] = 0;
	}

	// Синхронизация кадра (SOF)
	video_pixel start_pixel;
	wait_sof: do {
		#pragma HLS PIPELINE II=1
		stream_in.read(start_pixel);
	} while (start_pixel.user == 0);

	video_pixel pixel_in;

	// Основной цикл + 2 такта на «дожим»
	video_loop: for (int i = 0; i < TOTAL_PIXELS + 2; i++) {
	    #pragma HLS PIPELINE II=1
		#pragma HLS DEPENDENCE variable=local_hist inter false

		bool valid_in = (i < TOTAL_PIXELS);					// Пиксели приходят валидные

		ap_uint<32> old_val = 0;							// Значение для хранения прочитанного из BRAM
		ap_uint<8> p_in = 0;								// Пришедший пиксель

		if(valid_in) {
			if (i == 0) {
				pixel_in = start_pixel;
			} else {
				stream_in.read(pixel_in);
			}

			p_in = pixel_in.data(23, 16);		// Читаем входной пиксель
			old_val = local_hist[p_in];

		} // end valid_in

		// === FORWARDING ===
		ap_uint<32> actual_val = old_val;
		if (valid_d1 && (p_in == addr_d1)) {
			actual_val = data_d1;
		} else if (valid_d2 && (p_in == addr_d2)) {
			actual_val = data_d2;
		}

		// Запись результата, после двух тактов
		if (valid_d2) {
			local_hist[addr_d2] = data_d2;
		}

		// Сдвиг конвейера
		addr_d2  = addr_d1;   addr_d1  = p_in;
		data_d2  = data_d1;   data_d1  = actual_val  + 1;
		valid_d2 = valid_d1;  valid_d1 = valid_in;


	} // video_loop

	// Запись гистограммы в регистры axi-lite
	hist_loop: for(int i = 0; i < HIST; i++) {
		#pragma HLS PIPELINE II=1
		hist[i] = local_hist[i];
	} // end hist_loop

} // end Histogram













