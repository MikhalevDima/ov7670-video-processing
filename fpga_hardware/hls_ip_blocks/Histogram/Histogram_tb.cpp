#include <iostream>
#include <vector>
#include <cstdlib>
#include "ap_axi_sdata.h"
#include "hls_stream.h"

// Дублируем константы для теста
int const WIDTH = 640;
int const HEIGHT = 480;
int const HIST = 256;
int const TOTAL_PIXELS = WIDTH * HEIGHT;

// Определяем структуры
typedef ap_axiu<24, 1, 1, 1> video_pixel;
typedef hls::stream<video_pixel> video_stream;

// Прототип нашей HLS-функции
void Histogram(video_stream& stream_in, video_stream& stream_out, unsigned int hist[HIST]);

int main() {
    std::cout << "--- Запуск C-Simulation для гистограммы ---" << std::endl;

    // Стримы для тестирования
    video_stream input_stream("input_stream");
    video_stream output_stream("output_stream");

    // Инициализация массивов для результатов
    unsigned int hls_hist[HIST] = {0};
    unsigned int golden_hist[HIST] = {0}; // Эталонная программная гистограмма

    // 1. Генерация тестового кадра с известным распределением пикселей
    // Сделаем так, чтобы цвета чередовались (как в реальном видео) для проверки RAW-зависимостей
    std::cout << "Генерация тестового кадра..." << std::endl;

    for (int i = 0; i < TOTAL_PIXELS; i++) {
        video_pixel pixel;

        // Генерируем тестовый паттерн: например, пиксели от 0 до 255 циклически
        // Это гарантирует, что каждый цвет встретится одинаковое количество раз
        uint8_t test_color = i % HIST;

        // Записываем цвет в старший байт (23..16), как ожидает наш IP-блок
        pixel.data = (test_color << 16) | 0x0000;

        // Управляем сигналами AXI-Stream:
        // Сигнал user (Start of Frame) должен быть равен 1 строго на самом первом пикселе кадра
        pixel.user = (i == 0) ? 1 : 0;

        // Сигнал last (End of Line) равен 1 на последнем пикселе каждой строки
        pixel.last = ((i + 1) % WIDTH == 0) ? 1 : 0;

        pixel.keep = -1; // Сигналы валидности байт (все 1)
        pixel.strb = -1;

        // Отправляем пиксель в HLS-входной стрим
        input_stream.write(pixel);

        // Параллельно считаем эталонную "золотую" гистограмму на обычном C++
        golden_hist[test_color]++;
    }

    // 2. Вызов нашей HLS-функции (моделирование работы железа)
    std::cout << "Вызов аппаратной функции Histogram..." << std::endl;
    Histogram(input_stream, output_stream, hls_hist);

    // 3. Проверка результатов
    int error_count = 0;

    // Проверка выходного стрима (убедимся, что транзит Pass-Through работает и пиксели не потерялись)
    if (output_stream.size() != TOTAL_PIXELS) {
        std::cout << "ОШИБКА: Выходной стрим содержит " << output_stream.size()
                  << " пикселей вместо " << TOTAL_PIXELS << "!" << std::endl;
        error_count++;
    }

    // Попочередное сравнение гистограмм
    std::cout << "Сравнение результатов с эталоном..." << std::endl;
    for (int i = 0; i < HIST; i++) {
        if (hls_hist[i] != golden_hist[i]) {
            std::cout << "Несовпадение для цвета [" << i << "]: "
                      << "HLS = " << hls_hist[i] << ", "
                      << "Эталон = " << golden_hist[i] << std::endl;
            error_count++;
        }
    }

    // 4. Итог теста
    if (error_count == 0) {
        std::cout << "--- ТЕСТ УСПЕШНО ПРОЙДЕН! ---" << std::endl;
        return 0; // Vitis HLS считает 0 признаком успешного теста
    } else {
        std::cout << "--- ТЕСТ ПРОВАЛЕН! Обнаружено ошибок: " << error_count << " ---" << std::endl;
        return 1; // Любое ненулевое значение — признак ошибки
    }
}
