// =================================================================
// Project: OV7670 Video Processing System
// File: AGC.hpp
// Description: Driver for AGC module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#pragma once

#include "xparameters.h"
#include "xagc.h"

// Класс для управления модулем AGC
class AGC
{
public:
	// Конструктор
	AGC(uint16_t deviceID);

	// Деструктор
	~AGC();

	// Инициализация модуля
	int InitAGC();

	// Старт модуля
	void StartAGC();

	// Установления коэффициента усиления контрастности
	void SetGain(int Disp_cur, int Disp_target, float u_attack, float u_release);

private:

	uint16_t DevaiceID;								// Идентификатор модуля в xparameters.h
	XAgc AGCInst;									// Экземпляр модуля

	float Gain_old;     							// Внутреннее состояние регулятора

};
