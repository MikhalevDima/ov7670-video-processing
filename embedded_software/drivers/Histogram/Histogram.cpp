// =================================================================
// Project: OV7670 Video Processing System
// File: Histogram.cpp
// Description: Driver for Histogram module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#include "Histogram.hpp"

// Реализация методов класса Histogram

// Конструктор
Histogram::Histogram(uint16_t deviceID)
			: DevaiceID(deviceID) {};

// Деструктор
Histogram::~Histogram() {};

// Инициализация модуля
int Histogram::IntHistogram()
{
	int Status;
	// Инициализация модуля
	Status = XHistogram_Initialize(&HistogramInst, DevaiceID);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	return XST_SUCCESS;
}

// Старт модуля
void Histogram::StartHistogram()
{
	XHistogram_Start(&HistogramInst);
	XHistogram_EnableAutoRestart(&HistogramInst);
}

// Стоп модуля
void Histogram::StopHistogram()
{
	XHistogram_DisableAutoRestart(&HistogramInst);
}


// Получение гистограммы из модуля, в hist_buffer
void Histogram::ReadHistData()
{
	StopHistogram();
	while(!XHistogram_IsDone(&HistogramInst));

	XHistogram_Read_hist_Words(&HistogramInst, 0, hist_buffer ,256);		// Читаем гистограмму в буфер

	StartHistogram();
}


// Минимальное значение, первый индекс слева
int Histogram::MinHist(long unsigned int *buffer, long unsigned int threshold)
{
	 // Проходим по гистограмме СЛЕВА НАПРАВО (от 0 до 255)
	for (int i = 0; i < HIST_SIZE; i++) {

		// Если количество пикселей в текущей ячейке больше порога
		if (buffer[i] > threshold) {
			return i; // Возвращаем индекс (номер цвета) и выходим из функции
		}
	}

	return -1; // Если прошли всю гистограмму и ничего не нашли выше порога
}

// Максимальное значение, первый индекс справа
int Histogram::MaxHist(long unsigned int *buffer, long unsigned int threshold)
{
	 // Проходим по гистограмме Справо на лево (от 255 до 0)
	for (int i = HIST_SIZE-1; i > 0; i--) {

		// Если количество пикселей в текущей ячейке больше порога
		if (buffer[i] > threshold) {
			return i; // Возвращаем индекс (номер цвета) и выходим из функции
		}
	}

	return -1; // Если прошли всю гистограмму и ничего не нашли выше порога
}


// Ширина гистограммы
int Histogram::WidthHist(long unsigned int *buffer, long unsigned int threshold_left, long unsigned int threshold_right)
{
	// Левая граница
	int Left = MinHist(buffer, threshold_left);

	// Правая граница
	int Right = MaxHist(buffer, threshold_right);

	// Ширина
	int Width = Right - Left;

	return Width;
}


/* Статистика */

//  Аккумулятор пикселей
long unsigned int Histogram::Accumulator()
{
	 long unsigned int accumulator = 0;

	// Начинаем с i = 1, так как i = 0 всё равно даст 0
	for (int i = 1; i < HIST_SIZE; i++)
	{

		accumulator += (hist_buffer[i] * i);
	}

	return accumulator;
}

// Математическое ожидание (Средняя яркость)
int Histogram::MathMeanHist()
{

    long unsigned int accumulator = Accumulator();

    int Math = accumulator / TOTAL_PIXEL;

    return Math;
}


// Дисперсия
int Histogram::DispHist()
{
    // 1. Получаем уже вычисленное математическое ожидание (среднюю яркость)
    int Math = MathMeanHist();

    // 2. Объявляем большой аккумулятор для промежуточной суммы.
    long unsigned int accumulator = 0;

    // 3. Цикл строго по размеру гистограммы (256 ячеек)
    for (int i = 0; i < HIST_SIZE; i++)
    {
        // Вычисляем отклонение текущего цвета от среднего значения
        int deviation = i - Math;

        // Возводим отклонение в квадрат
        long unsigned int deviation_sq = (long unsigned int)(deviation * deviation);

        // Умножаем на количество пикселей в текущей ячейке и копим сумму
        accumulator += deviation_sq * hist_buffer[i];
    }

    // 4. Делаем
    int Disp = accumulator / TOTAL_PIXEL;

    return Disp;
}


/* Вывод на печать */

// Печать гистограммы в printf
void Histogram::PrintHistogram(long unsigned int *buffer)
{
	 // Переносим курсор в начало строки
	xil_printf("\r");

	// Печатаем каждое 10-е значение гистограммы (индексы: 0, 10, 20, 30...)
	for (int i = 0; i < 256; i += 20) {
		// Формат %-6lu означает: вывести long unsigned int,
		// выровнять по левому краю и выделить строго 6 символов под число.
		// Выделение фиксированной ширины (6 мест)
		xil_printf("[%3d]:%-6lu ", i, buffer[i]);
	}

	// Принудительно выталкиваем данные в UART
	fflush(stdout);

	usleep(100000);
}

// Печать минимального значения
void Histogram::PrintMinHist(long unsigned int *buffer)
{
	// Поднимаем порог до 500 пикселей (настройте под свое видео)
	int left_index = MinHist(buffer, 500);

	// Возвращаем курсор в начало строки
	xil_printf("\r");

	// Выводим статус НА АНГЛИЙСКОМ, чтобы не было кракозябр
	if (left_index != -1) {
		xil_printf("Left Threshold found at index: [%3d]:%-6d", left_index, buffer[left_index]);
	} else {
		xil_printf("Left Threshold NOT found!");
	}

	// Даем задержку для терминала
	usleep(100000);
}

// Печать максимальное значения
void Histogram::PrintMaxHist(long unsigned int *buffer)

{
	// Поднимаем порог до 500 пикселей
	int right_index = MaxHist(buffer, 500);

	// Возвращаем курсор в начало строки
	xil_printf("\r");

	// Выводим статус НА АНГЛИЙСКОМ, чтобы не было кракозябр
	if (right_index != -1) {
		xil_printf("Right Threshold found at index: [%3d]:%-6d", right_index, buffer[right_index]);
	} else {
		xil_printf("Right Threshold NOT found!");
	}

	// Даем задержку для терминала
	usleep(100000);
}


// Печать ширины гистограммы
void Histogram::PrintWidhtHist(long unsigned int *buffer)
{
	// Считаем ширину гистограммы
	int Widht = WidthHist(buffer, 100, 100);

	// Возвращаем курсор в начало строки
	xil_printf("\r");

	// Выводим ширину гистограммы
	xil_printf("Widht histogram: [%3d]", Widht);

	// Даем задержку для терминала
	usleep(100000);

}

// Вывод на печать математическое ожидание
void Histogram::PrintMathMeanHist()
{
	// Считаем математическое ожидание
	int Math = MathMeanHist();

	// Возвращаем курсор в начало строки
	xil_printf("\r");

	// Выводим математическое ожидание в консоль
	xil_printf("MathMean: [%3d]", Math);

	// Даем задержку для терминала
	usleep(100000);
}

// Вывод на печать дисперсии
void Histogram::PrintDiapHist()
{
	// Считаем математическое ожидание
	int Disp = DispHist();

	// Возвращаем курсор в начало строки
	xil_printf("\r");

	// Выводим математическое ожидание в консоль
	xil_printf("Disp: [%3d]", Disp);

	// Даем задержку для терминала
	usleep(100000);
}








