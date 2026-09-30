// =================================================================
// Project: OV7670 Video Processing System
// File: PolarityReversal.hpp
// Description: Driver for PolarityReversal module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#pragma once

#include "xparameters.h"
#include "xpolarityreversal.h"


// Класс для управления модулем PolarityReversal

class PolarityReversal
{

public:

	// Конструктор
	PolarityReversal(uint16_t deviceID);

	// Деструктор
	~PolarityReversal();

	// Инициализация модуля
	int InitPolarityReversal();

	// Старт модуля
	void StartPolarityReversal();

	// Режим работы модуля (переключение полярности)
	void ModePolarityReversal(uint8_t mode);

private:
	uint16_t DeviceId;							// Индефикатор модуля в xparameters
	XPolarityreversal PolarityReversalInst;		// Экземпляр модуля

};
