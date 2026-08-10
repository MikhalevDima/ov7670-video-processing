#include <iostream>
#include <ap_axi_sdata.h>
#include <hls_stream.h>

typedef ap_axiu<24, 1, 1, 1> video_pixel;
typedef hls::stream<video_pixel> video_stream;

// Объявляем нашу функцию верхнего уровня
void GaussFilter(video_stream& stream_in, video_stream& stream_out);

// Вспомогательная структура для ручной проверки в тестбенче
struct test_rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

int main() {
    video_stream src_stream;
    video_stream dst_stream;

    const int width = 4;
    const int height = 4;

    // Создаем тестовую цветную картинку 4х4
    // В центре сделаем яркий красный квадрат, а вокруг — синий фон
    test_rgb input_image[4][4] = {
        {{0,0,200}, {0,0,200}, {0,0,200}, {0,0,200}},
        {{0,0,200}, {250,0,0}, {250,0,0}, {0,0,200}},
        {{0,0,200}, {250,0,0}, {250,0,0}, {0,0,200}},
        {{0,0,200}, {0,0,200}, {0,0,200}, {0,0,200}}
    };

    std::cout << "--- 1. ЗАПОЛНЕНИЕ ВХОДНОГО ПОТОКА ---" << std::endl;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            video_pixel p;

            // Пакуем тестовые цвета R, G, B в 24-битную шину data
            p.data.range(23, 16) = input_image[y][x].r;
            p.data.range(15, 8)  = input_image[y][x].g;
            p.data.range(7, 0)   = input_image[y][x].b;

            // Классическая установка видеосигналов
            p.user = (x == 0 && y == 0) ? 1 : 0;
            p.last = (x == width - 1) ? 1 : 0;

            p.keep = 0xF;
            p.strb = 0xF;

            src_stream.write(p);
        }
    }

    std::cout << "--- 2. ЗАПУСК ФИЛЬТРА ГАУССА (16 ТАКТОВ) ---" << std::endl;
    for (int i = 0; i < (width * height); i++) {
    	GaussFilter(src_stream, dst_stream);
    }

    std::cout << "--- 3. ПРОВЕРКА ВЫХОДНОГО ВИДЕОПОТОКА ---" << std::endl;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            video_pixel out_p = dst_stream.read();

            // Распаковываем получившиеся цвета обратно
            int out_r = out_p.data.range(23, 16).to_int();
            int out_g = out_p.data.range(15, 8).to_int();
            int out_b = out_p.data.range(7, 0).to_int();

            // Выводим результат в консоль для анализа
            std::cout << "Выход [" << x << "][" << y << "] -> "
                      << "RGB: (" << out_r << ", " << out_g << ", " << out_b << ") | "
                      << "USER: " << out_p.user.to_int() << " | "
                      << "LAST: " << out_p.last.to_int() << std::endl;
        }
    }

    std::cout << "\n>>> C-Simulation успешно завершена! <<<" << std::endl;
    return 0;
}
