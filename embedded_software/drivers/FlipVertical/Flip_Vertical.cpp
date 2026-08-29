// =================================================================
// Project: OV7670 Video Processing System
// File: Flip_Vertical.cpp
// Description: Driver for Flip_Vertical module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#include "Flip_Vertical.hpp"

// Конструктор
Flip_Vertical::Flip_Vertical(uint16_t deviceID, XScuGic &gicInstancePtr)
			: DeviceId(deviceID), GicInstancePtr(gicInstancePtr) {};

// Деструктор
Flip_Vertical::~Flip_Vertical() {};

// Инициализация модуля
int Flip_Vertical::InitFlipVertical(){

	int Status = XFlip_vertical_Initialize(&FlipVerticalInst, DeviceId);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	return XST_SUCCESS;
}

// Старт модуля
void Flip_Vertical::StartFlipVertical(){

	XFlip_vertical_Start(&FlipVerticalInst);
	XFlip_vertical_EnableAutoRestart(&FlipVerticalInst);

}

// Стоп модуля
void Flip_Vertical::StopFlipVertical(){

	XFlip_vertical_DisableAutoRestart(&FlipVerticalInst);

}

// Режим работы модуля (отражение верх - низ)
void Flip_Vertical::SetModeFlipVertical(uint8_t mode){

	switch (mode) {
		case 0:
			XFlip_vertical_Set_mode(&FlipVerticalInst, 0);	// Байпасс
			break;
		case 1:
			XFlip_vertical_Set_mode(&FlipVerticalInst, 1);	// Переворот по вертикале
			break;
		default:
			XFlip_vertical_Set_mode(&FlipVerticalInst, 0);	// Байпасс
			break;
	}

}

// Старт модуля в режиме прерывания
void Flip_Vertical::ProcessSingleFrameInterrupt(){

	// 1. Меняем адрес в зависимости от текущего шага
	if(buffer_toggle == 0) {
		XFlip_vertical_Set_frame_read(&FlipVerticalInst, (u64)HLS_FRAME_READ);
		XFlip_vertical_Set_frame_write(&FlipVerticalInst, (u64)HLS_FRAME_WRITE);
	} else {
		XFlip_vertical_Set_frame_read(&FlipVerticalInst, (u64)HLS_FRAME_WRITE);
		XFlip_vertical_Set_frame_write(&FlipVerticalInst, (u64)HLS_FRAME_READ);
	}

	// 2. Сбрасываем кэш для буфера чтения (чтобы ПЛИС увидела новые данные)
	u32 current_read_addr = (buffer_toggle == 0) ? HLS_FRAME_READ : HLS_FRAME_WRITE;
	Xil_DCacheFlushRange((UINTPTR)current_read_addr, 640 * 480);

	// 3. Сбрасываем флаг прерывания перед запуском
	hls_frame_ready = 0;

	// 4. Запускаем HLS-ядро вручную на один кадр
	XFlip_vertical_Start(&FlipVerticalInst);
}


// Метод для переключения кадров в режиме прерывания, размещается в main -> while
void Flip_Vertical::ProcessSingleFrameInterruptWhile(){

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
int Flip_Vertical::InitInterrupt(uint16_t InterruptID){

	int Status;
	Status = XScuGic_Connect(&GicInstancePtr,
			InterruptID,
			(Xil_InterruptHandler)HandlerInterruptFlipVertical,
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
	 XFlip_vertical_InterruptEnable(&FlipVerticalInst, 1);
	 XFlip_vertical_InterruptGlobalEnable(&FlipVerticalInst);

	 return XST_SUCCESS;

}

// Обработчик прерывания, будет вызываться при прерывании
void Flip_Vertical::HandlerInterruptFlipVertical(void *CallBackRef){

	// Приведение указателя CallBackRef к типу нашего экземпляра ReadWriteDDR
	Flip_Vertical *InstancePtr = static_cast<Flip_Vertical*>(CallBackRef);

	// 1. ОЧИЩАЕМ прерывание в ЖЕЛЕЗЕ:
	// Читаем статус регистра ISR. Так как он COR (Clear on Read),
	// чтение автоматически сбросит флаг в HLS-модуле и опустит линию прерывания.
	uint32_t IntrStatus = XFlip_vertical_InterruptGetStatus(&InstancePtr->FlipVerticalInst);

	// 2. Проверяем, что прерывание произошло именно по ap_done (нулевой бит)
	if (IntrStatus & 1) {
	// 3. Взводим флаг готовности кадра (переменная ДОЛЖНА быть volatile)
		InstancePtr->hls_frame_ready = 1;

	}

}








































