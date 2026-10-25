#include <iostream>
#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include "ap_fixed.h"

// Объявляем константы (должны совпадать с основным файлом)
int const WIDTH = 640;
int const HEIGHT = 480;
int const TOTAL_PIXELS = WIDTH * HEIGHT;

typedef ap_axiu<24, 1, 1, 1> video_pixel;
typedef hls::stream<video_pixel> video_stream;
typedef ap_ufixed<16, 4, AP_RND, AP_SAT> gain_t;

// Прототип тестируемой функции
void AGC(video_stream& stream_in, video_stream& stream_out, gain_t gain);

int main() {
    video_stream src_stream;
    video_stream dst_stream;

    // --- ТЕСТ 1: Увеличение яркости (Gain = 1.5) ---
    // На ARM мы бы сделали: (uint32_t)(1.5f * 4096.0f) = 6144
    // В тестбенче hls задаем напрямую float, ap_fixed сам сконвертируется
    gain_t test_gain_up = 1.5;

    std::cout << ">> Starting Test 1 (Gain = 1.5)..." << std::endl;

    // Заполняем входной стрим тестовым кадром (например, серый цвет 100)
    for (int i = 0; i < TOTAL_PIXELS; i++) {
        video_pixel pixel;
        ap_uint<8> luma = 100; // Исходная яркость

        pixel.data.range(23, 16) = luma;
        pixel.data.range(15, 8)  = 0; // Входные каналы могут быть любыми
        pixel.data.range(7, 0)   = 0;

        // Служебные сигналы AXI-Stream
        pixel.user = (i == 0) ? 1 : 0;          // Start of Frame
        pixel.last = (i == TOTAL_PIXELS - 1) ? 1 : 0; // End of Frame
        pixel.keep = 0x7;
        pixel.strb = 0x7;

        src_stream.write(pixel);
    }

    // Запускаем наш IP-блок
    AGC(src_stream, dst_stream, test_gain_up);

    // Проверяем результаты ТЕСТА 1
    bool test1_passed = true;
    for (int i = 0; i < TOTAL_PIXELS; i++) {
        video_pixel out_pixel;
        dst_stream.read(out_pixel);

        ap_uint<8> res_luma = out_pixel.data.range(23, 16);

        // Ожидаем: 100 * 1.5 = 150
        if (i == 0 || i == TOTAL_PIXELS - 1) { // Выведем первый и последний пиксель для контроля
            std::cout << "Pixel " << i << ": In=100, Gain=1.5 -> Out=" << (int)res_luma << std::endl;
        }
        if (res_luma != 150) {
            test1_passed = false;
        }
    }

    // --- ТЕСТ 2: Проверка ограничения сверху (Saturation) (Gain = 3.0) ---
    // Исходная яркость 100 * 3.0 = 300. Должно жестко ограничиться до 255!
    gain_t test_gain_sat = 3.0;
    std::cout << "\n>> Starting Test 2 (Saturation Check with Gain = 3.0)..." << std::endl;

    for (int i = 0; i < TOTAL_PIXELS; i++) {
        video_pixel pixel;
        pixel.data.range(23, 16) = 100;
        pixel.user = (i == 0) ? 1 : 0;
        pixel.last = (i == TOTAL_PIXELS - 1) ? 1 : 0;
        pixel.keep = 0x7;
        pixel.strb = 0x7;
        src_stream.write(pixel);
    }

    AGC(src_stream, dst_stream, test_gain_sat);

    bool test2_passed = true;
    for (int i = 0; i < TOTAL_PIXELS; i++) {
        video_pixel out_pixel;
        dst_stream.read(out_pixel);
        ap_uint<8> res_luma = out_pixel.data.range(23, 16);

        if (i == 0) {
            std::cout << "Pixel 0: In=100, Gain=3.0 -> Out=" << (int)res_luma << " (Expected 255 due to AP_SAT)" << std::endl;
        }
        if (res_luma != 255) {
            test2_passed = false;
        }
    }

    // Итоги тестирования
    std::cout << "\n==========================" << std::endl;
    if (test1_passed && test2_passed) {
        std::cout << "  ALL TESTS PASSED SUCCESSFULLY! " << std::endl;
        std::cout << "==========================" << std::endl;
        return 0; // Идеально, тестбенч сообщает Vitis HLS, что всё ок
    } else {
        std::cerr << "  TEST FAILED! Check logic." << std::endl;
        std::cout << "==========================" << std::endl;
        return 1; // Ошибка
    }
}
