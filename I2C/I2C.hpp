// =================================================================
// Project: OV7670 Video Processing System
// File: I2C.hpp
// Description: Driver for I2C module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#pragma once

#include "xparameters.h"
#include "xiicps.h"
#include "xscugic.h"

/* Класс I2C */

class I2C
{

public:

	//Конструктор
	I2C(uint16_t deviceID, XScuGic &GicInstancePtr);

	//Деструктор
	~I2C();

	//Инициализация
	int InitI2C();

	//Отправка данных в режиме опроса
	int MasterSentPolled(uint8_t *MsgPtr, int ByteCount, uint16_t SlaveAddr);

	//Приём данных в режиме опроса
	int MasterRecivePolled(uint8_t *MsgPtr, int ByteCount, uint16_t SlaveAddr);

	//Отправить данные
	void Write_with_interrupt(uint8_t *MsgPtr, int ByteCount, uint16_t SlaveAddr);

	//Приём данных
	void Read_with_interrupt(uint8_t *MsgPtr, int ByteCount, uint16_t SlaveAddr);

	//Настройка прерывания
	int InitInterruptI2C_MasterMode(uint16_t InterruptID);

	//С помощью этой фун. указывает какую фун. из main вызвать
	void SetOnCompleteHandler(I2CCallback handler);

	//Проверка занята шина или нет
	bool IsBusy_i2c();

	//Сброс контроллера I2C
	void Reset();

	volatile bool RecivieFlag;												//Флаг данные приняты
	volatile bool ErrorFlag;												//Флаг ошибки

private:
	uint16_t DeviceID;														//Идентификатор модуля
	XIicPs I2Cinst;															//Экземпляр модуля
	XScuGic &GicInstancePtr;												//Ссылка на контроллео GIC


	volatile bool IsBusy = false;											//Флаг занятости функции

	static void MyI2CHandler(void *CallBackRef, uint32_t StatusEvent);		// Обработчик прерывания
};







