/*
 * SPISD.h
 *
 *  Created on: Jun 13, 2022
 *      Author: edgardog
 */

#ifndef INC_SPISD_H_
#define INC_SPISD_H_

//Agrego definiciones de la HAL
#include "stm32f1xx_hal.h"

//Defino un ENUM para llevar el estado del dispositivo
enum FSMSPISD {Encendido,sinc,version,initCheck,finInit};


//Defino una estructura para mantener todo lo relacionado
//en una unica variable
typedef struct SPISD {
	SPI_HandleTypeDef *puertoSPI;
	enum FSMSPISD FSM;
	GPIO_TypeDef *csPuerto;
	uint16_t csPin;
	uint8_t sectorAddressing;
	 RTC_HandleTypeDef *hrtc;
} SPISD;

extern SPISD *mainSD;

uint8_t SPISD_DetectarSD(SPISD *spisd);
void SPISD_EnviarComando(SPISD *spisd, uint8_t* buffer, uint16_t cantidad);
void SPISD_RecibirRespuestaComando(SPISD *spisd,uint8_t* buffer, uint16_t cantidad);
uint8_t SPISD_LeerSector(SPISD *spisd,uint32_t sector,uint8_t *buffer);
uint8_t SPISD_EscribirSector(SPISD *spisd,uint32_t sector,const uint8_t *buffer);



#endif /* INC_SPISD_H_ */
