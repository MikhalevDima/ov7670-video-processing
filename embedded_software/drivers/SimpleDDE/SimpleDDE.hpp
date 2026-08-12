// =================================================================
// Project: OV7670 Video Processing System
// File: S
// Description: Driver for SimpleDDE module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#pragma once

#include "xparameters.h"
#include "xsimpledde.h"

class SimpleDDE {

public:
	// Конструктор
	SimpleDDE(uint16_t baseaddr);

	// Деструктор
	~SimpleDDE();

	// Инициализация модуля
	int InitSimpleDDE();

	// Запуск модуля
	void StartSimpleDDE();

	// Установка усиления мелктх деталей
	void SetSimpleDDE_K(float K);


private:
	uint16_t BaseAddr;
	XSimpledde SimpleDDE_Inst;
};
