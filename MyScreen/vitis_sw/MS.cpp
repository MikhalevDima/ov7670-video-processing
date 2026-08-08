// =================================================================
// Project: OV7670 Video Processing System
// File: MS.cpp
// Description: Driver for MS module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#include "MS.hpp"

//Конструктор
MS::MS(uint32_t base_addr)
{
	BaseAddr = base_addr;
	MSDisable();
}

//Деструктор
MS::~MS(){}


//Включение видеопотока
void MS::MSEnable()
{
	MYSCREEN_mWriteReg(BaseAddr, CONTROL_REG, ENABLE);
}

//Выключение видеопотока
void MS::MSDisable()
{
	MYSCREEN_mWriteReg(BaseAddr, CONTROL_REG, DISABLE);
}
