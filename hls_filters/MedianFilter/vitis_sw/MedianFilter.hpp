// =================================================================
// Project: OV7670 Video Processing System
// File: MedianFilter.hpp
// Description: Driver for MedianFilter module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#pragma once

#include "xparameters.h"
#include "xmedianfilter.h"

class MedianFilter {

public:
	// Конструктор
	MedianFilter(uint16_t deviceID);

	// Деструктор
	~MedianFilter();

	// Инициализация модуля
	int InitMedianFilter();

	// Запуск модуля
	void StartMedianFilter();

	// Установка порогового значения
	void SetThresholdMedianFilter(uint32_t threshold);

private:
	uint16_t DevaiceID;
	XMedianfilter InstMedianFilter;
};
