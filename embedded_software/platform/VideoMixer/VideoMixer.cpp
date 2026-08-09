// =================================================================
// Project: OV7670 Video Processing System
// File: VideoMixer.cpp
// Description: Driver for VideoMixer module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#include "VideoMixer.hpp"

//Конструктор
VideoMixer::VideoMixer(uint16_t deviceID)
{
	DeviceID = deviceID;
}

//Деструктор
VideoMixer::~VideoMixer(){};

//Инициализация модуля
int VideoMixer::IntVideoMixer()
{
	int Status;

	Status = XVMix_Initialize(&VideoMix, DeviceID);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}

	/* Установка слоя 0 (Master) */

	XVidC_VideoStream VidStrmIn; // Для выходного потока (1080p)			//Создаём структуру с параметрами входного потока

	//Заполняем структуру
	Status = XVidC_SetVideoStream(&VidStrmIn,
				XVIDC_VM_1920x1080_60_P,
				XVIDC_CSF_RGB, 						// Формат цвета (напр. RGB, YUV444)
				XVIDC_BPC_8, 						// Глубина цвета (8, 10, 12 бит)
				XVIDC_PPC_1);						// Пикселей за такт (согласно настройкам в Vivado)
	if (Status != XST_SUCCESS) {
				return XST_FAILURE;
	}


	XVMix_SetVidStream(&VideoMix, &VidStrmIn);

	Status = XVMix_LayerEnable(&VideoMix, XVMIX_LAYER_MASTER);					// Start master layer
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}

	return XST_SUCCESS;
}

//Установка первого слоя
int VideoMixer::SetLayer1_640x480()
{
	int Status;
	uint32_t Stride = 640*3;												//Количество пикселей на то сколько байт уходит на один пиксель

	XVidC_VideoWindow Layer1 = {480, 270, 640, 480};						//X, Y, Width, Height
	XVMix_SetLayerWindow(&VideoMix, XVMIX_LAYER_1, &Layer1, Stride);

	Status = XVMix_SetLayerAlpha(&VideoMix, XVMIX_LAYER_1, 255);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}

	Status = XVMix_LayerEnable(&VideoMix, XVMIX_LAYER_1);					// Start master layer
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}

	return XST_SUCCESS;
}


//Старт миксера
void VideoMixer::StartMixer()
{
	XVMix_Start(&VideoMix);

}

//Стоп миксер
void VideoMixer::StopMixer()
{
	XVMix_Stop(&VideoMix);
}














