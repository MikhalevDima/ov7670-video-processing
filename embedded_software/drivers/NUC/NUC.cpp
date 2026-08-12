// =================================================================
// Project: OV7670 Video Processing System
// File: NUC.cpp
// Description: Driver for NUC module
// Developer: MikhalevDima
// Year: 2026
// =================================================================


#include "NUC.hpp"

// Реализация Класса

// Конструктор
NUC::NUC(uint16_t deviceID)
	: DeviceID(deviceID)
{}

// Деструктор
NUC::~NUC(){}

// Инициализация модуля
int NUC::InitNUC()
{
	int Status;
	Status = XNuc_Initialize(&NUCInst, DeviceID);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	XNuc_Set_ddr_mem(&NUCInst, (u64) HLS_FRAME);			// Передадим указатель на память в DDR

	return XST_SUCCESS;
}

// Старт модуля
void NUC::StartNUC()
{
	XNuc_Start(&NUCInst);
	XNuc_EnableAutoRestart(&NUCInst);

}


// Режимы работы модуля
// 1. Байпас (без КГШ)
void NUC::BypassNUC()
{
	XNuc_Set_mode(&NUCInst, 0);
}
// 2. Счёт КГШ
void NUC::CalculateNUC()
{
	XNuc_Set_mode(&NUCInst, 2);
}
// 3. Применение КГШ
void NUC::ApplicationNUC()
{
	XNuc_Set_mode(&NUCInst, 1);
}

// Запуск КГШ
void NUC::RunNUC(uint16_t frame)
{
	static uint16_t buffer[FRAME_SIZE] = {0}; 			// Буфер аккумулятора пикселей
	static uint8_t bias_buffer[FRAME_SIZE] = {0};    	// Буфер для рассчитанных коэффициентов

	// Приводим физический адрес DDR к указателю, с которым может работать процессор
	uint8_t* ddr_ptr = (uint8_t*)HLS_FRAME;

	uint32_t total_sum = 0; // Общая сумма ВЕХ пикселей ВСЕХ кадров для честного среднего


	 // Обнуляем аккумулятор перед стартом калибровки
	for(int i = 0; i < 640 * 480; i++) {
		buffer[i] = 0;
	}


	XNuc_DisableAutoRestart(&NUCInst);					// Отключаем автостарт модуля

	// 1. Сбор статистики кадра
	for(int i = 0; i < frame; i++){

		CalculateNUC();									// Включаем режим расчёта КГШ
		XNuc_Start(&NUCInst);							// Запускаем модуль для записи на один кадр
		while(!XNuc_IsDone(&NUCInst));					// Ждём когда модуль отработает

		// Это заставит ARM читать данные честно из DDR, куда их только что положил HLS.
		Xil_DCacheInvalidateRange((INTPTR)ddr_ptr, FRAME_SIZE * sizeof(uint8_t));

		// Накапливаем пиксели текущего кадра в общий массив
		for(int i = 0; i < FRAME_SIZE; i++) {
			uint8_t current_pixel = ddr_ptr[i];
			buffer[i] += current_pixel;
			total_sum += current_pixel;
		}
	}

	// Общее количество пикселей во всех накопленных кадрах
	uint64_t total_pixels_processed = (uint64_t)FRAME_SIZE * frame;
	uint32_t global_mean = total_sum / total_pixels_processed;

	// Защита от деления на 0, если камера закрыта крышкой и всё чёрное
	if(global_mean < 10) global_mean = 10;


	// Определяем сдвиг для быстрой замены деления на frame
	// Внимание: вызывайте функцию строго с frame = 16, 32, 64 или 128!
	uint32_t shift_val = 4; // по умолчанию для 16 кадров (2^4 = 16)
	if (frame == 32)  shift_val = 5;
	if (frame == 64)  shift_val = 6;
	if (frame == 128) shift_val = 7;

	// 3. Расчёт коэффициентов
	for(int i = 0; i < FRAME_SIZE; i++) {
		//uint32_t pixel_average = buffer[i] / frame; // Усредненный пиксель в данной точке [i]

		// Быстрое деление через сдвиг вместо: buffer[i] / frame
		uint32_t pixel_average = buffer[i] >> shift_val;

		// Формула КГШ: коэффициент = (Общее_Среднее * Масштаб) / Пиксель_в_точке
		// Если пиксель в точке равен среднему, коэф = 128 (то есть 1.0)
		if (pixel_average == 0) pixel_average = 1;

		uint32_t calc_bias = (global_mean * 128) / pixel_average;

		// Ограничиваем, чтобы коэффициент поместился в uint8_t (0..255)
		//if(calc_bias > 255) calc_bias = 255;

		 // СМЯГЧЕНИЕ ШУМА (Ограничиваем диапазон коэффициентов)
		// Не даем калибровке выкручивать яркость на зашумленных пикселях
		if(calc_bias > 255) calc_bias = 220; // Максимум усиления (+25%)
		if(calc_bias < 50)  calc_bias = 96;  // Минимум ослабления (-25%)

		bias_buffer[i] = (uint8_t)calc_bias;
	}

	// 4. Запись коэффициентов в DDR
	for(int i = 0; i < FRAME_SIZE; i++) {
		ddr_ptr[i] = bias_buffer[i];
	}

	// чтобы HLS ядро при чтении увидело обновленные данные, а не старый записанный кадр.
	Xil_DCacheFlushRange((INTPTR)ddr_ptr, FRAME_SIZE * sizeof(uint8_t));

	// 5. Возврат в режим работы
	ApplicationNUC(); // Переключаем HLS в режим применения КГШ (mode = 1)
	StartNUC();       // Запускаем обратно в непрерывном режиме (AutoRestart)
}














