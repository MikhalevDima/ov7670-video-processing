// =================================================================
// Project: OV7670 Video Processing System
// File: GaussFilter.hpp
// Description: Driver for GaussFilter module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#pragma once

#include "xgaussfilter.h"
#include "xparameters.h"


class GaussFilter {

public:
	// Конструктор
	GaussFilter(uint16_t deviceID);

	// Деструктор
	~GaussFilter();

	// Инициализация модуля
	int InitGaussFilter();

	// Запуск модуля
	void StartGaussFilter();

	// Установить яркость от 0 до 255
	void Brightness(uint32_t brightness);

private:
	uint16_t DeviceId;			// Индефикатор модуля в xparameters
	XGaussefilter GaussFilterInst;	// Экземпляр модуля
};
