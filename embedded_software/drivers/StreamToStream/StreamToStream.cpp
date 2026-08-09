// =================================================================
// Project: OV7670 Video Processing System
// File: StreamToStream.cpp
// Description: Driver for StreamToStream module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#include "StreamToStrean.hpp"

// Конструктор
StreamToStream:: StreamToStream(uint16_t deviceID)
	: DeviceId(deviceID) {}

// Деструктор
StreamToStream:: ~StreamToStream() {}

// Инициализация модуля
int StreamToStream::InitStreamToStream(){

	int Status;

	// Инициализация модуля
	Status = XRgb_gray_Initialize(&StreamToStreamInst, DeviceId);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	return XST_SUCCESS;
}

// Старт Модуля
void StreamToStream::StartStreamToStream(){

	XRgb_gray_Start(&StreamToStreamInst);
	XRgb_gray_EnableAutoRestart(&StreamToStreamInst);

}
