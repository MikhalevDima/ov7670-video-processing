// =================================================================
// Project: OV7670 Video Processing System
// File: GaussFilter.cpp
// Description: Driver for GaussFilter module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#include "GaussFilter.hpp"

// Конструктор
GaussFilter:: GaussFilter(uint16_t deviceID)
	: DeviceId(deviceID) {}

// Деструктор
GaussFilter:: ~GaussFilter() {}

// Инициализация модуля
int GaussFilter::InitGaussFilter(){

	int Status;

	// Инициализация модуля
	Status = XGaussfilter_Initialize(&GaussFilterInst, DeviceId);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	return XST_SUCCESS;
}

// Старт Модуля
void GaussFilter::StartGaussFilter(){

	XGaussfilter_Start(&GaussFilterInst);
	XGaussfilter_EnableAutoRestart(&GaussFilterInst);

}

// Установить яркость от 0 до 255
void GaussFilter::Brightness(uint32_t brightness){

	XGaussfilter_Set_brightness(&GaussFilterInst, brightness);

}
