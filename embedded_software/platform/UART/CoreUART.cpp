// =================================================================
// Project: OV7670 Video Processing System
// File: CoreUART.cpp
// Description: Driver for UART module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#include "CoreUART.h"

/* Конструктор */
CoreUART::CoreUART(uint16_t DeviceID, uint32_t BaudRate, uint8_t OperMode,
		uint16_t UartIntrID, XScuGic& GicInstance)
	:DeviceID(DeviceID), BaudRate(BaudRate), OperMode(OperMode), UartIntrID(UartIntrID),
	 GicInstancePtr(&GicInstance){															//Список инициализации

	 // Инициализация буферов
	memset(TxBuffer, 0, sizeof(TxBuffer));
	memset(RxBuffer, 0, sizeof(RxBuffer));
	IsRxComplete = false;
}

/* Деструктор */
CoreUART:: ~CoreUART(){}


/* Инициализация */
int CoreUART::InitUART()
{
	XUartPs_Config *Config;
	int Status;

	// Поиск конфигурации
	Config = XUartPs_LookupConfig(DeviceID);
	if (NULL == Config) {
		return XST_FAILURE;
	}

	// Инициализация драйвера uart
	Status = XUartPs_CfgInitialize(&UartInstance, Config, Config->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	// Установка параметров
	XUartPs_SetBaudRate(&UartInstance, BaudRate);
	XUartPs_SetOperMode(&UartInstance, OperMode);


	// Настройка прерывания
	Status = SetupInterruptSystem();
	if(Status != XST_SUCCESS){
		xil_printf("Failed to SetupInterruptSystem()\n");
		return XST_FAILURE;

	}


	return XST_SUCCESS;
}

//Отправка данных
void CoreUART::SendData(const uint8_t* msg, int size) {
	if(size >= UART_BUFFER_SIZE){
		xil_printf("Error: int size more then buffer!!!");
		return;
	}

    // Перед вызовом этого метода внешний код должен заполнить CoreUART::TxBuffer
    // Например: myUart.GetTxBuffer()[0] = 'H'; myUart.SendData(1);
	// Заполняем внутренний буффер
	memcpy(TxBuffer, msg, size);

    // Используем внутренний TxBuffer, отправляем данные
	XUartPs_Send(&UartInstance, TxBuffer, size);
    // Ожидание завершения передачи
    //while (XUartPs_IsSending(&UartInstance)); Не нужно, так как работаем с прерываниями
}

//Приём данных
int CoreUART::ReceiveData(uint8_t* destinationBuffer, int maxLen) {

    memcpy(destinationBuffer, RxBuffer, maxLen);

    // После вызова этого метода внешний код может прочитать данные из CoreUART::RxBuffer
    return maxLen;
}

/* Проверка наличия данных для неблокирующего чтения */
bool CoreUART::IsDataReady() {
    // Проверяет, есть ли хотя бы один байт в приемном FIFO
    return XUartPs_IsReceiveData(UartInstance.Config.BaseAddress);
}

bool CoreUART::IsTxComplete() {

	return XUartPs_IsTransmitFull(UartInstance.Config.BaseAddress);
}


/* Метод настройки прерывания */
int CoreUART:: SetupInterruptSystem(){
	int Status;

	// Подключение UART к GIC, подключаем стандартный обработчик
	Status = XScuGic_Connect(GicInstancePtr, UartIntrID, (Xil_ExceptionHandler)XUartPs_InterruptHandler,
			&UartInstance);
	if (Status != XST_SUCCESS) {
			xil_printf("Failed to connect UART1 interrupt\n");
	        return XST_FAILURE;
	    }

	// Установка порога FIFO для срабатывания прерывания (например, при 1 байте)
	XUartPs_SetFifoThreshold(&UartInstance, UART_BUFFER);

	// Установка функции обратного вызова (вашего обработчика)
	XUartPs_SetHandler(&UartInstance, (XUartPs_Handler)UartHandler, this);

	// Включение маски прерывания на прием данных и, возможно, таймаут
	uint32_t IntrMask =  XUARTPS_IXR_RXOVR | XUARTPS_IXR_TXEMPTY | XUARTPS_IXR_RXFULL; // Пример масок
	XUartPs_SetInterruptMask(&UartInstance, IntrMask);

	// Включение прерывания UART в GIC
	    XScuGic_Enable(GicInstancePtr, UartIntrID);

	return XST_SUCCESS;
}


/******************************GuideFilter**********************************/

// Регистрация Guide фильтра
void CoreUART::RegisterGuideFilter(GuideFilter& filter){
	GuideFilterInst = &filter;
}

// Метод, который будет вызываться в прерывании для обработки GuideFilter
void CoreUART::ProcessGuidrFilter(){
	if (IsRxComplete && GuideFilterInst != nullptr) {

		int ReceivedNumber = atoi((char*)RxBuffer);

		// UART САМ напрямую вызывает метод фильтра и отдает ему число!
		GuideFilterInst->SetESPGuideFilter(ReceivedNumber);

		// Сбрасываем флаги
		IsRxComplete = false;
		ReceivedBytesCount = 0;
		xil_printf("\r\nESP Set %d", ReceivedNumber);
	}
}

/****************************END GuideFilter**********************************/


/**********************************NUC****************************************/

// Регистрация NUC модуля
void CoreUART::RegisterNUC(NUC& nuc)
{
	NUCInst = &nuc;
}

// Метод, который будет вызываться для обработки nuc модуля
void CoreUART::ProcessNUC()
{
	xil_printf("\r\n 1");

	if(IsRxComplete && NUCInst != nullptr) {
		int ReceivedNumber = atoi((char*)RxBuffer);

		xil_printf("\r\n 2");

		// Вызываем функцию КГШ
		if(ReceivedNumber == 55) {
			NUCInst -> RunNUC(32);
		}

		// Сбрасываем флаги
		IsRxComplete = false;
		ReceivedBytesCount = 0;
		xil_printf("\r\n 3.ReceivedNumber: %d", ReceivedNumber);
	}
}

/*********************************END NUC*************************************/

/* Мой обработчик прерывания */
/* Статический обработчик прерываний (Callback Handler) */
void CoreUART::UartHandler(void *CallBackRef, uint32_t Event, unsigned int EventData)
{
    // Приведение указателя CallBackRef к типу вашего экземпляра CoreUART
    CoreUART *InstancePtr = static_cast<CoreUART*>(CallBackRef);
    // Экземпляр к UART
    XUartPs* UartHWPtr = &(InstancePtr->UartInstance);

    // В зависимости от типа события (Event), выполняем действия
    switch (Event) {
        case XUARTPS_EVENT_RECV_DATA:
        {
        	// Читаем все байты в буфер
        	uint32_t BytesRead = XUartPs_Recv(UartHWPtr, InstancePtr->RxBuffer, UART_BUFFER);

        	//xil_printf("\nData receive Done...\n");
			//xil_printf("Count byte read in handler: %d\n", BytesRead);

        	// Добавляем символ конца строки в конец принятых данных, чтобы atoi/sscanf сработали корректно
			if (BytesRead < UART_BUFFER) {
				InstancePtr->RxBuffer[BytesRead] = '\0';
			}

			// Устанавливаем флаг, что обработчик отработал и у нас есть данные
			InstancePtr->IsRxComplete = true;
			InstancePtr->ReceivedBytesCount = BytesRead;

			InstancePtr->ProcessNUC();


            break;
        }
/*
        case XUARTPS_EVENT_RECV_TOUT:
            // Таймаут приема (полезно для пакетов переменной длины)
            // EventData содержит количество байт, полученных до таймаута
            break;
*/
        case XUARTPS_EVENT_SENT_DATA:
        	xil_printf("Data sent Done...\n");
            break;
        // ... обработка ошибок и других событий ...

    }
}


