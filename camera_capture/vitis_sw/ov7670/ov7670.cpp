#include "ov7670.hpp"


// Конструктор
ov7670::ov7670(I2C& I2C_interface, uint8_t ADDR_uint, uint32_t ADDR_base)
	: i2c_inter(I2C_interface), addr_uint(ADDR_uint), addr_base(ADDR_base)
{
	// Выключаем IP модуль
	Stop();
}

// Деструктор
ov7670::~ov7670()
{

}

// Инициализация камеры ov7670
void ov7670::Init_YUV_VGA()
{

	xil_printf("Initializing ov7670...\r\n");

	uint8_t  i2c_data_write[2] = {0};

	int Status;								// Статус для проверки правильности отправленных данных

	// 1. Сброс камеры
	i2c_data_write[0] = 0x12;
	i2c_data_write[1] = 0x80;

	// Отправляем адрес регистра и команды
	i2c_inter.MasterSentPolled(i2c_data_write, 2, addr_uint);

	 usleep(200000); // Пауза 200 мс для инициализации камеры

	// 2. Запись конфигурации

	for(int i = 1; i < int(sizeof(OV7670_YUV_VGA)/sizeof(reg_val)); i++)	// Пропускаем первый элемент reset
	{
		if(OV7670_YUV_VGA[i].reg == 0xFF) break;
		else {
			i2c_data_write[0] = OV7670_YUV_VGA[i].reg;
			i2c_data_write[1] = OV7670_YUV_VGA[i].val;

			// Проверка занятости шины
			while(i2c_inter.IsBusy_i2c());
			// Отправляем адрес регистра и команды
			i2c_inter.MasterSentPolled(i2c_data_write, 2, addr_uint);

		}
		usleep(20000);			// 20ms задержка
	}


	xil_printf("Initializing ov7670 finish.\r\n");
}

// Старт работы модуля IP ov7670
void ov7670::Start()
{
    // Читаем текущее состояние регистра управления
    uint32_t current_reg = OV7670_mReadReg(addr_base, CONTROL_REG);

	// Включаем IP модуль
	OV7670_mWriteReg(addr_base, CONTROL_REG, current_reg | ENABLE);
}


// Стоп работы модуля IP ov7670
void ov7670::Stop()
{
		// Выключаем IP модуль
		OV7670_mWriteReg(addr_base, CONTROL_REG, DISABLE);
}

// Включаем монохромный режим (ставим Бит 1 в единицу, сохраняя Бит 0)
void ov7670::Monohrome_Enable()
{
    // Читаем текущее состояние регистра управления
    uint32_t current_reg = OV7670_mReadReg(addr_base, CONTROL_REG);
    // Применяем побитовое ИЛИ, чтобы поднять нужный флаг
    OV7670_mWriteReg(addr_base, CONTROL_REG, current_reg | MONOHROME_ENABLE);
}

// Выключаем монохромный режим (сбрасываем Бит 1 в ноль, сохраняя Бит 0)
void ov7670::Monohrome_Disable()
{
    // Читаем текущее состояние регистра управления
    uint32_t current_reg = OV7670_mReadReg(addr_base, CONTROL_REG);
    // Применяем побитовое И-НЕ, чтобы занулить только Бит 1
    OV7670_mWriteReg(addr_base, CONTROL_REG, current_reg & ~MONOHROME_ENABLE);
}









