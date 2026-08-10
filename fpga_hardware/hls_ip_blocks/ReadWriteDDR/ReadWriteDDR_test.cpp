#include <iostream>
#include "hls_stream.h"
#include "ap_axi_sdata.h"

#define WIDTH 640
#define HEIGHT 480

typedef ap_axiu<24, 1, 1, 1> video_pixel;
typedef hls::stream<video_pixel> video_stream;


// Объявляем Топ функцию
void ReadWriteDDR(
		video_stream& stream_in,
		video_stream& stream_out,
		ap_uint<8> *frame_write,
		ap_uint<8> *frame_read,
		ap_uint<32> *stat);


int main(){

	// 1. Инициализация и виптуальная память

	// Выделение памяти на ПК под DDR буферы
	int frame_size = WIDTH * HEIGHT;

	ap_uint<8> *virtual_ddr_read = new ap_uint<8>[frame_size];
	ap_uint<8> *virtual_ddr_write = new ap_uint<8>[frame_size];
	ap_uint<32> virtual_stat = 0;

	// Объявляем потоки для симуляции
	video_stream sim_stream_in("sim_in");
	video_stream sim_stream_out("sim_out");


	// Подготовка буферов чтения/записи
	for(int i = 0; i < frame_size; i++){
		virtual_ddr_read[i] = 128;			// Буфер для чтения, пишем туда 128
		virtual_ddr_write[i] = 0;			// Буфер для чтения, обнуляем
	}

	// 2. Имитируем входной стрим, 1 кадр
	for(int r = 0; r < HEIGHT; r++){
		for(int c = 0; c < WIDTH; c++){
			video_pixel pix_in;

			// Записываем тестовое изображение в зелёный канал
			pix_in.data.range(23, 16) = (r + c) % 256;	// Просто градиент

			// Настраиваем обязательные сигналы AXI-Stream управления
			pix_in.user = (r == 0 && c == 0) ? 1 : 0;
			pix_in.last = (c == WIDTH - 1) ? 1 : 0;
			pix_in.keep = 0x7;
			pix_in.strb = 0x7;

			// Толкаем пиксель в стрим
			sim_stream_in.write(pix_in);
		}
	}

	// 3. Запуск Топ функции
	std::cout << "Starting HLS Core Simulation..." << std::endl;

	// Вызываем функцию один раз (она обработает один кадр)
	ReadWriteDDR(sim_stream_in, sim_stream_out, virtual_ddr_write, virtual_ddr_read, &virtual_stat);

	std::cout << "Simulation finished. Checking data..." << std::endl;

	// 4. Проверка результата
	int error_count  = 0;

	// Проверка 1: Успешно ли записался кадр из стрима в virtual_ddr_write?
	    for(int r = 0; r < HEIGHT; r++) {
	        for(int c = 0; c < WIDTH; c++) {
	            ap_uint<8> expected_val = (r + c) % 256;
	            ap_uint<8> actual_val = virtual_ddr_write[r * WIDTH + c];

	            if(actual_val != expected_val) {
	                error_count++;
	                if(error_count < 10) { // Выведем только первые несколько ошибок
	                    std::cout << "DDR Write Error at [" << r << "][" << c << "]: "
	                              << "Expected " << expected_val << ", got " << actual_val << std::endl;
	                }
	            }
	        }
	    }

	// Проверка 2: Успешно ли прочитался кадр из virtual_ddr_read во выходной стрим?
	// (Проверяем, что в sim_stream_out лежит ровно то, что мы записали в ddr_read - то есть 128)
	if (sim_stream_out.size() != frame_size) {
		std::cout << "Error: Output stream size mismatch! Expected " << frame_size
				  << ", got " << sim_stream_out.size() << std::endl;
		error_count++;
	}

	while(!sim_stream_out.empty()) {
	    video_pixel p = sim_stream_out.read();
	    // Здесь при желании можно дополнительно проверить p.data, p.user и p.last
	}

	// Освобождаем память на ПК
	delete[] virtual_ddr_read;
	delete[] virtual_ddr_write;

	// 5. Итоги теста
	if (error_count == 0) {
		std::cout << "!!! TEST PASSED SUCCESSFULLY !!!" << std::endl;
		return 0; // HLS поймет, что все супер
	} else {
		std::cout << "!!! TEST FAILED WITH " << error_count << " ERRORS !!!" << std::endl;
		return 1; // HLS заблокирует косимуляцию и покажет ошибку
	}

}























