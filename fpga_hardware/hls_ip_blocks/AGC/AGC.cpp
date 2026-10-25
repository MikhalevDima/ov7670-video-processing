#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include "ap_fixed.h"

int const WIDTH = 640;
int const HEIGHT = 480;
int const HIST = 256;
int const TOTAL_PIXELS = WIDTH * HEIGHT; // 307200 пикселей

typedef ap_axiu<24, 1, 1, 1> video_pixel;
typedef hls::stream<video_pixel> video_stream;

// 16 бит всего, 4 бита целая часть.
// AP_RND - округление к ближайшему, AP_SAT - насыщение при переполнении
typedef ap_ufixed<16, 4, AP_RND, AP_SAT> gain_t;

// AGC
void AGC(video_stream& stream_in, video_stream& stream_out, gain_t gain)
{

    // Настраиваем порты видео потока
    #pragma HLS INTERFACE axis port=stream_in
    #pragma HLS INTERFACE axis port=stream_out

    // Регистр для ARM. Он будет обновляться "на лету" без остановки конвейера
    #pragma HLS INTERFACE s_axilite port=gain bundle=CTRL_BUS
	#pragma HLS INTERFACE mode=s_axilite port=return bundle=CTRL_BUS

    // Внутренний тип для безопасного умножения и автоматического насыщения до 8 бит
    // 8 бит всего, 8 бит целая часть (дробной нет, нам нужен чистый пиксель)
	typedef ap_ufixed<8, 8, AP_RND, AP_SAT> sat_pixel_t;


    video_pixel pixel_in;
    video_pixel pixel_out;
    static gain_t local_gain = 0;

	// Основной цикл
	video_loop: for (int i = 0; i < TOTAL_PIXELS; i++)
    {
        #pragma HLS PIPELINE II=1

        // Читаем пиксель

		stream_in.read(pixel_in);
		if(pixel_in.user == 1) {
			local_gain = gain;
		}

        // Вытаскиваем яркость (серая компонента из верхнего байта)
        ap_uint<8> raw_luma = pixel_in.data.range(23, 16);

        // ПРИМЕНЯЕМ УСИЛЕНИЕ:
        // Автоматически: (uint8 * fixed16_4) -> результат приводится к sat_pixel_t (8 бит целых).
        // Если результат > 255, благодаря AP_SAT он жестко станет 255 (без переполнения в 0).
        sat_pixel_t multiplied = raw_luma * local_gain;

        // А теперь приводим к обычному целому числу.
        // HLS сам увидит, что у final_luma 0 бит дробной части, и сам сдвинет результат вправо на 12 бит!

        ap_uint<8> final_luma = (ap_uint<8>)multiplied;

        // Дублируем серый цвет во все 3 канала (RGB)
        pixel_out.data.range(23, 16) = final_luma;
        pixel_out.data.range(15, 8)  = final_luma;
        pixel_out.data.range(7, 0)   = final_luma;

        // Копируем служебные сигналы AXI-Stream (TLAST, TUSER, TKEEP, TSTRB)
        pixel_out.keep = pixel_in.keep;
        pixel_out.strb = pixel_in.strb;
        pixel_out.user = pixel_in.user;
        pixel_out.last = pixel_in.last;

        // Отправляем пиксель дальше по конвейеру
        stream_out.write(pixel_out);

	} // end video_loop

} // end AGC























