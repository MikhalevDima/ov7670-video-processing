// =================================================================
// Project: OV7670 Video Processing System
// File: GuideFilter.hpp
// Description: Driver for GuideFilter module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#pragma once

#include "xparameters.h"
#include "xguidefilter.h"


class GuideFilter {

public:
	// Конструктор
	GuideFilter(uint16_t deviceID);

	// Деструктор
	~GuideFilter();

	// Инициализация модуля
	int IntGuideFilter();

	// Старт модуля
	void StartGuideFilter();

	// Установка ESP
	void SetESPGuideFilter(int ESP);

private:
	uint16_t DeviceID;					// ID устройства
	XGuidefilter GuideFilterInst;		// Экземпляр модуля

};
