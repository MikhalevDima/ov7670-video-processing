// =================================================================
// Project: OV7670 Video Processing System
// File: VideoMixer.hpp
// Description: Driver for VideoMixer module
// Developer: MikhalevDima
// Year: 2026
// =================================================================

#ifndef VIDEOMIXER_H
#define VIDEOMIXER_H

#include "xparameters.h"
#include "xv_mix_l2.h"
#include "xvidc.h"

/* Объявление прототипа класса VideoMixer */

class VideoMixer {

public:

	//Конструктор
	VideoMixer(uint16_t deviceID);

	//Деструктор
	~VideoMixer();

	//Инициализация модуля
	int IntVideoMixer();

	//Layer1
	int SetLayer1_640x480();

	//Запуск миксера
	void StartMixer();

	//Стоп миксера
	void StopMixer();

private:
	uint16_t DeviceID;								// Идентификатор модуля в xparameters
	XV_Mix_l2 VideoMix;								// Экземпляр модуля
};


#endif /// VIDEOMIXER_H
