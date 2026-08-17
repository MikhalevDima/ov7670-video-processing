// =================================================================
// Project: OV7670 Video Processing System
// File: Flip_Horizont.cpp
// Description: Driver for Flip_Horizont module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#include "Flip_Horizont.hpp"

// Конструктор
Flip_Horizont::Flip_Horizont(uint16_t deviceID)
		: DeviceId(deviceID) {};

// Деструктор
Flip_Horizont::~Flip_Horizont(){};

// Инициализация модуля
int Flip_Horizont::InitFlipHorizont(){

	int Status;

	// Инициализация модуля
	Status = XFlip_horizont_Initialize(&FlipHorizontInst, DeviceId);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	return XST_SUCCESS;
}

// Запуск модуля
void Flip_Horizont::StartFlipHorizont(){

	XFlip_horizont_Start(&FlipHorizontInst);
	XFlip_horizont_EnableAutoRestart(&FlipHorizontInst);
}

// Режим работы модуля (отзеркалено по горизонтале или нет)
void Flip_Horizont::SetFlipHorizont(uint8_t mode){

	switch (mode) {
		case 0:
			XFlip_horizont_Set_mode(&FlipHorizontInst, mode);	// Байпасс
			break;
		case 1:
			XFlip_horizont_Set_mode(&FlipHorizontInst, mode);	// Переворот по горизонтале
			break;
		default:
			XFlip_horizont_Set_mode(&FlipHorizontInst, 0);
			break;
	}

}
