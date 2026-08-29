// =================================================================
// Project: OV7670 Video Processing System
// File: Flip_Vertical.hpp
// Description: Driver for Flip_Vertical module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#pragma once

#include "xparameters.h"
#include "xflip_vertical.h"
#include "xscugic.h"			// Контроллер прерывания
#include "xil_cache.h"

// Класс для управления модулем Flip_Vertical

class Flip_Vertical {

public:
	// Конструктор
	Flip_Vertical(uint16_t deviceID, XScuGic &gicInstancePtr);

	// Деструктор
	~Flip_Vertical();

	// Инициализация модуля
	int InitFlipVertical();

	// Старт модуля
	void StartFlipVertical();

	// Стоп модуля
	void StopFlipVertical();

	// Режим работы модуля (отражение верх - низ)
	void SetModeFlipVertical(uint8_t mode);

	// Старт модуля в режиме прерывания
	void ProcessSingleFrameInterrupt();

	// Метод для переключения кадров в режиме прерывания, размещается в main -> while
	void ProcessSingleFrameInterruptWhile();

	// Настройка режима прерывания
	int InitInterrupt(uint16_t InterruptID);

private:

	uint16_t DeviceId;													// Индентификатор модуля в xparameters
	XFlip_vertical	FlipVerticalInst;									// Экземпляр модуля


	volatile uint32_t hls_frame_ready = 0;								// Флаг прерывания
	int buffer_toggle; 													// Флаг переключения
	inline static const uint32_t HLS_FRAME_READ = 0x1ED00000;			// Свободный буфер для чтения (1 МБ)
	inline static const uint32_t HLS_FRAME_WRITE = 0x1EE00000;			// Свободный буфер для записи (1 МБ)

	XScuGic &GicInstancePtr;											// Ссылка на контроллео GIC
	uint16_t InterruptID;												// ID прерывания
	static void HandlerInterruptFlipVertical(void *CallBackRef);		// Обработчик прерывания

};
