#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <ap_int.h>

int const WIDTH = 640;
int const HEIGHT = 480;

// Определяем структуру пикселя
typedef ap_axiu<24, 1, 1, 1> video_pixel;
typedef hls::stream<video_pixel> video_stream;

// Структура для хранения данных пикселя в формате RGB
struct rgb_data {
	ap_uint<8> r;
	ap_uint<8> g;
	ap_uint<8> b;
};

// Функция байпасс
void Bypass(video_stream& stream_in, video_stream& stream_out) {

	/* Задержка для синхронизации кадра */

		video_pixel start_pixel;					// Переменная для вычитки мусорных пикселей
		do{
			#pragma HLS PIPELINE II=1
			stream_in.read(start_pixel);
		} while (start_pixel.user == 0);

	/* Конец задержки пикселя */


		video_pixel pixel_of_stream_in;				// Переменные для чтения пикселя из текущего потока
		video_pixel pixel_of_stream_out;			// Переменные для записи пикселя в текущий поток

	// Читаем кадр из потока AXI-STREAM
		video_row: for(int r = 0; r < HEIGHT; r ++){
			video_col: for(int c = 0; c < WIDTH; c++){
				#pragma HLS PIPELINE II=1

				if(r == 0 && c == 0) {
					pixel_of_stream_in = start_pixel;		// Первый принятый пиксель

				} else {
					stream_in.read(pixel_of_stream_in);		// Принимаем остальные пиксели
				}

				// Просто копируем входные пиксели в выходные
				pixel_of_stream_out = pixel_of_stream_in;

				// Выходной поток
				stream_out.write(pixel_of_stream_out);
		}
	}

}

// Функция переворота изображения по горизонтале (слева - направо)
void Horizont(video_stream& stream_in, video_stream& stream_out) {

	// Строчный буфер: две строки шириной WIDTH, храним 8-битный яркостный (grayscale) пиксель.
	// Используем BRAM для хранения больших строчных буферов эффективно в ресурсах FPGA.
	// Не партиционируем первую размерность полностью, чтобы HLS мог реализовать буфер в BRAM.
	ap_uint<8> linebuffer[2][WIDTH];
	#pragma HLS RESOURCE variable=linebuffer core=RAM_2P_BRAM

	/* Задержка для синхронизации кадра */
	video_pixel start_pixel;
	do{
		#pragma HLS PIPELINE II=1
		stream_in.read(start_pixel);
	} while (start_pixel.user == 0);
	/* Конец задержки пикселя */

	video_pixel pixel_of_stream_in;
	video_pixel pixel_of_stream_out;

	ap_uint<1> write_buf = 0;  // Буфер для записи (0 или 1)

	// HEIGHT + 1 для задержки в 1 строку
	video_row: for(int r = 0; r < HEIGHT + 1; r ++){
		video_col: for(int c = 0; c < WIDTH; c++){
			#pragma HLS PIPELINE II=1

			// === ЗАПИСЬ В БУФЕР ===
			if (r < HEIGHT) {
				if(r == 0 && c == 0) {
					pixel_of_stream_in = start_pixel;
				} else {
					stream_in.read(pixel_of_stream_in);
				}

				ap_uint<8> pixel_in = pixel_of_stream_in.data.range(23, 16);
				linebuffer[write_buf][c] = pixel_in;
			}

			// === ВЫВОД ИЗ БУФЕРА ===
			if (r > 0) {
				// Всегда читаем из предыдущего буфера: индекс (write_buf ^ 1)
				ap_uint<8> pixel_out = linebuffer[write_buf ^ 1][WIDTH - 1 - c];

				pixel_of_stream_out.data.range(23, 16) = pixel_out;
				pixel_of_stream_out.data.range(15, 8)  = pixel_out;
				pixel_of_stream_out.data.range(7, 0)   = pixel_out;

				pixel_of_stream_out.keep = 0x7;
				pixel_of_stream_out.strb = 0x7;

				pixel_of_stream_out.user = (r == 1 && c == 0) ? 1 : 0;
				pixel_of_stream_out.last = (c == WIDTH - 1) ? 1 : 0;

				stream_out.write(pixel_of_stream_out);
			}
		} // end c

		if(r < HEIGHT) {
			write_buf = write_buf ^ 1; // Переключаем буфер для записи (xor с 1)
		}
	} // end r
}


// Top функция
void Flip_Horizont(video_stream& stream_in,
				  video_stream& stream_out,
				  ap_uint<8> mode){

	// AXI-Stream вход: видео данные (tdata/tuser/tlast). HLS создаст интерфейс AXIS.
	#pragma HLS INTERFACE mode=axis port=stream_in
	// AXI-Stream выход: видео данные (tdata/tuser/tlast).
	#pragma HLS INTERFACE mode=axis port=stream_out

	// Управляющий регистр: доступ к параметрам по AXI-Lite (control bus).
	#pragma HLS INTERFACE s_axilite port=mode bundle=CTRL_BUS
	#pragma HLS INTERFACE mode=s_axilite port=return bundle=CTRL_BUS

	// Логика выбора режима работы по значению регистра mode

	switch (mode) {
		case 0:
			Bypass(stream_in, stream_out);
			break;
		case 1:
			Horizont(stream_in, stream_out);
			break;
		default:
			Bypass(stream_in, stream_out);
			break;
	} // end switch
}
























