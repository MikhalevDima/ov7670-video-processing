// =================================================================
// Project: OV7670 Video Processing System
// File: VDMA.cpp
// Description: Driver for VDMA module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#include "VDMA.hpp"


/* Конструктор */
VDMA::VDMA(uint16_t deviveID)
{
	DeviceId = deviveID;

}

/* Деструктор */
VDMA::~VDMA(){}

/* Инициализация VDMA */
int VDMA::InitVDMA()
{

	// Размечаем наши 16 МБ памяти как Device Memory (кэш полностью отключен)
	Xil_SetTlbAttributes(0x1F000000, DEVICE_MEMORY);
	Xil_SetTlbAttributes(0x1F100000, DEVICE_MEMORY);
	Xil_SetTlbAttributes(0x1F200000, DEVICE_MEMORY);

	XAxiVdma_Config *Config;
	int Status;

	/* Поиск конфигурации*/
	Config = XAxiVdma_LookupConfig(DeviceId);
	if (NULL == Config) {
		return XST_FAILURE;
	}

	/* Инициализация драйвера VDMA */
	Status = XAxiVdma_CfgInitialize(&VDMAinst, Config, Config->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}



	/* Установка HSIZE, STRIDE, VSIZE для S2MM и MM2S каналов */
	DmaSetupWrite.VertSizeInput = IMAGE_HEIGHT;
	DmaSetupWrite.HoriSizeInput = IMAGE_WIDTH*3;		// /8 так как Memory Map Data Width = 64 бит (8 байт) 8 байт за трансфер
	DmaSetupWrite.Stride = STRIDE;
	DmaSetupWrite.FrameDelay = 0;
	DmaSetupWrite.EnableCircularBuf = 1;
	DmaSetupWrite.EnableSync = 1;
	DmaSetupWrite.PointNum = 0;
	DmaSetupWrite.EnableFrameCounter = 0;
	DmaSetupWrite.FixedFrameStoreAddr = 0;

	// Физические адреса для кадров в ddr
	DmaSetupWrite.FrameStoreStartAddr[0] = 0x1F000000; // Буфер 1
	DmaSetupWrite.FrameStoreStartAddr[1] = 0x1F100000; // Буфер 2 (+1 МБ)
	DmaSetupWrite.FrameStoreStartAddr[2] = 0x1F200000; // Буфер 3 (+1 МБ)

	Status = XAxiVdma_DmaConfig(&VDMAinst, WRITE,
	        &DmaSetupWrite);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
	}

	DmaSetupRead.VertSizeInput = IMAGE_HEIGHT;
	DmaSetupRead.HoriSizeInput = IMAGE_WIDTH*3;
	DmaSetupRead.Stride = STRIDE;
	DmaSetupRead.FrameDelay = 0;
	DmaSetupRead.EnableCircularBuf = 1;
	DmaSetupRead.EnableSync = 1;
	DmaSetupRead.PointNum = 0;
	DmaSetupRead.EnableFrameCounter = 0;
	DmaSetupRead.FixedFrameStoreAddr = 0;

	// Каналы чтения и записи должны смотреть в одни и те же адреса
	DmaSetupRead.FrameStoreStartAddr[0]  = 0x1F000000;
	DmaSetupRead.FrameStoreStartAddr[1]  = 0x1F100000;
	DmaSetupRead.FrameStoreStartAddr[2]  = 0x1F200000;

	Status = XAxiVdma_DmaConfig(&VDMAinst, READ,
	        &DmaSetupRead);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
	}

	// Задание базовых адресов для S2MM (запись)
	Status = XAxiVdma_DmaSetBufferAddr(&VDMAinst, WRITE,
			DmaSetupWrite.FrameStoreStartAddr);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
	}

	// Задание базовых адресов для MM2S (чтение)
	Status = XAxiVdma_DmaSetBufferAddr(&VDMAinst, READ,
			DmaSetupRead.FrameStoreStartAddr);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
	}

	// Запуск VDMA запись
	Status = XAxiVdma_StartWriteFrame(&VDMAinst,
			&DmaSetupWrite);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
	}

	// Запуск VDMA чтение
	Status = XAxiVdma_StartReadFrame(&VDMAinst,
			&DmaSetupRead);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
	}
	
	return XST_SUCCESS;
}


















