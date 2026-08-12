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

// Метод применения bias
void bias(video_stream& stream_in,
		video_stream& stream_out,
		ap_uint<8>*gain)
{
	// Локальная переменная для, пакетного режима чтения/записи в память DDR
	// создадим в BRAM
	ap_uint<8> buffer_gain[WIDTH];
	#pragma HLS bind_storage variable=buffer_gain type=RAM_2P impl=BRAM

	/* Задержка для синхронизации кадра */
	// Переменная для вычитки мусорных пикселей
		video_pixel start_pixel;

		do{
			#pragma HLS PIPELINE II=1
			stream_in.read(start_pixel);
		} while (start_pixel.user == 0);


	/* Конец задержки пикселя */

	// Переменная для чтения текущего пикселя потока
	video_pixel pixel_of_stream_in;
	video_pixel pixel_of_stream_out;

	// Читаем кадр из axi-stream и читаем gain из DDR
			video_row: for(int r = 0; r < HEIGHT; r++){

				// Читаем значения gain из DDR в BRAM
				// Вычисляем адрес для чтения
				int ddr_offset = r * WIDTH;
				read_row_for_ddr: for (int r_ddr = 0; r_ddr < WIDTH; r_ddr++){
					#pragma HLS PIPELINE II=1
					buffer_gain[r_ddr] = gain[ddr_offset + r_ddr];
				}

				video_col: for(int c = 0; c < WIDTH; c++){
					#pragma HLS PIPELINE II=1

					if(r == 0 && c == 0) {
						// Используем сохраненный при синхронизации пиксель
						pixel_of_stream_in = start_pixel;
					} else {

						// Для всех остальных позиций читаем из стрима нормально
						stream_in.read(pixel_of_stream_in);

					}

					// Принимаем пиксель из входного потока
					ap_uint<8> pixel_in = pixel_of_stream_in.data.range(23, 16);	// Читаем только зелёный канал


					// Применяем одноточечную КГШ
					ap_uint<16> pixel_KGH = (ap_uint<16>)pixel_in * buffer_gain[c];

					// 1. Сдвигаем вправо на 7 бит, но сохраняем результат в 16-битную переменную,
					// чтобы не потерять значения больше 255 (например, 300)
					ap_uint<16> pixel_scaled = pixel_KGH >> 7;

					// 2. Делаем проверку ОДИН раз, используя уже сдвинутое значение
					ap_uint<8> pixel_clipping = (pixel_scaled > 255) ? (ap_uint<8>)255 : (ap_uint<8>)pixel_scaled;

					// Выходной поток

					// 1. Собираем обратно поток RGB888
					ap_uint<24> pixel_rgb;
					pixel_rgb.range(23, 16) = pixel_clipping;
					pixel_rgb.range(15, 8) = pixel_clipping;
					pixel_rgb.range(7, 0) = pixel_clipping;
					pixel_of_stream_out.data = pixel_rgb;

					// 2. Формирование выходного потока
					// Настройки управляющих сигналов AXI-Stream
					pixel_of_stream_out.keep = -1; // Все единицы
					pixel_of_stream_out.strb = -1; // Все единицы

					// Восстанавливаем сигналы разметки кадра:
					// USER ставим в 1 только на самом первом пикселе кадра
					pixel_of_stream_out.user = (r == 0 && c == 0) ? 1 : 0;
					// LAST ставим в 1 в конце каждой строки видео
					pixel_of_stream_out.last = (c == WIDTH - 1) ? 1 : 0;

					// Отправляем пиксель в выходной поток
					stream_out.write(pixel_of_stream_out);


				}
		}
}

