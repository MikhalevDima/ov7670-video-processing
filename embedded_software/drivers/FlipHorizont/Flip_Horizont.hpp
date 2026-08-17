// =================================================================
// Project: OV7670 Video Processing System
// File: Flip_Horizont.hpp
// Description: Driver for Flip_Horizont module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#pragma once

#include "xparameters.h"
#include "xflip_horizont.h"


// Класс для управления модулем Flip_Horizont

class Flip_Horizont {

public:
	// Конструктор
	Flip_Horizont(uint16_t deviceID);

	// Деструктор
	~Flip_Horizont();

	// Инициализация модуля
	int InitFlipHorizont();

	// Запуск модуля
	void StartFlipHorizont();

	// Режим работы модуля (отзеркалено по горизонтале или нет)
	void SetFlipHorizont(uint8_t mode);

private:

	uint16_t DeviceId;						// Индефикатор модуля в xparameters
	XFlip_horizont FlipHorizontInst;		// Экземпляр модуля


};
