#include <ap_axi_sdata.h>
#include <hls_stream.h>


const int MAX_WIDTH = 640;
const int MAX_HEIGHT = 480;


typedef ap_axiu<24, 1, 1, 1> video_pixel;
typedef hls::stream<video_pixel> video_stream;

// Структура для хранения RGB
struct rgb_data {
	ap_uint<8> r;
	ap_uint<8> g;
	ap_uint<8> b;
};

void GaussFilter(video_stream& stream_in, video_stream& stream_out, int brightness){

	#pragma HLS INTERFACE axis port=stream_in
	#pragma HLS INTERFACE axis port=stream_out
	#pragma HLS INTERFACE s_axilite port=brightness bundle=CTRL_BUS
	#pragma HLS INTERFACE s_axilite port=return bundle=CTRL_BUS

	// Строчный буфер три строки
	rgb_data line_buf[3][MAX_WIDTH];
	#pragma HLS ARRAY_PARTITION variable=line_buf complete dim = 1

	// Буфер для окна
	rgb_data window[3][3];
	#pragma HLS ARRAY_PARTITION variable=window complete dim = 0

	// Линия задержки для USER и LAST (длина: ширина кадра + 2 пикселя)
	ap_uint<1> delay_user[MAX_WIDTH + 2];
	ap_uint<1> delay_last[MAX_WIDTH + 2];
	#pragma HLS ARRAY_PARTITION variable=delay_user complete
    #pragma HLS ARRAY_PARTITION variable=delay_last complete

	L_row: for(int y = 0; y < MAX_HEIGHT; y ++){
		L_col: for(int x = 0; x < MAX_WIDTH; x ++){

		#pragma HLS PIPELINE II=1

		video_pixel pixel_in;
		stream_in.read(pixel_in);

		// Выдёргиваем цвета
		rgb_data in_pixel;
		in_pixel.r = pixel_in.data.range(23, 16);
		in_pixel.g = pixel_in.data.range(15, 8);
		in_pixel.b = pixel_in.data.range(7, 0);

		// Сдвиг строчного буфера
		rgb_data r0 = line_buf[0][x];
		rgb_data r1 = line_buf[1][x];

		line_buf[0][x] = r1;
		line_buf[1][x] = line_buf[2][x];
		line_buf[2][x] = in_pixel;


		// Сдвиг окна влево
		for (int r = 0; r < 3; r++) {
			window[r][0] = window[r][1];
			window[r][1] = window[r][2];
		}

		// Заполняем правый новый столбец окна
		window[0][2] = r0;
		window[1][2] = r1;
		window[2][2] = in_pixel;

		//Сдвигаем все триггеры задержки влево (в железе это произойдет параллельно за 1 такт)
		for (int i = 0; i < MAX_WIDTH + 1; i++) {
		#pragma HLS UNROLL // Подсказка компилятору развернуть этот мелкий цикл в параллельные провода
		delay_user[i] = delay_user[i + 1];
		delay_last[i] = delay_last[i + 1];
		}
		// Записываем свежие флаги с текущего входа в самую последнюю ячейку
		delay_user[MAX_WIDTH + 1] = pixel_in.user;
		delay_last[MAX_WIDTH + 1] = pixel_in.last;


		// Фильтр Гауссса
		int sum_r = 0, sum_g = 0, sum_b = 0;

		// Матрица коэффициентов Гаусса
		const int coeff[3][3] = {
			{1, 2, 1},
			{2, 4, 2},
			{1, 2, 1}
		};

		for (int r = 0; r < 3; r++) {
			for (int c = 0; c < 3; c++) {
				sum_r += window[r][c].r * coeff[r][c];
				sum_g += window[r][c].g * coeff[r][c];
				sum_b += window[r][c].b * coeff[r][c];
			}
		}

		// Это предотвратит потерю данных при сложении, если значение уйдет выше 255
		int res_r = (sum_r >> 4) + brightness;
		int res_g = (sum_g >> 4) + brightness;
		int res_b = (sum_b >> 4) + brightness;

		// Делаем правильную проверку верхнего и НИЖНЕГО предела (на случай отрицательного brightness)
		if (res_r > 255) res_r = 255; else if (res_r < 0) res_r = 0;
		if (res_g > 255) res_g = 255; else if (res_g < 0) res_g = 0;
		if (res_b > 255) res_b = 255; else if (res_b < 0) res_b = 0;

		// Безопасно записываем проверенные значения в структуру rgb_data
		rgb_data out_pixel;
		out_pixel.r = res_r;
		out_pixel.g = res_g;
		out_pixel.b = res_b;


		// Обрезка мусора на краях кадра
		if (y < 2 || x < 2 || y == MAX_HEIGHT - 1 || x == MAX_WIDTH - 1) {
			out_pixel = in_pixel; // Присваиваем всю структуру целиком!
		}

		// Сборка выходного пикселя
		video_pixel pixel_out;
		pixel_out.data.range(23, 16) = out_pixel.r;
		pixel_out.data.range(15, 8)  = out_pixel.g;
		pixel_out.data.range(7, 0)   = out_pixel.b;

		pixel_out.user = delay_user[0];
		pixel_out.last = delay_last[0];

		pixel_out.keep = pixel_in.keep;
		pixel_out.strb = pixel_in.strb;

		stream_out.write(pixel_out);

	}
}


}




















