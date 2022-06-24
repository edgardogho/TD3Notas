#include "SPISD.h"

//Defino los mensajes
// Al generar codigo (y no definiciones) deben declararse en un .c y no en el .h
uint8_t SPISD_INITSEQ[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF };
uint8_t SPISD_CMD0[] = { 0x40, 0x0, 0x0, 0x0, 0x0, 0x95, 0xFF };
uint8_t SPISD_CMD8[] = { 0x48, 0x0, 0x0, 0x1, 0xAA, 0x87, 0xFF };
uint8_t SPISD_CMD55[] = { 0x77, 0x00, 0x0, 0x0, 0x00, 0x00, 0xFF };
uint8_t SPISD_CMD41[] = { 0x69, 0x40, 0x0, 0x0, 0x00, 0x00, 0xFF };
uint8_t SPISD_CMD58[] = { 0x7A, 0x0, 0x0, 0x0, 0x0, 0x75, 0xFF };
uint8_t SPISD_CMD51[] = { 0x73, 0x0, 0x0, 0x0, 0x0, 0x75, 0xFF };
uint8_t SPISD_CMD17[] = { 0x51, 0x0, 0x0, 0x0, 0x0, 0x00, 0xFF };
uint8_t SPISD_CMD24[] = { 0x58, 0x0, 0x0, 0x0, 0x0, 0x00, 0xFF };

//Declaro un buffer para recibir valores por SPI
//Esto es lugar temporario para procesar respuestas
uint8_t recbuffer[8];

/**
 * Escribe un sector en la SD
 * En el caso de direccionar al byte, revisa si SectorAddressing es 0
 * y multiplica el sector por 512 para apuntar al byte.
 */
