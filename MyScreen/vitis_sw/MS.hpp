// =================================================================
// Project: OV7670 Video Processing System
// File: MS.hpp
// Description: Driver for MS module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#ifndef MS_H
#define MS_H

#include "xil_io.h"
#include "MyScreen.h"
#include "xparameters.h"

/* Определение класса MS */

class MS {

	//Регистры IP модуля
	enum {
			CONTROL_REG = MYSCREEN_S00_AXI_SLV_REG0_OFFSET,
			REG1 = MYSCREEN_S00_AXI_SLV_REG1_OFFSET,
			REG2 = MYSCREEN_S00_AXI_SLV_REG2_OFFSET,
			REG3 = MYSCREEN_S00_AXI_SLV_REG3_OFFSET
	};

	//Команды управления в CONTROL REG
	enum {
			ENABLE = 0x00000001,
			DISABLE = 0x00000000
	};

public:
	//Конструктор
	MS(uint32_t base_addr);

	//Деструктор
	~MS();

	//Включение видеопотока
	void MSEnable();
	//Выключение видеопотока
	void MSDisable();

private:
	uint32_t BaseAddr;
};

#endif // MS_H
