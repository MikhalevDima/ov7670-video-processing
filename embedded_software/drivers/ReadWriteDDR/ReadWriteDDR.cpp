// =================================================================
// Project: OV7670 Video Processing System
// File: ReadWriteDDR.cpp
// Description: Driver for ReadWriteDDR module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#include "ReadWriteDDR.hpp"

// Реализация Класса

// Конструктор
ReadWriteDDR:: ReadWriteDDR(uint16_t device_id, XScuGic &gicInstancePtr)
	: DeviceID(device_id), GicInstancePtr(gicInstancePtr) {}

// Деструктор
ReadWriteDDR:: ~ReadWriteDDR(){}

// Инициализация модуля
int ReadWriteDDR::InitReadWriteDDR()
{
	int Status;
	Status = XReadwriteddr_Initialize(&ReadWriteInst, DeviceID);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	buffer_toggle = 0; // Инициализируем флаг переключения
	return XST_SUCCESS;

}

// Старт модуля
void ReadWriteDDR::ProcessSingleFrame()
{
	/*
	// 1. Приводим числовой адрес к указателю на байт (unsigned char)
	uint8_t *buffer_ptr = (uint8_t *)HLS_FRAME_READ;

	// 2. Запускаем цикл по всему размеру кадра
	for (int i = 0; i < (640 * 480); i++) {
	    buffer_ptr[i] = 128;
	}


	// 3. ОБЯЗАТЕЛЬНО выталкиваем данные из кэша ARM в физическую DDR память!
	// Без этой строчки ПЛИС (HLS-ядро) прочитает из DDR старый мусор.
	Xil_DCacheFlushRange(HLS_FRAME_READ, 640 * 480);
*/

	// 1. Меняем адреса в зависимости от текущего шага
	if (buffer_toggle == 0) {
		XReadwriteddr_Set_frame_read(&ReadWriteInst,  (u64)HLS_FRAME_READ);
		XReadwriteddr_Set_frame_write(&ReadWriteInst, (u64)HLS_FRAME_WRITE);
	} else {
		XReadwriteddr_Set_frame_read(&ReadWriteInst,  (u64)HLS_FRAME_WRITE);
		XReadwriteddr_Set_frame_write(&ReadWriteInst, (u64)HLS_FRAME_READ);
	}

	// 2. Сбрасываем кэш для буфера чтения (чтобы ПЛИС увидела новые данные)
	u32 current_read_addr = (buffer_toggle == 0) ? HLS_FRAME_READ : HLS_FRAME_WRITE;
	Xil_DCacheFlushRange((UINTPTR)current_read_addr, 640 * 480);

	// 3. Запускаем HLS-ядро вручную на один кадр
	XReadwriteddr_Start(&ReadWriteInst);

	// 4. Ждем, пока оно отработает кадр (Polling)
	while (!XReadwriteddr_IsDone(&ReadWriteInst)) {
		// Ожидание
	}

	// 5. Инвалидируем кэш буфера записи
	u32 current_write_addr = (buffer_toggle == 0) ? HLS_FRAME_WRITE : HLS_FRAME_READ;
	Xil_DCacheInvalidateRange((UINTPTR)current_write_addr, 640 * 480);

	// 6. Переключаем флаг для следующего вызова
	buffer_toggle = !buffer_toggle;
}

// Старт модуля в режиме прерывания
void ReadWriteDDR::ProcessSingleFrameInterrupt()
{
	// 1. Меняем адреса в зависимости от текущего шага
	if (buffer_toggle == 0) {
		XReadwriteddr_Set_frame_read(&ReadWriteInst,  (u64)HLS_FRAME_READ);
		XReadwriteddr_Set_frame_write(&ReadWriteInst, (u64)HLS_FRAME_WRITE);
	} else {
		XReadwriteddr_Set_frame_read(&ReadWriteInst,  (u64)HLS_FRAME_WRITE);
		XReadwriteddr_Set_frame_write(&ReadWriteInst, (u64)HLS_FRAME_READ);
	}

	// 2. Сбрасываем кэш для буфера чтения (чтобы ПЛИС увидела новые данные)
	u32 current_read_addr = (buffer_toggle == 0) ? HLS_FRAME_READ : HLS_FRAME_WRITE;
	Xil_DCacheFlushRange((UINTPTR)current_read_addr, 640 * 480);

	// Сбрасываем флаг прерывания перед запуском
	hls_frame_ready = 0;

	// 3. Запускаем HLS-ядро вручную на один кадр
	XReadwriteddr_Start(&ReadWriteInst);


}

