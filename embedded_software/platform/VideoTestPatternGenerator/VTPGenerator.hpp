// =================================================================
// Project: OV7670 Video Processing System
// File: VTPGenerator.hpp
// Description: Driver for VTPGenerator module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#ifndef VTPGENERATOR_H
#define VTPGENERATOR_H

#include "xparameters.h"
#include "stdint.h"
#include "stdbool.h"
#include "xv_tpg.h"
#include "sleep.h"


/* Определение класса генератора шаблонов */
class GenCore
{
public:
	GenCore(uint16_t deviceID);				// Конструктор
	~GenCore();								// Деструктор

	int VTPinit();							// Инициализация генератора шаблонов

	void SetBackground_1080p(uint32_t ID);	// Установка шаблона фона по ID размером 1080р
	/*
	  XTPG_BKGND_H_RAMP = 1,
	  XTPG_BKGND_V_RAMP = 2,
	  XTPG_BKGND_TEMPORAL_RAMP = 3,
	  XTPG_BKGND_SOLID_RED = 4,
	  XTPG_BKGND_SOLID_GREEN = 5,
	  XTPG_BKGND_SOLID_BLUE = 6,
	  XTPG_BKGND_SOLID_BLACK = 7,
	  XTPG_BKGND_SOLID_WHITE = 8,
	  XTPG_BKGND_COLOR_BARS = 9,
	  XTPG_BKGND_ZONE_PLATE = 10,
	  XTPG_BKGND_TARTAN_COLOR_BARS = 11,
	  XTPG_BKGND_CROSS_HATCH = 12,
	  XTPG_BKGND_RAINBOW_COLOR = 13,
	  XTPG_BKGND_HV_RAMP = 14,
	  XTPG_BKGND_CHECKER_BOARD = 15,
	  XTPG_BKGND_PBRS = 16,
	  XTPG_BKGND_DP_COLOR_RAMP = 17,
	  XTPG_BKGND_DP_BW_VERTICAL_LINE = 18,
	  XTPG_BKGND_DP_COLOR_SQUARE = 19,
	  XTPG_BKGND_LAST = 20
	*/

	void GenMotion();						// Стартуем и включаем разные шаблоны

private:

	uint16_t DeviceID;						// ID генератора шаблонов
	XV_tpg tpg;								// Экземпляр генератора шаблонов

};


#endif // VTPGENERATOR_H
