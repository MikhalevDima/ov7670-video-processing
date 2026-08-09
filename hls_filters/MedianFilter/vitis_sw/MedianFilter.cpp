// =================================================================
// Project: OV7670 Video Processing System
// File: MedianFilter.cpp
// Description: Driver for MedianFilter module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#include "MedianFilter.hpp"

// Конструктор
MedianFilter:: MedianFilter(uint16_t deviceID)
			: DevaiceID(deviceID) {}

// Деструктор
MedianFilter:: ~MedianFilter() {}

// Инициализация модуля
int MedianFilter::InitMedianFilter(){

	int Status;

	Status = XMedianfilter_Initialize(&InstMedianFilter, DevaiceID);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	return XST_SUCCESS;

}

// Запуск модуля
void MedianFilter::StartMedianFilter(){

	XMedianfilter_Start(&InstMedianFilter);
	XMedianfilter_EnableAutoRestart(&InstMedianFilter);

}

// Установка порогового значения
void MedianFilter::SetThresholdMedianFilter(uint32_t threshold){

	if(threshold > 255) {
	        threshold = 255;
	    }

	XMedianfilter_Set_threshold(&InstMedianFilter, threshold);

}




