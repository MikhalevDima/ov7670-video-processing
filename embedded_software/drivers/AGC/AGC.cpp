// =================================================================
// Project: OV7670 Video Processing System
// File: AGC.cpp
// Description: Driver for AGC module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#include "AGC.hpp"

// Реализация функций уласса AGC

// Конструктор
AGC::AGC(uint16_t deviceID)
	: DevaiceID(deviceID)
	{
		Gain_old = 1.0f; // Стартуем с нейтрального усиления = 1.0
	};

// Деструктор
AGC::~AGC() {};

// Инициализация модуля
int AGC::InitAGC()
{
	int Status;
	// Инициализация модуля
	Status = XAgc_Initialize(&AGCInst, DevaiceID);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}

	// Сразу прописываем в HLS начальное значение 1.0 (1.0 * 4096 = 4096)
	XAgc_Set_gain(&AGCInst, 4096);

	return XST_SUCCESS;
}

// Старт модуля
void AGC::StartAGC()
{
	XAgc_Start(&AGCInst);
	XAgc_EnableAutoRestart(&AGCInst);
}

// Установления коэффициента усиления контрастности
void AGC::SetGain(int Disp_cur, int Disp_target, float u_attack, float u_release)
{
    // Защита от тишины / темноты (Noise Gate)
    if (Disp_cur < 5) {
        return;
    }

    // 1. Вычисляем отношение
    float ratio = (float)Disp_target / (float)Disp_cur;

    // 2. Нелинейность АРУ: выбираем скорость адаптации
    // Если ratio < 1.0, значит текущая дисперсия больше целевой (кадр слишком яркий) -> АТАКА
    // Если ratio > 1.0, значит кадр темный -> СПАД (RELEASE)
    float u = (ratio < 1.0f) ? u_attack : u_release;

    // 3. Расчёт нового коэффициента усиления во float
    float Gain_float = Gain_old * (1.0f + u * (ratio - 1.0f));

    // 4. Ограничение коэффициента усиления (во float диапазоне от 0.8 до 2.0)
    if (Gain_float < 0.8f) Gain_float = 0.8f;
    if (Gain_float > 2.0f) Gain_float = 2.0f;

    // Сохраняем текущее значение для следующего шага
    Gain_old = Gain_float;

    // 5. Переводим в фиксированную точку для HLS (умножаем на 4096)
    // ap_ufixed<16,4> ожидает, что 1.0 превратится в 4096
    uint32_t Gain_hls = (uint32_t)(Gain_float * 4096.0f);

    // 6. Передача коэффициента усиления в HLS
    XAgc_Set_gain(&AGCInst, Gain_hls);

}
