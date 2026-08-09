// =================================================================
// Project: OV7670 Video Processing System
// File: CoreGIC.hpp
// Description: Driver for Generic Interrupt Controller
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#ifndef CoreGIC_H
#define CoreGIC_H

#include "xparameters.h"
#include "xscugic.h"
#include "xil_io.h"

class CoreGIC
{

public:
	// Конструктор / деструктор
	CoreGIC(uint32_t DeviceID);
	~CoreGIC();

	// Инициализация GIC модуля
	int IntInterruptController();

	//Получения эекземпляра контроллера прерывания GIC
	XScuGic& GetInstanceInterruptController();

	//Глобальное включение прерывания
	void ExceptionEnable();

private:
	// Device ID GIC в системе
	uint32_t DeviceID;
	// Экземпляр контроллеоа прерывания GIC
	XScuGic InstanceInterruptController;
};


#endif // CoreGIC_H
