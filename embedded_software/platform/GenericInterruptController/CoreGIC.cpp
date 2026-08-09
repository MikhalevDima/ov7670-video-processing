// =================================================================
// Project: OV7670 Video Processing System
// File: CoreGIC.cpp
// Description: Driver for Generic Interrupt Controller
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#include "CoreGIC.h"

// Конструктор
CoreGIC::CoreGIC(uint32_t DeviceID)
	:DeviceID(DeviceID){

}

// Деструктор
CoreGIC::~CoreGIC(){}


// Инициализация GIC модуля
int CoreGIC::IntInterruptController(){
	XScuGic_Config *IntcConfig;
	// ... логика поиска и инициализации GIC ...
	IntcConfig = XScuGic_LookupConfig(DeviceID);
	XScuGic_CfgInitialize(&InstanceInterruptController, IntcConfig, IntcConfig->CpuBaseAddress);

	// Настройка обработчиков ARM ядра
	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
								 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
								 &InstanceInterruptController);

//	Xil_ExceptionEnable(); // Глобальное включение прерываний

	return XST_SUCCESS;
}

//Глобальное включение прерывания
void CoreGIC::ExceptionEnable(){
	Xil_ExceptionEnable(); // Глобальное включение прерываний
}


//Получения эекземпляра контроллера прерывания GIC
XScuGic& CoreGIC:: GetInstanceInterruptController(){
	return InstanceInterruptController;
}

/*
 * В коде Zynq важно соблюдать порядок:
Инициализация GIC.
Инициализация I2C (XIicPs_CfgInitialize).
Установка колбэка (XIicPs_SetStatusHandler).
Подключение к GIC (XScuGic_Connect для системного обработчика драйвера).
Включение прерывания в GIC (XScuGic_Enable).
Включение исключений процессора (Xil_ExceptionEnable).
Если вызвать Xil_ExceptionEnable() до того, как настроили обработчик I2C,
первое же случайное прерывание может увести процессор в StubHandler.
 */











