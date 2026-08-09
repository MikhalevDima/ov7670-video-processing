// =================================================================
// Project: OV7670 Video Processing System
// File: I2C.cpp
// Description: Driver for I2C module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#include "I2C.hpp"

//Конструктор
I2C::I2C(uint16_t deviceID, XScuGic &GicInstancePtr)
	:DeviceID(deviceID), GicInstancePtr(GicInstancePtr)
{
	RecivieFlag = false;
}

//Деструктор
I2C::~I2C(){}

//Инициализация модуля
int I2C::InitI2C()
{
	int Status;
	XIicPs_Config* Config;

	// Поиск конфигурации
	Config = XIicPs_LookupConfig(DeviceID);
	if (NULL == Config) {
		return XST_FAILURE;
	}

	// Инициализация модуля I2C
	Status = XIicPs_CfgInitialize(&I2Cinst, Config, Config->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	//XIicPs_Reset(&I2Cinst);								//Сброс до использования, не нужно иначе надо заново инициализировать

	Status = XIicPs_SetSClk(&I2Cinst, 400000);				//Установка частоты 100 кГц
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}

	return XST_SUCCESS;

}

//Проверка занята шина или нет
bool I2C::IsBusy_i2c(){
	bool Status = (bool)XIicPs_BusIsBusy(&I2Cinst);
	return Status;
}


//Сброс контроллера I2C
void I2C::Reset(){
	XIicPs_Reset(&I2Cinst);
}

//Отправка данных в режиме опроса
int I2C::MasterSentPolled(uint8_t *MsgPtr, int ByteCount, uint16_t SlaveAddr){

	int Status = XIicPs_MasterSendPolled(&I2Cinst, MsgPtr, ByteCount, SlaveAddr);

	return Status;
}

//Приём данных в режиме опроса
int I2C::MasterRecivePolled(uint8_t *MsgPtr, int ByteCount, uint16_t SlaveAddr){

	int Status = XIicPs_MasterRecvPolled(&I2Cinst, MsgPtr, ByteCount, SlaveAddr);

	return Status;

}

//Отправить данные
void I2C::Write_with_interrupt(uint8_t *MsgPtr, int ByteCount, uint16_t SlaveAddr)
{
	while(IsBusy);													//Ждём, если предыдущая передача ещё идет
	IsBusy = true;
	XIicPs_MasterSend(&I2Cinst, MsgPtr, ByteCount, SlaveAddr);
	while(IsBusy);
}

//Приём данных
void I2C::Read_with_interrupt(uint8_t *MsgPtr, int ByteCount, uint16_t SlaveAddr)
{
	RecivieFlag = false;
	XIicPs_MasterRecv(&I2Cinst, MsgPtr, ByteCount, SlaveAddr);

}


//Настройка прерывания модуля I2C
int I2C::InitInterruptI2C_MasterMode(uint16_t InterruptID)			//Передаём ссылку на GIC контроллер
{
	int Status;

	// Установка функции обратного вызова (вашего обработчика)
		XIicPs_SetStatusHandler(&I2Cinst, (void*) this, (XIicPs_IntrHandler)MyI2CHandler); // CallBack - на мой класс (this)

	//Регестрируем обработчик в GIC контроллере
	Status =  XScuGic_Connect(&GicInstancePtr, InterruptID, (Xil_ExceptionHandler)XIicPs_MasterInterruptHandler,
			&I2Cinst);
	if (Status != XST_SUCCESS) {
			xil_printf("Failed to connect I2C interrupt\n");
	       return XST_FAILURE;
	    }


	//Устанавливаем приоритет вызова
	XScuGic_SetPriorityTriggerType(&GicInstancePtr, InterruptID, 0xA0, 0x1);

	//Включаем прервывание в модуле I2C
	 XScuGic_Enable(&GicInstancePtr, InterruptID);

	return XST_SUCCESS;

}

//Мой обработчик
void I2C::MyI2CHandler(void *CallBackRef, uint32_t StatusEvent)

{
	// Приведение указателя CallBackRef к типу нашего экземпляра I2C
	I2C *InstancePtr = static_cast<I2C*>(CallBackRef);

	//Отправка данных завершена
	if(StatusEvent & (XIICPS_EVENT_COMPLETE_SEND | XIICPS_EVENT_ERROR | XIICPS_EVENT_NACK)){
		InstancePtr -> IsBusy = false;					//Освобождаем шину
	}

	//Если приём данных завершён
	if(StatusEvent & XIICPS_EVENT_COMPLETE_RECV){
		xil_printf("Recive completed I2C0.\n");

	}

	//Произошла ошибка
	if(StatusEvent & XIICPS_EVENT_ERROR){
		xil_printf("ERROR I2C0.\n");

	}
}












