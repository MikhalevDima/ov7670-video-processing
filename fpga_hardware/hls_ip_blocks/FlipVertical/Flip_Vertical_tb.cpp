#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <iostream>


typedef ap_axiu<24, 1, 1, 1> video_pixel;
typedef hls::stream<video_pixel> video_stream;

// Объявляем нашу функцию верхнего уровня
void Flip_Vertical(video_stream& stream_in,
				  video_stream& stream_out,
				  ap_uint<8> *frame_write,
				  ap_uint<8> *frame_read,
				  ap_uint<8> mode);

// Вспомогательная структура для ручной проверки в тестбенче
struct test_rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

int main(){

	// Создадим тестовую картинку
	const int width = 640;
	const int height = 480;
	test_rgb input_image[height][width];

	// Выделение памяти на ПК под DDR буферы
	int frame_size = width * height;
	ap_uint<8> *virtual_ddr_read = new ap_uint<8>[frame_size];
	ap_uint<8> *virtual_ddr_write = new ap_uint<8>[frame_size];

	// Подготовка буферов чтения/записи всё обнуляем
	for(int i = 0; i < frame_size; i++){
		virtual_ddr_write[i] = 0;										// Буфер для записи
	}

	// Заполняем тестовую картинку
	std::cout << "--- 1. ИСХОДНОЕ ИЗОБРАЖЕНИЕ ---" << std::endl;
	for(int r = 0; r < height; r++) {
		for(int c = 0; c < width; c++) {
			input_image[r][c] = {uint8_t(r*5 + c), uint8_t(r*5 + c), uint8_t(r*5 + c)};
		}
	}
	std::cout << std::endl;

	for(int r = 0; r < height; r++) {
	    for(int c = 0; c < width; c++) {
	        virtual_ddr_read[r * width + c] = input_image[r][c].r;		// Буфер для чтения
	    }
	}



	// Создаём входной/выходной поток
	video_stream src_stream("src_in");
	video_stream dst_stream("dst_out");


	    // Заполнение входного потока
		std::cout << "--- 2 ЗАПОЛНЕНИЕ ВХОДНОГО ПОТОКА ---" << std::endl;
		for(int r = 0; r < height; r++){
			for(int c = 0; c < width; c++){
				video_pixel p;

				// Пакуем тестовые цвета R, G, B в 24-битную шину data
				p.data.range(23,16) = input_image[r][c].r;
				p.data.range(15,8) = input_image[r][c].g;
				p.data.range(7,0) = input_image[r][c].b;

				// Установка управляющих сигналов видеосигналов
				p.user = (r == 0 && c == 0) ? 1 : 0;
				p.last = (c == width - 1) ? 1 : 0;

				p.keep = 0x7;
				p.strb = 0x7;

				// Записываем пиксель в поток
				src_stream.write(p);

			} // end c
		} // end r

		std::cout << std::endl;

		// Запуск переворота Vertical
		std::cout << "--- 3. ЗАПУСК ПЕРЕВОРОТ ПО ВЕРТИКАЛЕ ---" << std::endl;
		Flip_Vertical(src_stream, dst_stream, virtual_ddr_write, virtual_ddr_read, 1);

		 std::cout << "1. Ожидаемый результат, успешно ли записался кадр в virtual_ddr_write:" << std::endl;
		 	int error_count = 0;
			for(int r = 0; r < height; r++) {
				for(int c = 0; c < width; c++) {

					int expected = input_image[r][c].r;						// Ожидаемое значение
					int actual_val = virtual_ddr_write[r * width + c];		// Актуальное значение

					// Проверка результата
					if(actual_val != expected) {
						error_count++;
						 if(error_count < 10) { // Выведем только первые несколько ошибок
							std::cout << "DDR Write Error at [" << r << "][" << c << "]: "
									  << "Expected " << expected << ", got " << actual_val << std::endl;
						}
					}
				} // end r
			}	// end c

			std::cout << std::endl;

			std::cout << "2. Ожидаемый результат, успешно ли записался кадр в поток из virtual_ddr_read:" << std::endl;
			int error_count_2 = 0;

			for(int r = 0; r < height; r++) {
			    for(int c = 0; c < width; c++) {
			        if(!dst_stream.empty()) {
			            video_pixel out_p = dst_stream.read();

			            // Распаковываем получившиеся данные из стрима обратно
			            int out_r = out_p.data.range(23, 16).to_int();
			            int out_g = out_p.data.range(15, 8).to_int();
			            int out_b = out_p.data.range(7, 0).to_int();

			            // Идеальный эталон: строка инвертирована, колонка прямая
			            int expected_val = input_image[(height - 1) - r][c].r;

			            // Проверяем правильность всех трех каналов
			            bool correct = (out_r == expected_val && out_g == expected_val && out_b == expected_val);
			            if(!correct) {
			                error_count_2++;
			                if(error_count_2 < 10) { // Выведем только первые 10 ошибок
			                    std::cout << "DDR Read Error at [" << r << "][" << c << "]: "
			                              << "Expected " << expected_val << ", got " << out_r << std::endl;
			                }
			            }
			        } else {
			            // Если стрим закончился раньше времени
			            error_count_2++;
			        }
			    }
			}


		// Освобождаем память на ПК
		delete[] virtual_ddr_read;
		delete[] virtual_ddr_write;

		// Итоги теста
		if (error_count == 0 && error_count_2 == 0) {
			std::cout << "!!! TEST PASSED SUCCESSFULLY !!!" << std::endl;
			return 0; // HLS поймет, что все супер
		} else {
			std::cout << "!!! TEST FAILED WITH " << error_count << " ERRORS !!!" << std::endl;
			std::cout << "!!! TEST FAILED WITH " << error_count_2 << " ERRORS !!!" << std::endl;
			return 1; // HLS заблокирует косимуляцию и покажет ошибку
		}

} // end main
























