// =================================================================
// Project: OV7670 Video Processing System
// File: CoreUART.hpp
// Description: Driver for UART module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#ifndef CoreUART_H
#define CoreUART_H

#include "xparameters.h"

#include "xscugic.h"
#include "xuartps.h"
#include "xstatus.h"
#include "xil_io.h"
#include "xil_printf.h"
#include <cstdlib> 				// Библиотека, где объявлена функция atoi
#include "GuideFilter.hpp"
#include "NUC.hpp"

// Размер буфера 64 байт
const int UART_BUFFER_SIZE = 64;
// Размер буфера в UART
const int UART_BUFFER = 2;

/* Объявление прототипа класса UARTps */
class CoreUART
{

public:
	CoreUART(uint16_t DeviceID, uint32_t BaudRate, uint8_t OperMode,
			uint16_t UartIntrID, XScuGic& GicInstance);									// Конструктор
	~CoreUART();																		// Деструктор

	int InitUART();																		// Инициализация UART
	int SetupInterruptSystem();														    // Настройка прерывания

	/* Сделаем буфер внутри класса
    void SendData(uint8_t* DataBuffer, uint32_t ByteCount);
    u32 ReceiveData(uint8_t* DataBuffer, uint32_t ByteCount);
*/
	void SendData(const uint8_t* msg, int size);
	int ReceiveData(uint8_t* destinationBuffer, int maxLen);

	// Данные пришли надо обрабатывать
    bool IsDataReady(); 																// Проверка наличия данных в буфере

    // Отправка данных завершена если false
    bool IsTxComplete();

    // Флаг приёма данных
    volatile bool IsRxComplete;

    //Количество принятых байт
    volatile int ReceivedBytesCount;

    // Регистрация Guide фильтра
    void RegisterGuideFilter(GuideFilter& filter);
	// Метод, который будет вызываться в прерывании
	void ProcessGuidrFilter();

	// Регистрация NUC модуля
	void RegisterNUC(NUC& nuc);
	// Метод, который будет вызываться для обработки nuc модуля
	void ProcessNUC();

private:
    XUartPs UartInstance; 										// Экземпляр драйвера Xilinx SDK
    uint16_t DeviceID;
    uint32_t BaudRate;											// Скорость передачи данных
	uint8_t OperMode;											// Тип работы UART
	uint16_t UartIntrID;										// ID прерывания
	XScuGic* GicInstancePtr;									// Указатель на контроллео GIC
	//GuideFilter* GuideFilterInst = nullptr; 					// Указатель на Guide фильтр
	NUC* NUCInst = nullptr;										// Указатель на NUC модуль

	// Буферы внутри класса
	uint8_t TxBuffer[UART_BUFFER_SIZE];
	uint8_t RxBuffer[UART_BUFFER_SIZE];

	/*
	 * 1. Нестатическая функция C++: Имеет скрытый указатель this (адрес конкретного объекта),
	 * который автоматически передается компилятором при вызове.
	 * Прототип функции в итоге не соответствует тому, что ожидает Си-API.
	 *
	 * 2. Статическая функция C++ (static): Не имеет скрытого указателя this.
	 * Её сигнатура (прототип) становится точно такой же, как у обычной Си-функции, и её адрес
	 * (указатель на функцию) можно безопасно передать в качестве колбэка.
	 *
	 * Таким образом, static позволяет "мостикнуть" разрыв между объектно-ориентированным
	 * C++ и процедурным Си-API, используемым в Vitis SDK.
	 */
	static void UartHandler(void *CallBackRef, u32 Event, unsigned int EventData);		// Обработчик прерывания

};

#endif // CoreUART_H
