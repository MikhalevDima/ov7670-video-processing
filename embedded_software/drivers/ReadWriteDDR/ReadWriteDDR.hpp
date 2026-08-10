// =================================================================
// Project: OV7670 Video Processing System
// File: ReadWriteDDR.hpp
// Description: Driver for ReadWriteDDR module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#pragma once

#include "xparameters.h"
#include "xreadwriteddr.h"
#include "xil_cache.h"
#include "xil_mmu.h"
#include "sleep.h"

#include "xscugic.h"	// Контроллер прерывания

class ReadWriteDDR {


public:


	// Конструктор
	ReadWriteDDR(uint16_t device_id, XScuGic &gicInstancePtr);

	// Деструктор
	~ReadWriteDDR();

	// Инициализвция модуля
	int InitReadWriteDDR();

	// Старт модуля
	void ProcessSingleFrame();

	// Старт модуля в режиме прерывания
	void ProcessSingleFrameInterrupt();

	// Метод для переключения кадров в режиме прерывания, размещается в main -> while
	void ProcessSingleFrameInterruptWhile();

	// Настройка режима прерывания
	int InitInterrupt(uint16_t InterruptID);

	// Чтение статистики min, max, avg
	void StatRead();



private:
	uint16_t DeviceID;					// ID устройства
	XReadwriteddr ReadWriteInst;		// Экземпляр модуля
	int buffer_toggle; 					// Флаг переключения

	XScuGic &GicInstancePtr;			// Ссылка на контроллео GIC
	uint16_t InterruptID;				// ID прерывания
	volatile uint32_t hls_frame_ready = 0;		// Флаг прерывания

	inline static const uint32_t HLS_FRAME_READ = 0x1ED00000;	// Свободный буфер для чтения (1 МБ)
	inline static const uint32_t HLS_FRAME_WRITE = 0x1EE00000;	// Свободный буфер для записи (1 МБ)
	inline static const uint32_t HLS_STAT_BUFFER = 0x1EF00000;	// Буфер для статистики (1 МБ)

	static void HandlerInterruptReadWriteDDR(void *CallBackRef);		// Обработчик прерывания

};
