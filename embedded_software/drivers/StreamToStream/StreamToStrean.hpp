// =================================================================
// Project: OV7670 Video Processing System
// File: StreamToStream.hpp
// Description: Driver for StreamToStream module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#pragma once

#include "xrgb_gray.h"
#include "xparameters.h"


class StreamToStream {

public:
	// Конструктор
	StreamToStream(uint16_t deviceID);

	// Деструктор
	~StreamToStream();

	// Инициализация модуля
	int InitStreamToStream();

	// Запуск модуля
	void StartStreamToStream();

private:
	uint16_t DeviceId;				// Индефикатор модуля в xparameters
	XRgb_gray StreamToStreamInst;	// Экземпляр модуля
};
