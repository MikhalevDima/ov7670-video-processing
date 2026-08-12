// =================================================================
// Project: OV7670 Video Processing System
// File: AdvancedDDE.hpp
// Description: Driver for AdvancedDDE module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#pragma once

#include "xparameters.h"
#include "xadvanceddde.h"
#include "math.h"

class AdvancedDDE {

public:

	// Конструктор
	AdvancedDDE(uint16_t);

	// Деструктор
	~AdvancedDDE();

	// Инициализация модуля
	int IntAdvancedDDE();

	// Запуск модуля
	void StartAdvancedDDE();

	// Обновление DDE таблиц
	void SetDDETable(float gamma, float k_max, float threshold);

private:
	uint16_t BaseAddr;
	XAdvanceddde AdvancedDDEInst;
};
