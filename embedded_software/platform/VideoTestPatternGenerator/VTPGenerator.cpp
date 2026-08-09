// =================================================================
// Project: OV7670 Video Processing System
// File: VTPGenerator.cpp
// Description: Driver for VTPGenerator module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#include "VTPGenerator.h"

/* Реализация класса драйвера генератора тестовых шаблонов. */

// Сохраняем идентификатор аппаратного экземпляра до инициализации драйвера.
GenCore:: GenCore(uint16_t deviceID)
{
	DeviceID = deviceID;
}

// Драйвер не владеет динамической памятью, поэтому освобождать ресурсы здесь не нужно.
GenCore:: ~GenCore(){}		// Деструктор


/* Инициализация генератора шаблонов */
int GenCore::VTPinit()
{
	XV_tpg_Config *Config;
	int Status;

	/*
	 * Получаем конфигурацию, сгенерированную Vivado для выбранного
	 * аппаратного экземпляра. В ней находится, в частности, базовый адрес.
	 */
	Config = XV_tpg_LookupConfig(DeviceID);
	if (NULL == Config) {
		return XST_FAILURE;
	}

	/*
	 * Связываем программную структуру драйвера с регистровым интерфейсом
	 * генератора по базовому адресу из конфигурации.
	 */
	Status = XV_tpg_CfgInitialize(&tpg, Config, Config->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;

}

/* Установка шаблона по ID с разрешением 1080p. */
void GenCore::SetBackground_1080p(uint32_t ID)
{
/*
  XTPG_BKGND_H_RAMP = 1,
  XTPG_BKGND_V_RAMP = 2,
  XTPG_BKGND_TEMPORAL_RAMP = 3,
  XTPG_BKGND_SOLID_RED = 4,
  XTPG_BKGND_SOLID_GREEN = 5,
  XTPG_BKGND_SOLID_BLUE = 6,
  XTPG_BKGND_SOLID_BLACK = 7,
  XTPG_BKGND_SOLID_WHITE = 8,
  XTPG_BKGND_COLOR_BARS = 9,
  XTPG_BKGND_ZONE_PLATE = 10,
  XTPG_BKGND_TARTAN_COLOR_BARS = 11,
  XTPG_BKGND_CROSS_HATCH = 12,
  XTPG_BKGND_RAINBOW_COLOR = 13,
  XTPG_BKGND_HV_RAMP = 14,
  XTPG_BKGND_CHECKER_BOARD = 15,
  XTPG_BKGND_PBRS = 16,
  XTPG_BKGND_DP_COLOR_RAMP = 17,
  XTPG_BKGND_DP_BW_VERTICAL_LINE = 18,
  XTPG_BKGND_DP_COLOR_SQUARE = 19,
  XTPG_BKGND_LAST = 20
*/
	XV_tpg_Set_bckgndId(&tpg, ID);
	XV_tpg_Set_width(&tpg, 1920);					// Ширина кадра
	XV_tpg_Set_height(&tpg, 1080);					// Высота кадра
	// Автоматически перезапускаем генерацию после завершения каждого кадра.
	XV_tpg_EnableAutoRestart(&tpg);
	XV_tpg_Start(&tpg);								// Старт

}

/* Функция включения разных шаблонов с автоматическим переключением. */
void GenCore:: GenMotion(){

	// Настройки изображения должны совпадать с форматом видеопотока системы.
	XV_tpg_Set_width(&tpg, 1920);
	XV_tpg_Set_height(&tpg, 1080);

	// Параметры изменения контраста zone-plate-шаблона по горизонтали и вертикали.
	XV_tpg_Set_ZplateHorContDelta(&tpg, 2);
	XV_tpg_Set_ZplateHorContStart(&tpg, 2);
	XV_tpg_Set_ZplateVerContDelta(&tpg, 2);
	XV_tpg_Set_ZplateVerContStart(&tpg, 2);

	// Включаем движение шаблона и задаём его скорость.
	XV_tpg_Set_motionSpeed(&tpg, 2);
	XV_tpg_Set_motionEn(&tpg, 1);

	// Генератор продолжает работать без повторного запуска между кадрами.
	XV_tpg_EnableAutoRestart(&tpg);
	XV_tpg_Start(&tpg);

	// Переключаемся между шаблонами 1..19 каждые две секунды.
	int pattern = 1;

	while(true)
	{
		XV_tpg_Set_bckgndId(&tpg, pattern);

		// После последнего доступного шаблона начинаем последовательность заново.
		if(++pattern > 19)
		{
			pattern = 1;
		}

		usleep(2000000);
	}
}
