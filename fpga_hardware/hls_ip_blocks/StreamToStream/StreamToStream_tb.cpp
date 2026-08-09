#include <iostream>
#include <ap_axi_sdata.h>
#include <hls_stream.h>

// Определяем структуру пикселя
typedef ap_axiu<24, 1, 1, 1> video_pixel;
typedef hls::stream<video_pixel> video_stream;

// Объявление функции верхнего уровня
void StreamToStream(video_stream& stream_in, video_stream& stream_out);

int main(){

	video_stream src_stream;
	video_stream dst_stream;

	const int width = 4;
	const int height = 2;

	// 1. Имитация генерации кадров (Запись во входной поток)
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			video_pixel p;
			p.data = (y << 8) | x; // Условный цвет пикселя

			// Установка флага TUSER на первом пикселе кадра
			p.user = (x == 0 && y == 0) ? 1 : 0;

			// Установка флага TLAST на последнем пикселе каждой строки
			p.last = (x == width - 1) ? 1 : 0;

			src_stream.write(p);
		}
	}

	 // 2. Запуск обрабатывающей HLS функции в цикле для каждого пикселя
	    for (int i = 0; i < (width * height); i++) {
	    	StreamToStream(src_stream, dst_stream);
	    }

	    // 3. Проверка выходного потока
	    if (dst_stream.size() != (width * height)) {
	        std::cerr << "Ошибка: Количество пикселей на выходе не совпадает!" << std::endl;
	        return 1;
	    }

	    std::cout << "Проверка пикселей:" << std::endl;
	    for (int y = 0; y < height; y++) {
	        for (int x = 0; x < width; x++) {
	            video_pixel out_p = dst_stream.read();
	            std::cout << "Пиксель [" << x << "][" << y << "] - Данные: " << out_p.data.to_int()
	                      << ", USER: " << out_p.user.to_int()
	                      << ", LAST: " << out_p.last.to_int() << std::endl;
	        }
	    }

	    std::cout << "Тест успешно пройден!" << std::endl;
	    return 0;
}
