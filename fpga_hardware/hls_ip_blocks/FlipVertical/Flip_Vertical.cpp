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

// Функция переворота по вертикале (верх - низ)
// 1. Запись кадра в память ddr
void Vertical_write_to_ddr(video_stream& stream_in, ap_uint<8> *frame_write){

	// Локальный буфер в BRAM для хранения строки, для записи в DDR
	ap_uint<8> row_buffer_write[WIDTH];
	#pragma HLS bind_storage variable=row_buffer_write type=RAM_2P impl=BRAM

	video_pixel pixel_of_stream_in;				// Переменные для чтения пикселя из текущего потока

	/* Задержка для синхронизации кадра */

	video_pixel start_pixel;					// Переменная для вычитки мусорных пикселей

	do{
		#pragma HLS PIPELINE II=1
		stream_in.read(start_pixel);
	} while (start_pixel.user == 0);


	/* Конец задержки пикселя */

	// Читаем кадр из axi-stream
	video_row: for(int r = 0; r < HEIGHT; r ++){
		video_col: for(int c = 0; c < WIDTH; c++){
			#pragma HLS PIPELINE II=1

			if(r == 0 && c == 0) {
				pixel_of_stream_in = start_pixel;		// Первый принятый пиксель

			} else {
				stream_in.read(pixel_of_stream_in);		// Принимаем остальные пиксели
				}

			// Читаем пиксель из входного потока
			ap_uint<8> pixel_in;
			pixel_in = pixel_of_stream_in.data.range(23, 16);	// Читаем только один канал

			// Записываем принятый пиксель в локальный буфер
			row_buffer_write[c] = pixel_in;

		} // end video_col

		// Сбрасываем накопленную строку в DDR, в промежутке между приёмом строк

		int ddr_offset = r * WIDTH;	// Вычисляем смещение

		write_row_to_ddr: for(int c = 0; c < WIDTH; c++) {
			#pragma HLS PIPELINE II=1
			frame_write[ddr_offset + c] = row_buffer_write[c];
		}

	}	// end video_row

}	// end Vertical_write_to_ddr

// 2. Чтение перевёрнутого кадра из ddr
void Vertical_read_from_ddr(video_stream& stream_out, ap_uint<8> *frame_read){

	// Локальный буфер в BRAM для хранения строки, для чтения из DDR
	ap_uint<8> row_buffer_read[WIDTH];
	#pragma HLS bind_storage variable=row_buffer_read type=RAM_2P impl=BRAM

	video_pixel pixel_of_stream_out;			// Переменные для записи пикселя в текущий поток

	// Читаем кадр из DDR в буфер и формируем выходной axi-stream
	video_row: for(int r = 0; r < HEIGHT; r++){

		int inverted_r = (HEIGHT - 1) - r;			// вычисляем перевёрнутый индекс строки
		int ddr_offset = inverted_r * WIDTH;		// Вычисляем смещение

		read_row_for_ddr: for(int c = 0; c < WIDTH; c++){
			#pragma HLS PIPELINE II=1
			row_buffer_read[c] = frame_read[ddr_offset + c];	// читаем из ddr в буфер (читаем с конца кадра)
		}

		video_col: for(int c = 0; c < WIDTH; c++){

			// Читаем пиксель из буфера
			ap_uint<8> pixel_out;
			pixel_out = row_buffer_read[c];

			// Формируем выходной поток axi-stream
			pixel_of_stream_out.data.range(23, 16) = pixel_out;
			pixel_of_stream_out.data.range(15, 8) = pixel_out;
			pixel_of_stream_out.data.range(7, 0) = pixel_out;

			pixel_of_stream_out.keep = 0x7;
			pixel_of_stream_out.strb = 0x7;

			pixel_of_stream_out.user = (r == 0 && c == 0) ? 1 : 0;
			pixel_of_stream_out.last = (c == WIDTH - 1) ? 1 : 0;

			stream_out.write(pixel_of_stream_out);

		} // end video_col

	} // end video_row

} // end Vertical_read_from_ddr

// 3. Промежуточная функция, чтобы использовать переворот в конвеере, а при Bypass поток проходил без задержек
void Vertical_processing(video_stream& stream_in, ap_uint<8> *frame_write,
						 video_stream& stream_out, ap_uint<8> *frame_read){

	#pragma HLS DATAFLOW
	Vertical_write_to_ddr(stream_in, frame_write);
	Vertical_read_from_ddr(stream_out, frame_read);
}

// Top функция
void Flip_Vertical(video_stream& stream_in,
				  video_stream& stream_out,
				  ap_uint<8> *frame_write,
				  ap_uint<8> *frame_read,
				  ap_uint<8> mode){

	#pragma HLS INTERFACE mode=axis port=stream_in
	#pragma HLS INTERFACE mode=axis port=stream_out

	// 1. Объявляем, что frame_read/write — это Master AXI порт для чтения данных (depth=(WIDTH*HEIGHT)-только для симуляции)
	// offset=slave - указывает, что адрес будет передаваться через axi-lite
	#pragma HLS interface mode=m_axi port=frame_read offset=slave bundle=gmem_read depth=(WIDTH*HEIGHT)
	#pragma HLS interface mode=m_axi port=frame_write offset=slave bundle=gmem_write depth=(WIDTH*HEIGHT)

	// 2. Объявляем, что адрес для frame_read/write передается через Slave axilite
	#pragma HLS interface mode=s_axilite port=frame_read bundle=CTRL_BUS
	#pragma HLS interface mode=s_axilite port=frame_write bundle=CTRL_BUS

	// 3. Управляющий регистр, передаём данные через axilite
	#pragma HLS INTERFACE s_axilite port=mode bundle=CTRL_BUS
	#pragma HLS INTERFACE mode=s_axilite port=return bundle=CTRL_BUS


	// Логика выбора режима работы по значению регистра mode
	switch (mode) {
		case 0:
			Bypass(stream_in, stream_out);
			break;
		case 1:
			Vertical_processing(stream_in, frame_write, stream_out, frame_read);
			break;
		default:
			Bypass(stream_in, stream_out);
			break;
	}

}
























