// =================================================================
// Project: OV7670 Video Processing System
// File: SimpleDDE.cpp
// Description: Driver for SimpleDDE module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#include "SimpleDDE.hpp"

// Реализация класса

// Конструктор
SimpleDDE:: SimpleDDE(uint16_t baseaddr)
				: BaseAddr(baseaddr) {}

// Деструктор
SimpleDDE:: ~SimpleDDE(){}

// Инициализация модуля
int SimpleDDE::InitSimpleDDE(){

	int Status;

	Status = XSimpledde_Initialize(&SimpleDDE_Inst, BaseAddr);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	return XST_SUCCESS;

}

// Запуск модуля
void SimpleDDE::StartSimpleDDE(){

	XSimpledde_Start(&SimpleDDE_Inst);
	XSimpledde_EnableAutoRestart(&SimpleDDE_Inst);

}

// Установка усиления мелктх деталей
void SimpleDDE::SetSimpleDDE_K(float K){

	if(K > 7) K = 7;
	if(K < -6) K = -6;

	int data = K * 16;

	XSimpledde_Set_K(&SimpleDDE_Inst, data);

}