void calculate_bias(video_stream& stream_in,
				ap_uint<8> *ddr_in,
				video_stream& stream_out)
{
	// Локальная переменная для, пакетного режима чтения/записи в память DDR
	// создадим в BRAM
	ap_uint<8> buffer[WIDTH];
	#pragma HLS bind_storage variable=buffer type=RAM_2P impl=BRAM

	 video_pixel dummy;

	// СТРАХОВКА: Вычищаем FIFO от старого кадра, если мы включились посреди него.
	// Читаем поток до тех пор, пока не встретим конец строки/кадра (last == 1)
	// Это гарантирует, что мы сбросили весь застрявший из-за переключения режимов мусор.

	 do {
		#pragma HLS PIPELINE II=1
		stream_in.read(dummy);
	} while (dummy.last == 0);


	/* Задержка для синхронизации кадра */
	// Переменная для вычитки мусорных пикселей
		video_pixel start_pixel;

		do{
			#pragma HLS PIPELINE II=1
			stream_in.read(start_pixel);
		} while (start_pixel.user == 0);


	/* Конец задержки пикселя */

	// Переменная для чтения текущего пикселя потока
	video_pixel pixel_of_stream_in;
	video_pixel pixel_of_stream_out;

	// Читаем поток и зписываем его в ddr
	video_row: for(int r = 0; r < HEIGHT; r++) {
		video_col: for(int c = 0; c < WIDTH; c++){

			// Приём первого пикселя
			if( c == 0 && r == 0){
				pixel_of_stream_in = start_pixel;
			} else {
				stream_in.read(pixel_of_stream_in);
			}

			// Принимаем пиксель из входного потока
			ap_uint<8> pixel_in = pixel_of_stream_in.data.range(23, 16);	// Читаем только зелёный канал

			// Пишем пиксель в локальный буфер
			buffer[c] = pixel_in;


			// Передадим поток так же на выход
			ap_uint<8> pixel_out = pixel_in;

			// Собираем обратно поток RGB888
			ap_uint<24> pixel_rgb;
			pixel_rgb.range(23, 16) = pixel_out;
			pixel_rgb.range(15, 8) = pixel_out;
			pixel_rgb.range(7, 0) = pixel_out;
			pixel_of_stream_out.data = pixel_rgb;

			// Настройки управляющих сигналов AXI-Stream
			pixel_of_stream_out.keep = -1; // Все единицы
			pixel_of_stream_out.strb = -1; // Все единицы

			// Восстанавливаем сигналы разметки кадра:
			// USER ставим в 1 только на самом первом пикселе кадра
			pixel_of_stream_out.user = (r == 0 && c == 0) ? 1 : 0;
			// LAST ставим в 1 в конце каждой строки видео
			pixel_of_stream_out.last = (c == WIDTH - 1) ? 1 : 0;


			// Отправляем пиксель в выходной поток
			stream_out.write(pixel_of_stream_out);

		}

		// Сбрасываем накопленную строку в ddr
		// Вычисляем смещение
		int ddr_offset = r * WIDTH;

		// Пишем в ddr
		write_row_to_ddr: for(int c = 0; c < WIDTH; c++){
			#pragma HLS PIPELINE II=1

			// HLS автоматически превратит этот последовательный цикл в Burst Write
			ddr_in[ddr_offset + c] = buffer[c];
		}
	}
}


// Вспомогательная функция для режима байпаса (mode = 0)
void bypass(video_stream& stream_in,
		video_stream& stream_out)
{
	// Первый пиксель
	video_pixel start_pixel;

	do {
		stream_in.read(start_pixel);
	} while(start_pixel.user == 0);

	// Принимаем весь кадр
	video_pixel pixel_in;
	video_row: for(int r = 0; r < HEIGHT; r ++){
		video_col: for(int c = 0; c < WIDTH; c ++){

			// Обрабатываем первый пиксель
			if(r == 0 && c == 0){
				pixel_in = start_pixel;
			} else {
				stream_in.read(pixel_in);
			}

			// Передаём пиксель во входной поток без изменений
			stream_out.write(pixel_in);
		}
	}
}

// Топ функция
void NUC(video_stream& video_in,
	    video_stream& video_out,
	    ap_uint<8>*   ddr_mem,  // Общий указатель на DDR для чтения и записи
	    ap_uint<8>    mode)      // НАШ УПРАВЛЯЮЩИЙ РЕГИСТР
{

	// 1. Настройка видеопотоков (AXI-Stream)
	#pragma HLS INTERFACE axis port=video_in
	#pragma HLS INTERFACE axis port=video_out

	// 2. Настройка мастера памяти (AXI4-Master для работы с DDR)
	// bundle=gmem связывает этот порт с общей системной шиной памяти
	#pragma HLS INTERFACE m_axi port=ddr_mem offset=slave bundle=gmem depth=307200

	// 3. НАСТРОЙКА РЕГИСТРОВ УПРАВЛЕНИЯ (AXI4-Lite)
	// bundle=control объединяет эти параметры в одну общую шину управления
	#pragma HLS INTERFACE s_axilite port=mode bundle=control
	#pragma HLS INTERFACE s_axilite port=ddr_mem bundle=control // Передаем адрес памяти тоже через AXI-Lite
	#pragma HLS INTERFACE s_axilite port=return bundle=control  // Регистр управления самим IP-блоком (start, done, idle)

	// Логика выбора режима работы по значению регистра mode
	switch(mode) {
		case 0:
			bypass(video_in, video_out);
			break;
		case 1:
			bias(video_in, video_out, ddr_mem);
			break;
		case 2:
			calculate_bias(video_in, ddr_mem, video_out);
			break;
		default:
			bypass(video_in, video_out);
			break;
	}
}













