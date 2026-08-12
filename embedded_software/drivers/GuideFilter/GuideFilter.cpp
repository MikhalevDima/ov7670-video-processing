// =================================================================
// Project: OV7670 Video Processing System
// File: GuideFilter.cpp
// Description: Driver for GuideFilter module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#include "GuideFilter.hpp"

// Конструктор
GuideFilter :: GuideFilter(uint16_t deviceID)
				:DeviceID(deviceID) {}

// Деструктор
GuideFilter:: ~GuideFilter() {};

// Инициализация модуля
int GuideFilter:: IntGuideFilter(){

	int Status;
	Status = XGuidefilter_Initialize(&GuideFilterInst, DeviceID);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	return XST_SUCCESS;
}

// Запуск модуля
void GuideFilter::StartGuideFilter(){

	XGuidefilter_Start(&GuideFilterInst);
	XGuidefilter_EnableAutoRestart(&GuideFilterInst);

}

// Установка ESP
void GuideFilter::SetESPGuideFilter(int ESP){


	XGuidefilter_Set_EPS(&GuideFilterInst, ESP);

}






