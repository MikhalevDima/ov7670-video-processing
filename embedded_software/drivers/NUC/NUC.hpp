// =================================================================
// Project: OV7670 Video Processing System
// File: NUC.hpp
// Description: Driver for NUC module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#pragma once

#include "xnuc.h"
#include "xparameters.h"
#include "xil_cache.h"

#define WIDTH 640
#define HEIGHT 480
#define FRAME_SIZE WIDTH*HEIGHT

class NUC
{
public:

	// Конструктор
	NUC(uint16_t deviceID);

	// Деструктор
	~NUC();

	// Инициализация модуля
	int InitNUC();

	// Старт модуля
	void StartNUC();

	// Режим работы модуля

	// 1. Байпас (без КГШ)
	void BypassNUC();
	// 2. Счёт КГШ
	void CalculateNUC();
	// 3. Применение КГШ
	void ApplicationNUC();

	// Запуск КГШ
	void RunNUC(uint16_t frame);



private:

	uint16_t DeviceID;										// Идентификатор устройства
	XNuc NUCInst;											// Экземпляр модуля

	inline static const uint32_t HLS_FRAME = 0x1ED00000;	// Свободный буфер для записи (1 МБ)


};

