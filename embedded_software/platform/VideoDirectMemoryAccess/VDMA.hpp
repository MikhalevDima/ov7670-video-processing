// =================================================================
// Project: OV7670 Video Processing System
// File: VDMA.hpp
// Description: Driver for VDMA module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#ifndef VDMA_H
#define VDMA_H

#include "xaxivdma.h"
#include "xaxivdma_hw.h"
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_mmu.h"
#include "xil_cache.h"
#include "sleep.h"

const uint32_t IMAGE_WIDTH = 640;							// Ширина кадра
const uint32_t IMAGE_HEIGHT = 480;							// Высота кадра
const uint32_t STRIDE = IMAGE_WIDTH*3;						// Количество байт, сколько занимает строка
const uint32_t NUM_BUFFERS = 3;								// Количество буферов
const uint32_t FRAME_SIZE = IMAGE_WIDTH*IMAGE_HEIGHT*3;		// Размер одного кадра


/* Объявление кдасса VDMA */
class VDMA {

	/* Direction */
	enum {
		WRITE = XAXIVDMA_WRITE,
		READ = XAXIVDMA_READ
	};

public:
	VDMA(uint16_t deviceID);					// Конструктор
	~VDMA();									// Деструктор

	int InitVDMA();								// Инициализация VDMA


private:
	uint16_t DeviceId;
	XAxiVdma VDMAinst;
	XAxiVdma_DmaSetup DmaSetupRead;
	XAxiVdma_DmaSetup DmaSetupWrite;

	/* Выделение памяти под три кадра, выровненной по границе 32 байта
	 * размещаем буфер в отдельной памяти ddr в данном случае не используем */
	//uint8_t FrameBuffer[FRAME_SIZE*NUM_BUFFERS]__attribute__((aligned(32)));

};

#endif // VDMA_H
