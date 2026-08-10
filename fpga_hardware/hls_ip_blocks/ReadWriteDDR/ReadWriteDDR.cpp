#include "hls_stream.h"
#include "ap_axi_sdata.h"

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

// 1. Подфункция для чтения стрима и записи в DDR
void stream_to_ddr(
		video_stream& stream_in,
		ap_uint<8> *frame_write,
		ap_uint<32> *stat)
{
	/* Пример как указать где хранить локальный буфер

		unsigned char local_buffer[2048];
		Вариант А: Принудительно разместить в Block RAM (двухпортовый type=RAM_2P)
		#pragma HLS bind_storage variable=local_buffer type=RAM_2P impl=BRAM

		Вариант Б: Принудительно разместить в LUT-памяти (Distributed RAM)
		#pragma HLS bind_storage variable=local_buffer type=RAM_2P impl=LUTRAM

	 */

		// Локальная переменная для, пакетного режима чтения/записи в память DDR
		// создадим в BRAM
		ap_uint<8> row_buffer_write[WIDTH];
		#pragma HLS bind_storage variable=row_buffer_write type=RAM_2P impl=BRAM

		// Локальные переменные для хранения статистики пикселей
		// создадим в LUT
		ap_uint<8> min_val = 255;
		ap_uint<8> max_val = 0;
		ap_int<32> sum_val = 0;


		/* Задержка для синхронизации кадра */
		// Переменная для вычитки мусорных пикселей
		video_pixel dummy_pixel;

		do{
			stream_in.read(dummy_pixel);
		} while (dummy_pixel.user == 0);

		row_buffer_write[0] = dummy_pixel.data.range(23, 16);		// Приняли первый пиксель

		/* Конец задержки пикселя */

		// Начинаем собирать статистику с первого пикселя
		if(row_buffer_write[0] < min_val) min_val = row_buffer_write[0];
		if(row_buffer_write[0] > max_val) max_val = row_buffer_write[0];
		sum_val += row_buffer_write[0];

		// Читаем кадр из axi-stream и записываем в DDR
		video_row: for(int r = 0; r < HEIGHT; r++){

			// 1. Читаем строку из axi-stream в BRAM
			read_row: for(int c = 0; c < WIDTH; c++){
				#pragma HLS PIPELINE II=1

				// Самый первый пиксель всего кадра (0,0) мы УЖЕ прочитали выше!
				// Чтобы не пропустить шаг и не сместить индексы, делаем проверку:
				if (r == 0 && c == 0) {
					// Пропускаем чтение из стрима, так как мы его уже записали перед циклом
					continue;
				}


				// Читаем пиксель из входного потока
				ap_uint<8> pixel_in;
				video_pixel pixel_of_stream_in;
				stream_in.read(pixel_of_stream_in);
				pixel_in = pixel_of_stream_in.data.range(23, 16);	// Читаем только зелёный канал

				// Записываем в локальный буфер
				row_buffer_write[c] = pixel_in;

				// Продолжаем сбор статистики
				if(row_buffer_write[c] < min_val) min_val = row_buffer_write[c];
				if(row_buffer_write[c] > max_val) max_val = row_buffer_write[c];
				sum_val += row_buffer_write[c];

		}

			// 2. Сбрасывам накопленную строку в DDR, в промежутке между строк
			// Вычисляем смещение
			int ddr_offset = r * WIDTH;

			write_row_to_ddr: for(int c = 0; c < WIDTH; c++){
				#pragma HLS PIPELINE II=1

				// HLS автоматически превратит этот последовательный цикл в Burst Write
				frame_write[ddr_offset + c] = row_buffer_write[c];
			}

		}

		// Запись статистики в axi-lite
		ap_uint<8> avg_val = sum_val / (WIDTH * HEIGHT);
		*stat = ((ap_uint<32>(avg_val) << 16) | (ap_uint<32>(max_val) << 8) | (ap_uint<8>(min_val)));

}

