// =================================================================
// Project: OV7670 Video Processing System
// File: PolarityReversal.cpp
// Description: Driver for PolarityReversal module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#include "PolarityReversal.hpp"

// Реализация функций в классе PolarityReversal

// Крнструктор
PolarityReversal::PolarityReversal(uint16_t deviceID)
				: DeviceId(deviceID) {};

// Деструктор
PolarityReversal::~PolarityReversal() {};

// Инициализация модуля
int PolarityReversal::InitPolarityReversal(){

	int Status;

	// Инициализация модуля
	Status = XPolarityreversal_Initialize(&PolarityReversalInst, DeviceId);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	return XST_SUCCESS;
}

// Старт модуля
void PolarityReversal::StartPolarityReversal()
{
	XPolarityreversal_Start(&PolarityReversalInst);
	XPolarityreversal_EnableAutoRestart(&PolarityReversalInst);
}

// Режим работы модуля (переключение полярности)
void PolarityReversal::ModePolarityReversal(uint8_t mode)
{
	switch (mode) {
		case 0:
			XPolarityreversal_Set_mode(&PolarityReversalInst, 0);		// Режим байпасс
			break;
		case 1:
			XPolarityreversal_Set_mode(&PolarityReversalInst, 1);		// Смена поляоности изображения
			break;
		default:
			XPolarityreversal_Set_mode(&PolarityReversalInst, 0);
			break;
	}
}