// Метод для переключения кадров в режиме прерывания, размещается в main -> while
void ReadWriteDDR::ProcessSingleFrameInterruptWhile()
{

	if(hls_frame_ready == 1) {
		// Инвалидируем кэш буфера записи, чтобы процессор увидел новые записи
		u32 current_write_addr = (buffer_toggle == 0) ? HLS_FRAME_WRITE : HLS_FRAME_READ;
		Xil_DCacheInvalidateRange((UINTPTR)current_write_addr, 640 * 480);

		// Переключаем флаг для следующего вызова
		buffer_toggle = !buffer_toggle;

		// Запускаем следующий кадр
		ProcessSingleFrameInterrupt();

	}
}

// Настройка режима прерывания
int ReadWriteDDR::InitInterrupt(uint16_t InterruptID)
{

	int Status;
	Status = XScuGic_Connect(&GicInstancePtr,
            InterruptID,
            (Xil_InterruptHandler)HandlerInterruptReadWriteDDR,
            this);

	if (Status != XST_SUCCESS) {
			xil_printf("Failed to connect I2C interrupt\n");
		   return XST_FAILURE;
		}

	//Включаем прервывание в контроллере GIC
	 XScuGic_Enable(&GicInstancePtr, InterruptID);

	 // Фиксируем обработку по ВОСХОДЯЩЕМУ ФРОНТУ (0x03)
	 // Это заставит GIC Zynq-7000 реагировать именно на момент окончания работы (фронт)
	 XScuGic_SetPriorityTriggerType(&GicInstancePtr, InterruptID, 0xA0, 0x03);


	 //Разрешаем прерывания внутри самого HLS-ядра
	 XReadwriteddr_InterruptEnable(&ReadWriteInst, 1);
	 XReadwriteddr_InterruptGlobalEnable(&ReadWriteInst);

	 return XST_SUCCESS;
}

// Обработчик прерывания
void ReadWriteDDR::HandlerInterruptReadWriteDDR(void *CallBackRef)
{
	// Приведение указателя CallBackRef к типу нашего экземпляра ReadWriteDDR
	ReadWriteDDR *InstancePtr = static_cast<ReadWriteDDR*>(CallBackRef);

	// 1. ОЧИЩАЕМ прерывание в ЖЕЛЕЗЕ:
	// Читаем статус регистра ISR. Так как он COR (Clear on Read),
	// чтение автоматически сбросит флаг в HLS-модуле и опустит линию прерывания.
	uint32_t IntrStatus = XReadwriteddr_InterruptGetStatus(&InstancePtr->ReadWriteInst);

	// 2. Проверяем, что прерывание произошло именно по ap_done (нулевой бит)
	if (IntrStatus & 1) {
	// 3. Взводим флаг готовности кадра (переменная ДОЛЖНА быть volatile)
		InstancePtr->hls_frame_ready = 1;

	}
}


// Читаем статистику
void ReadWriteDDR::StatRead()
{
	int Stat;
	Stat = XReadwriteddr_Get_stat(&ReadWriteInst);

	// Разбор Stat
	uint8_t min_p = Stat & 0xFF;
	uint8_t max_p = (Stat >> 8) & 0xFF;
	uint8_t avg = (Stat >> 16) & 0xFF;

	// Печатаем результат
	xil_printf("Stat -> Min: %d | Max: %d | Avg: %d\r", min_p, max_p, avg);
	usleep(100);
}


















