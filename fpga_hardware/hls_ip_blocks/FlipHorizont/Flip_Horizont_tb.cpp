#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <iostream>


typedef ap_axiu<24, 1, 1, 1> video_pixel;
typedef hls::stream<video_pixel> video_stream;

// Объявляем нашу функцию верхнего уровня
void Flip_Horizont(video_stream& stream_in,
				  video_stream& stream_out,
				  ap_uint<8> mode);

// Вспомогательная структура для ручной проверки в тестбенче
struct test_rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};


int main(){

	// Создаём входной/выходной поток
	video_stream src_stream;
	video_stream dst_stream;

	// Создадим тестовую картинку 5х5
	const int width = 640;
	const int height = 480;

	test_rgb input_image[height][width];

	// Заполняем тестовую картинку
	    std::cout << "--- 1. ИСХОДНОЕ ИЗОБРАЖЕНИЕ ---" << std::endl;
	    for(int r = 0; r < height; r++) {
	        for(int c = 0; c < width; c++) {
	            input_image[r][c] = {uint8_t(r*5 + c), uint8_t(r*5 + c), uint8_t(r*5 + c)};
	            std::cout << (int)input_image[r][c].r << " ";
	        }
	        std::cout << std::endl;
	    }
	    std::cout << std::endl;


	// Заполнение входного потока
	std::cout << "--- 1. ЗАПОЛНЕНИЕ ВХОДНОГО ПОТОКА ---" << std::endl;
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

			 std::cout << "Вход [" << r << "][" << c << "] = "
			                      << (int)input_image[r][c].r
			                      << " user=" << p.user.to_int()
			                      << " last=" << p.last.to_int() << std::endl;

			// Записываем пиксель в поток
			src_stream.write(p);

		} // end c
	} // end r

	std::cout << std::endl;

	// Запуск переворота Horizont
	std::cout << "--- 2. ЗАПУСК ПЕРЕВОРОТ ПО ГОРИЗРНТАЛЕ ---" << std::endl;
	Flip_Horizont(src_stream, dst_stream, 1);

	 std::cout << "Ожидаемый результат (горизонтальное отражение):" << std::endl;
	    for(int r = 0; r < height; r++) {
	        for(int c = 0; c < width; c++) {
	            // Ожидаемое значение - отраженное по горизонтали
	            int expected = input_image[r][width - 1 - c].r;
	            std::cout << expected << " ";
	        }
	        std::cout << std::endl;
	    }
	    std::cout << std::endl;

	    std::cout << "Полученный результат:" << std::endl;
	    bool test_passed = true;
	    for(int r = 0; r < height; r++) {
	        for(int c = 0; c < width; c++) {
	            if(!dst_stream.empty()) {
	                video_pixel out_p = dst_stream.read();

	                // Распоковываем получившиеся данные обратно
	                int out_r = out_p.data.range(23, 16).to_int();
	                int out_g = out_p.data.range(15, 8).to_int();
	                int out_b = out_p.data.range(7, 0).to_int();
	                int expected = input_image[r][width - 1 - c].r;

	                // Проверяем правильность
	                bool correct = (out_r == expected && out_g == expected && out_b == expected);
	                if(!correct) test_passed = false;

	                std::cout << "Выход [" << r << "][" << c << "] -> "
	                         << "RGB: (" << out_r << ", " << out_g << ", " << out_b << ") "
	                         << "| Ожидалось: " << expected
	                         << " | USER:" << out_p.user.to_int()
	                         << " | LAST:" << out_p.last.to_int()
	                         << (correct ? " ✓" : " ✗")
	                         << std::endl;
	            } else {
	                std::cout << "Выход [" << r << "][" << c << "] -> ПОТОК ПУСТ!" << std::endl;
	                test_passed = false;
	            }
	        }
	    }

	    if(test_passed) {
	        std::cout << "\n>>> TEST PASSED! <<<" << std::endl;
	    } else {
	        std::cout << "\n>>> TEST FAILED! <<<" << std::endl;
	    }

	    return 0;
	}