// 2. Подфункция чтения из DDR и чтения в стрим
void ddr_to_stream(
		ap_uint<8> *frame_read,
		video_stream& stream_out)
{

	// Локальная переменная для, пакетного режима чтения/записи в память DDR
	// создадим в BRAM
	ap_uint<8> row_buffer_read[WIDTH];
	#pragma HLS bind_storage variable=row_buffer_read type=RAM_2P impl=BRAM

	// Читаем кадр из DDR и делаем axi-stream
		video_row_for_ddr: for(int r = 0; r < HEIGHT; r++){

			// 1. Читаем строку из DDR в буфер
			// Вычисляем адрес для чтения
			int ddr_offset = r * WIDTH;
			read_row_for_ddr: for(int c = 0; c < WIDTH; c++){
				#pragma HLS PIPELINE II=1
				row_buffer_read[c] = frame_read[ddr_offset + c];
			}

			video_col_for_ddr: for(int c = 0; c < WIDTH; c++){
				#pragma HLS PIPELINE II=1

				// Читаем пиксель из входного потока
				ap_uint<8> pixel_out;
				video_pixel pixel_of_stream_out;

				pixel_out = row_buffer_read[c];

				// Собираем обратно поток RGB888
				ap_uint<24> pixel_rgb;
				pixel_rgb.range(23, 16) = pixel_out;
				pixel_rgb.range(15, 8) = pixel_out;
				pixel_rgb.range(7, 0) = pixel_out;
				pixel_of_stream_out.data = pixel_rgb;

				pixel_of_stream_out.keep = 0x7;
				pixel_of_stream_out.strb = 0x7;


				// Внутри цикла ddr_to_stream, где идет запись в stream_out:
				if (r == 0 && c == 0) {
				    pixel_of_stream_out.user = 1;
				} else {
				    pixel_of_stream_out.user = 0; // Это исключит залипание единицы на шине
				}

				if (c == WIDTH - 1) {
					pixel_of_stream_out.last = 1;
				} else {
					pixel_of_stream_out.last = 0;
				}

				stream_out.write(pixel_of_stream_out);

			}

		}

}

// Top Function
void ReadWriteDDR(
		video_stream& stream_in,
		video_stream& stream_out,
		ap_uint<8> *frame_write,
		ap_uint<8> *frame_read,
		ap_uint<32> *stat)
{
	#pragma HLS INTERFACE mode=axis port=stream_in
	#pragma HLS INTERFACE mode=axis port=stream_out

	// 1. Объявляем, что frame_in — это Master AXI порт для чтения данных (depth=(WIDTH*HEIGHT)-только для симуляции)
	// offset=slave - указывает, что адрес будет передаваться через axi-lite
	#pragma HLS interface mode=m_axi port=frame_read offset=slave bundle=gmem_video depth=(WIDTH*HEIGHT)
	#pragma HLS interface mode=m_axi port=frame_write offset=slave bundle=gmem_video depth=(WIDTH*HEIGHT)

	// 2. Для записи статистики в память через axi-lite
	#pragma HLS INTERFACE mode=s_axilite port=stat bundle=CTRL_BUS

	// 3. Объявляем, что адрес для frame_in передается через Slave AXI-Lite
	#pragma HLS interface mode=s_axilite port=frame_read bundle=CTRL_BUS
	#pragma HLS interface mode=s_axilite port=frame_write bundle=CTRL_BUS

	#pragma HLS INTERFACE mode=s_axilite port=return bundle=CTRL_BUS

	// Заставляем HLS создать независимые глубокие FIFO-буферы для входного и выходного стримов
	//#pragma HLS INTERFACE mode=axis port=stream_in register_mode=both depth=640
	//#pragma HLS INTERFACE mode=axis port=stream_out register_mode=both depth=640

	#pragma HLS DATAFLOW	// Конвеер

	stream_to_ddr(stream_in, frame_write, stat);
	ddr_to_stream(frame_read, stream_out);


}
























