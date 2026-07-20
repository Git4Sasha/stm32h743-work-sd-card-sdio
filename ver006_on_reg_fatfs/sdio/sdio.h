/*
 * sdio.h
 *
 *  Created on: 20 июн. 2026 г.
 *      Author: user
 */

#ifndef SDIO_SDIO_H_
#define SDIO_SDIO_H_


// Ошибки при работе процедуры инициализации карты
#define SDIO_Init_err_cmd0                ((int32_t)-9)
#define SDIO_Init_err_cmd8                ((int32_t)-10)
#define SDIO_Init_err_cmd55_1             ((int32_t)-11)
#define SDIO_Init_err_cmd55_Resp1         ((int32_t)-12)
#define SDIO_Init_err_cmd55_c             ((int32_t)-13)
#define SDIO_Init_err_cmd41_c             ((int32_t)-14)
#define SDIO_Init_err_R3                  ((int32_t)-15)
#define SDIO_Init_err_volt                ((int32_t)-16)
#define SDIO_Init_err_cmd2_R2             ((int32_t)-17)
#define SDIO_Init_err_GetRelAdd           ((int32_t)-18)
#define SDIO_Init_err_GetCSD              ((int32_t)-19)
#define SDIO_Init_err_SetBlockSize        ((int32_t)-20)
#define SDIO_Init_err_cmd55_cmd6          ((int32_t)-21)
#define SDIO_Init_err_cmd6_R1             ((int32_t)-22)

// Ошибки при работе процедуры SDIO_ReadSectors
#define SDIO_ReadSectors_err_cmd17        ((int32_t)-1)
#define SDIO_ReadSectors_err_cmd18        ((int32_t)-2)
#define SDIO_ReadSectors_err_StopTrans    ((int32_t)-3)

// Ошибки при работе процедуры SDIO_WriteSectors
#define SDIO_WriteSectors_err_cmd24        ((int32_t)-1)
#define SDIO_WriteSectors_err_cmd25        ((int32_t)-2)
#define SDIO_WriteSectors_err_StopTrans    ((int32_t)-3)

#define SDCARD_BLOCK_SIZE                 ((uint32_t)512)         // Размер блока в байтах для работы с SD картой
#define SD_WORK_FREQ                      ((uint32_t)48000000)     // Частота на которой будет выполняться обмен с картой после инициализации


extern uint32_t SD_BlockNumber;      // Кол-во блоков в SD карте (обычно размер блока это 512 байт, т.о. объём карты памяти это кол-во блоков * на 512 байт)
extern uint64_t SD_Size;             // Объём SD карты
extern uint32_t SD_InitOK;           // Если карта была проинициализированна, то эта переменная будет равна 1


int32_t SDIO_Init(void);                                            // Инициализация
int32_t SDIO_GetCardState(uint32_t *CardSatatus);                   // Получение состояния карты
int32_t SDIO_ReadSectors(uint32_t sn, uint32_t sc, uint8_t *buf);   // Чтение данных с карты
int32_t SDIO_WriteSectors(uint32_t sn, uint32_t sc, uint8_t *buf);  // Запись данных на карту
int32_t SDIO_WaitData0Release(uint32_t tout);                       // Ожидание освобождения линии data0 (если ниния data0 прятянута к земле (после операции чтения/записи), это значит, что sd карта занята и не может быть использована



#endif /* SDIO_SDIO_H_ */
