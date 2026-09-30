#include <hls_stream.h>
#include "ap_axi_sdata.h"

int const WIDTH = 640;
int const HEIGHT = 480;

// Определяем структуру пикселя
typedef ap_axiu<24, 1, 1, 1> video_pixel;
typedef hls::stream<video_pixel> video_stream;

// Функция байпасс
void Bypass(
		video_stream& stream_in,
		video_stream& stream_out)
{

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

// Функция смены полярности изображения
void Reversal(
		video_stream& stream_in,
		video_stream& stream_out)

{
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

				ap_uint<8> pixel_in;
				ap_uint<8> pixel_out;

				pixel_in = pixel_of_stream_in.data.range(23,16);
				pixel_out = ~pixel_in;

				// Формируем выходной поток

				pixel_of_stream_out.data.range(23, 0) = (((ap_uint<24>)pixel_out << 16) |
														 ((ap_uint<24>)pixel_out << 8)  |
														 pixel_out);

				pixel_of_stream_out.user = pixel_of_stream_in.user;
				pixel_of_stream_out.last = pixel_of_stream_in.last;
				pixel_of_stream_out.keep = 0x7;
				pixel_of_stream_out.strb = 0x7;

				stream_out.write(pixel_of_stream_out);
			} // end video_col

	} // end video_row
}

// Топ функция
void PolarityReversal(
		video_stream& stream_in,
		video_stream& stream_out,
		ap_uint<8> mode)
{
	#pragma HLS INTERFACE mode=axis port=stream_in
	#pragma HLS INTERFACE mode=axis port=stream_out

	// 2. Для записи статистики в память через axi-lite
	#pragma HLS INTERFACE mode=s_axilite port=mode bundle=CTRL_BUS
	#pragma HLS INTERFACE mode=s_axilite port=return bundle=CTRL_BUS

	switch (mode) {
		case 0:
			Bypass(stream_in, stream_out);
			break;
		case 1:
			Reversal(stream_in, stream_out);
			break;
		default:
			Bypass(stream_in, stream_out);
			break;
	}

} // end PolarityReversal