uint8_t SPISD_EscribirSector(SPISD *spisd, uint32_t sector,
		const uint8_t *buffer) {
	//Verificar que la memoria SD soporte direccionamiento al sector y no al byte.
	if (!spisd->sectorAddressing)
		sector = sector * 512;

	//El sector es uint32, pero hay que enviarlo en 4 bytes en SPISD_CMD24
	//Lo partimos en 4 partes con shifts y mascaras.
	SPISD_CMD24[4] = sector & 0xff;
	SPISD_CMD24[3] = (sector & 0xff00) >> 8;
	SPISD_CMD24[2] = (sector & 0xff0000) >> 16;
	SPISD_CMD24[1] = (sector & 0xff000000) >> 24;
	//Enviamos el comando CMD24
	SPISD_EnviarComando(spisd, SPISD_CMD24, sizeof(SPISD_CMD24));
	HAL_Delay(1);
	uint8_t timeout = 10;
	recbuffer[0] = 0xFF;
	while (timeout > 0) {
		SPISD_RecibirRespuestaComando(spisd, recbuffer, 4);
		//Si la respuesta en 0, todo marcha bien...
		if (recbuffer[0]==0)
			break;
		//Si no es 0
		timeout--;
		//Si paso 5 veces y no responde, reiniciamos la SD
		if (timeout == 5) {
			if (!SPISD_DetectarSD(spisd))
				return 0; // Falla la SD
		}
		HAL_Delay(20);
		SPISD_EnviarComando(spisd, SPISD_CMD24, sizeof(SPISD_CMD24));
		HAL_Delay(2);
	}
	if (recbuffer[0] != 0)
		return 0; //Fallo escribiendo.
	HAL_Delay(1);
	uint8_t outputBuffer[515]; //Armamos un buffer con start, datos y CRC (mentiroso)
	outputBuffer[0] = 0xFE; //Start
	for (int i = 0; i < 512; i++) {
		outputBuffer[i + 1] = buffer[i];
	}
	outputBuffer[513] = 0xFF; //CRC mentiroso1
	outputBuffer[514] = 0xFF; //CRC mentiroso2
	HAL_GPIO_WritePin(spisd->csPuerto, spisd->csPin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(spisd->puertoSPI, outputBuffer, 515, 100);
	HAL_GPIO_WritePin(spisd->csPuerto, spisd->csPin, GPIO_PIN_SET);
	SPISD_RecibirRespuestaComando(spisd, recbuffer, 2);
	recbuffer[0] = recbuffer[0] & 0x1F; //Mascara para los 5 bits menos significativos
	//Debe devolver xxx00101 si el sector fue escrito correctamente.
	if (recbuffer[0] != 0x05) {
		return 0; //Error
	}
	return 1;
}

/**
 * Lee un sector de la SD. La misma debe estar inicializada
 *
 */
uint8_t SPISD_LeerSector(SPISD *spisd, uint32_t sector, uint8_t *buffer) {
	//Si la memoria direcciona al byte, multiplicamos sector*512.
	if (!spisd->sectorAddressing)
		sector = sector * 512;
	//Pasamos el sector/byte a 4 bytes
	SPISD_CMD17[4] = sector & 0xff;
	SPISD_CMD17[3] = (sector & 0xff00) >> 8;
	SPISD_CMD17[2] = (sector & 0xff0000) >> 16;
	SPISD_CMD17[1] = (sector & 0xff000000) >> 24;
	//Comando 17, leer sector (o byte).
	SPISD_EnviarComando(spisd, SPISD_CMD17, sizeof(SPISD_CMD17));
	//Ahora la SD deberia responder on 0x00 indicando que esta lista
	//para enviar el sector.. pero esto puede tardar...
	uint8_t timeout = 10;
	while (timeout > 0) {
		//Busco la respuesta
		SPISD_RecibirRespuestaComando(spisd, recbuffer, 1);
		if (recbuffer[0] == 0)
			break; //Todo funciono bien
		//Tengo que mandar el comando de vuelta.
		timeout--;
		//A veces queda en IDLE, asi que si timeout es 5, mandamos
		//de nuevo la secuencia de init
		if (timeout == 5) {
			if (!SPISD_DetectarSD(spisd))
				return 0;
			HAL_Delay(100);
		}
		HAL_Delay(20);
		SPISD_EnviarComando(spisd, SPISD_CMD17, sizeof(SPISD_CMD17));
		HAL_Delay(2);

	}
	//Pude haber salido por timeout o porque recbuffer[0]==0
	if (recbuffer[0] != 0)
		return 0; //No responde mas a comandos.

	//Si la memoria responde 0 a CMD17, entonces va a comenzar a enviar el sector
	//El comienzo del sector es siempre 0xFE, pero depende la memoria, puede responder
	//con una cantidad de 0xFF primero. Leemos de a uno hasta que llega el primer 0xFF
	recbuffer[0] = 0xFF;
	timeout = 100; //Maximo 100 lecturas
	HAL_GPIO_WritePin(spisd->csPuerto, spisd->csPin, GPIO_PIN_RESET);
	while (timeout > 0) {
		HAL_SPI_Receive(spisd->puertoSPI, recbuffer, 1, 100);
		//SPISD_RecibirRespuestaComando(spisd,recbuffer,1);
		if (recbuffer[0] == 0xFE) {
			break;
		}
		timeout--; //Si no hubo break, decrementamos el timeout
	}

	//Si recbuffer[0] NO es 0xFE, levantamos CS y chau, la SD no responde.
	if (recbuffer[0] != 0xFE) {
		HAL_GPIO_WritePin(spisd->csPuerto, spisd->csPin, GPIO_PIN_SET); //Subo CS
		return 0;
	}
	//Si recibimos el 0xFE, entonces podemos recibir los 512+2 bytes (2 de CRC).
	HAL_SPI_Receive(spisd->puertoSPI, buffer, 512, 200);
	//Ahora falta el CRC
	recbuffer[0] = 0xFF;
	recbuffer[1] = 0xFF;
	HAL_SPI_Receive(spisd->puertoSPI, recbuffer, 2, 200);
	HAL_GPIO_WritePin(spisd->csPuerto, spisd->csPin, GPIO_PIN_SET);
	return 1;
}

/**
 * Envia comando a la SD controlando CS.
 */
void SPISD_EnviarComando(SPISD *spisd, uint8_t *buffer, uint16_t cantidad) {
	HAL_GPIO_WritePin(spisd->csPuerto, spisd->csPin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(spisd->puertoSPI, buffer, cantidad, 200);
	HAL_GPIO_WritePin(spisd->csPuerto, spisd->csPin, GPIO_PIN_SET);
}

/**
 * Recibe por SPI una cantidad de bytes de forma bloqueante.
 * Se asegura que la linea (MOSI) este en 1 mientras se recibe.
 */
void SPISD_RecibirRespuestaComando(SPISD *spisd, uint8_t *buffer,
		uint16_t cantidad) {
	//Cuando se recibe, la linea debe quedar en 1, por ende ponemos 0xFF en cada byte
	//que se quiera transmitir.
	for (int i = 0; i < cantidad; i++) {
		buffer[i] = 0xFF;
	}
	HAL_GPIO_WritePin(spisd->csPuerto, spisd->csPin, GPIO_PIN_RESET);
	HAL_SPI_Receive(spisd->puertoSPI, buffer, cantidad, 200);
	HAL_GPIO_WritePin(spisd->csPuerto, spisd->csPin, GPIO_PIN_SET);
}

/**
 * Ejecuta la secuencia de inicializacion. Detecta el tipo de tarjeta
 * y deja esa info en la estructura.
 */
uint8_t SPISD_DetectarSD(SPISD *spisd) {
	//La SD recien insertada se encuentra en modo SD
	//Debemos enviar al menos 72 clocks con CS en 1
	spisd->FSM = Encendido;
	HAL_GPIO_WritePin(spisd->csPuerto, spisd->csPin, GPIO_PIN_SET);
	HAL_SPI_Transmit(spisd->puertoSPI, SPISD_INITSEQ, sizeof(SPISD_INITSEQ),
			100);
	HAL_GPIO_WritePin(spisd->csPuerto, spisd->csPin, GPIO_PIN_RESET);
	//Ahora debemos enviar CMD0 (Software Reset)
	SPISD_EnviarComando(spisd, SPISD_CMD0, sizeof(SPISD_CMD0));
	SPISD_RecibirRespuestaComando(spisd, recbuffer, 1);
	if (recbuffer[0] != 0x01) {
		// Hubo un error, la SD no responde 0x01 al sw reset
		// Hacemos otro intento antes de desistir...
		HAL_GPIO_WritePin(spisd->csPuerto, spisd->csPin, GPIO_PIN_SET);
		HAL_SPI_Transmit(spisd->puertoSPI, SPISD_INITSEQ, sizeof(SPISD_INITSEQ),
				100);
		HAL_GPIO_WritePin(spisd->csPuerto, spisd->csPin, GPIO_PIN_RESET);
		//Ahora debemos enviar CMD0 (Software Reset)
		SPISD_EnviarComando(spisd, SPISD_CMD0, sizeof(SPISD_CMD0));
		SPISD_RecibirRespuestaComando(spisd, recbuffer, 1);

		if (recbuffer[0] != 0x01)
			return 0; //Error en la SD, no responde.
	}
	spisd->FSM = sinc;
	//Ahora mandamos CMD8 (Check Voltage Range)
	SPISD_EnviarComando(spisd, SPISD_CMD8, sizeof(SPISD_CMD8));
	SPISD_RecibirRespuestaComando(spisd, recbuffer, 6);
	if (recbuffer[0] != 0x01) {
		// Hubo un error, la SD no responde 0x01 al check voltage range
		return 0;
	}
	spisd->FSM = version;

	//Ahora hay que mandar CMD55 y CMD41 (lo que genera ACMD41).
	//Este par va a devolver valores, pero en algun momento debe
	//devolver 0
	uint8_t reintento = 3;
	uint8_t encontrado = 0;
	while (reintento >= 0) {
		reintento--;
		SPISD_EnviarComando(spisd, SPISD_CMD55, sizeof(SPISD_CMD55));
		//Siempre a CMD55 debe responder 0x01.
		SPISD_RecibirRespuestaComando(spisd, recbuffer, 6);
		if (recbuffer[0] != 0x01) {

			return 0;
		}
		//Mando CMD41 lo cual genera ACMD41.
		SPISD_EnviarComando(spisd, SPISD_CMD41, sizeof(SPISD_CMD41));
		SPISD_RecibirRespuestaComando(spisd, recbuffer, 6);
		//Si la respuesta en 0x00, podemos continuar
		if (recbuffer[0] == 0x00) {
			encontrado = 1;
			break;
		}
		//Sino, demoramos 100ms y volvemos a probar.
		HAL_Delay(100);
	}
	if (encontrado) {
		spisd->FSM = initCheck;
	} else {
		return 0;
	}
	//Enviamos Comando 58 , para saber que tipo de memoria SD tenemos.
	SPISD_EnviarComando(spisd, SPISD_CMD58, sizeof(SPISD_CMD58));
	SPISD_RecibirRespuestaComando(spisd, recbuffer, 5);
	if (recbuffer[0] != 0x00) {
		// Fallo el CMD58
		return 0;
	}
	//Este comando informa si la SD es HC
	if ((recbuffer[1] & 0x40)) {
		//La SD direcciona al sector (SDHC)
		spisd->sectorAddressing = 1;
	} else {
		//La SD direcciona al byte (SD)
		spisd->sectorAddressing = 0;
	}
	return 1;

}
