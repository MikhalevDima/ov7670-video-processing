// =================================================================
// Project: OV7670 Video Processing System
// File: AdvancedDDE.cpp
// Description: Driver for AdvancedDDE module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#include "AdvancedDDE.hpp"

// Конструктор
AdvancedDDE::AdvancedDDE(uint16_t baseaddr)
	: BaseAddr(baseaddr) {}

// Деструктор
AdvancedDDE::~AdvancedDDE() {};

// Инициализация модуля
int AdvancedDDE::IntAdvancedDDE(){

	int Status;
	Status = XAdvanceddde_Initialize(&AdvancedDDEInst, BaseAddr);
	if (Status != XST_SUCCESS) {
				return XST_FAILURE;
			}
		return XST_SUCCESS;
}

// Запуск модуля
void AdvancedDDE::StartAdvancedDDE(){

	XAdvanceddde_Start(&AdvancedDDEInst);
	XAdvanceddde_EnableAutoRestart(&AdvancedDDEInst);

}

// Обновление DDE таблиц
void AdvancedDDE::SetDDETable(float gamma, float k_max, float threshold){

	uint8_t base_table[256];
	uint8_t k_table[256];

	// 1. Расчёт таблицы гамма для базового слоя
	for(int i = 0; i < 256; i++){
		float norm = (float)i / 255.0f;
		float compressed = powf(norm, gamma) * 255.0f;
		base_table[i] = (uint8_t) compressed;
	}

	// 2. Расчёт S-образной кривой для K
	for(int i = 0; i < 256; i++){
		// Сигмоида: плавно поднимает К около значения threshold
		float s_curve = 1.0f / (1.0f + expf(-0.05f * ((float)i - threshold)));
		float k_val = k_max * s_curve;

		// Переводим в формат ap_fixed<8,4> (умножаем на 16)
		int k_fixed = (int)(k_val * 16.0f);
		if (k_fixed > 255) k_fixed = 255;

		k_table[i] = (u8)k_fixed;
	}

	// 3. Запись массивов в ПЛИС k_table
	// Упаковываем 256 по 8 бит слов в 64 по 32 бита для k_table
	int packet_k_table[64];
	for(int i = 0; i < 64; i++){
		int index = i * 4;
		packet_k_table[i] = ((u32)k_table[index + 3] << 24) |
							((u32)k_table[index + 2] << 16) |
							((u32)k_table[index + 1] << 8)  |
							((u32)k_table[index + 0]);

	}

	// Упаковываем 256 по 8 бит слов в 64 по 32 бита для base_table
	int packet_base_table[64];
	for(int i = 0; i < 64; i++){
		int index = i * 4;
		packet_base_table[i] = ((u32)base_table[index + 3] << 24) |
							   ((u32)base_table[index + 2] << 16) |
							   ((u32)base_table[index + 1] << 8)  |
							   ((u32)base_table[index + 0]);

		}

	// 4. Отправка packet_k_table и packet_base_table
	XAdvanceddde_Write_K_lut_Words(&AdvancedDDEInst, 0, (word_type*)packet_k_table, 64);
	XAdvanceddde_Write_base_lut_Words(&AdvancedDDEInst, 0, (word_type*)packet_base_table, 64);

}










