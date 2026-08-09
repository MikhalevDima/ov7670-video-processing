#include <ap_axi_sdata.h>
#include <hls_stream.h>

// Определяем структуру пикселя
typedef ap_axiu<24, 1, 1, 1> video_pixel;
typedef hls::stream<video_pixel> video_stream;

void StreamToStream(video_stream& stream_in, video_stream& stream_out){

	// Настройка интерфейсов AXI
#pragma HLS INTERFACE axis port=stream_in
#pragma HLS INTERFACE axis port=stream_out
#pragma HLS INTERFACE s_axilite port=return bundle=CTRL_BUS

#pragma HLS PIPELINE II=1

	video_pixel pixel;

	// Чтение пикселя
	stream_in.read(pixel);

	// Запись пикселя
	stream_out.write(pixel);
}


