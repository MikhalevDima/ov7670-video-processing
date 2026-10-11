// =================================================================
// Project: OV7670 Video Processing System
// File: Histogram.hpp
// Description: Driver for Histogram module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#pragma once

#include "xparameters.h"
#include "xhistogram.h"
#include "xil_printf.h"
#include "sleep.h"
#include <cstdio>
#include <cmath>

// Класс для управления модулем Histogram

class Histogram
{
public:
	// Конструктор
	Histogram(uint16_t deviceID);

	// Деструктор
	~Histogram();

	// Инициализация модуля
	int IntHistogram();

	// Старт модуля
	void StartHistogram();

	// Стоп модуля
	void StopHistogram();

	// Получение гистограммы из модуля, в hist_buffer
	void ReadHistData();

	// Минимальное значение, первый индекс слева
	int MinHist( long unsigned int *buffer, long unsigned int threshold);

	// Максимальное значение, первый индекс справа
	int MaxHist( long unsigned int *buffer, long unsigned int threshold);

	// Ширина гистограммы
	int WidthHist(long unsigned int *buffer, long unsigned int threshold_left, long unsigned int threshold_right);

	/* Статистика */

	// Аккумулятор пикселей
	long unsigned int Accumulator();

	// Математическое ожидание (Средняя яркость)
	int MathMeanHist();

	// Дисперсия
	int DispHist();

	/* Вывод на печать */

	// Печать гистограммы в printf
	void PrintHistogram(long unsigned int *buffer);

	// Печать минимального значения
	void PrintMinHist(long unsigned int *buffer);

	// Печать максимальное значения
	void PrintMaxHist(long unsigned int *buffer);

	// Печать ширины гистограммы
	void PrintWidhtHist(long unsigned int *buffer);

	// Вывод на печать математическое ожидание
	void PrintMathMeanHist();

	// Вывод на печать дисперсии
	void PrintDiapHist();


	long unsigned int hist_buffer[256];		// Буфер для хранения гистограммы

private:
	uint16_t DevaiceID;						// Идентификатор модуля в xparameters.h
	XHistogram HistogramInst;				// Экземпляр модуля

	// Константа количества ячеек в гистограмме
	int HIST_SIZE = 256;

	// Общее количестов пикселей
	int TOTAL_PIXEL = 307200;

};
