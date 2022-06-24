/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file   fatfs.c
  * @brief  Code for fatfs applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#include "Globals.h"
/* USER CODE END Header */
#include "fatfs.h"

uint8_t retUSER;    /* Return value for USER */
char USERPath[4];   /* USER logical drive path */
FATFS USERFatFS;    /* File system object for USER logical drive */
FIL USERFile;       /* File object for USER */

/* USER CODE BEGIN Variables */

/* USER CODE END Variables */

void MX_FATFS_Init(void)
{
  /*## FatFS: Link the USER driver ###########################*/
  retUSER = FATFS_LinkDriver(&USER_Driver, USERPath);

  /* USER CODE BEGIN Init */
  /* additional user code for init */
/*  f_mount(&USERFatFS,USERPath,0);
  f_open(&USERFile,"Archivo.txt",FA_CREATE_ALWAYS | FA_WRITE);
  uint32_t output;
  if (f_write(&USERFile,"Hola Mundo SD Card!",sizeof("Hola Mundo SD Card!"),(void*)&output)==FR_OK)
  {
	  if (f_sync(&USERFile)==FR_OK){
		  f_close(&USERFile);
	  }


  }*/
  /* USER CODE END Init */
}

/**
  * @brief  Gets Time from RTC
  * @param  None
  * @retval Time in DWORD
  */
DWORD get_fattime(void)
{
  /* USER CODE BEGIN get_fattime */
	DWORD time=0;
	RTC_TimeTypeDef sTime = {0};
	RTC_DateTypeDef DateToUpdate = {0};
	HAL_RTC_GetTime(pRTC, &sTime, RTC_FORMAT_BCD);
	HAL_RTC_GetDate(pRTC, &DateToUpdate, RTC_FORMAT_BCD);
	uint8_t year = 2000+((DateToUpdate.Year>>4)&0x0F)*10+(DateToUpdate.Year&0x0F);
	year = year-1980;
	uint8_t month = ((DateToUpdate.Month>>4)&0x0F)*10+(DateToUpdate.Month&0x0F);
	uint8_t day = ((DateToUpdate.Date>>4)&0x0F)*10+(DateToUpdate.Date&0x0F);
	uint8_t hour = ((sTime.Hours>>4)&0x0F)*10+(sTime.Hours&0x0F);
	uint8_t minute = ((sTime.Minutes>>4)&0x0F)*10+(sTime.Minutes&0x0F);
	uint8_t second = ((sTime.Seconds>>4)&0x0F)*10+(sTime.Seconds&0x0F);

	time= year <<25 |
			month<< 21 |
			day << 16 |
			hour << 11 |
			minute << 5 |
			second >> 1;
  return time;
  /* USER CODE END get_fattime */
}

/* USER CODE BEGIN Application */

/* USER CODE END Application */
