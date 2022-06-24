/*
 * Globals.h
 *
 *  Created on: Jun 14, 2022
 *      Author: edgardog
 */

#ifndef INC_GLOBALS_H_
#define INC_GLOBALS_H_

#include "stm32f1xx_hal.h"
#include "fatfs.h"
extern RTC_HandleTypeDef *pRTC;

//Archivos SD
extern uint8_t retUSER;
extern char USERPath[4];
extern FATFS USERFatFS;
extern FIL USERFile;

#endif /* INC_GLOBALS_H_ */
