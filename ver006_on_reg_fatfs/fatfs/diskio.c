/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2025        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Basic definitions of FatFs */
#include "diskio.h"		/* Declarations FatFs MAI */

/* Example: Declarations of the platform and disk functions in the project */
#include "sdio.h"

#include "usart1.h"


// Во всех функциях есть параметр pdrv - физический диск, если бы дисков было бы несколько, то этот параметр как раз и указывал бы, для какого конкретно диска была запущена функция
// Если диск в системе один, то этот параметр можно не обрабатвать


/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (BYTE pdrv)				/* Physical drive nmuber to identify the drive */
{
	int result;

	// Тут нужно вызвать инициализацию SD карты

	if(SD_InitOK) return 0;        // Если инициализация уже проходила, то возвращаем 0

	result = SDIO_Init();
	if(result) return STA_NOINIT;

	return 0; // Если проблем с инициализацией нет, то возвращаем 0
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (BYTE pdrv)   /* Physical drive nmuber to identify the drive */
{
  int result;
  uint32_t dst;

  // Нужно вернуть какое-то состояние диска
  // Наверное стоит тут вызывать процедуру SDIO_GetCardState
  if(!SD_InitOK) return STA_NOINIT;

  result = SDIO_GetCardState(&dst);
  if(result) return STA_NOINIT;

  if(dst & 1U<<8) return 0;

  return 0;
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
  uint32_t dst;
	int result;

	result = SDIO_ReadSectors(sector, count, buff);
	if(result)
	  return RES_ERROR;
	else
	{
	  result = SDIO_WaitData0Release(0xFFFFFFFF);   // Ожидание освобождения линии data0, что означает, что текущая операция полностью завершена и sd карта может обрабатывать новые команды
    if(result) return RES_ERROR;
	  return RES_OK;
	}
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
  uint32_t dst;
	int result;

  result = SDIO_WriteSectors(sector, count, (uint8_t*)buff);
  if(result)
    return RES_ERROR;
  else
  {
    result = SDIO_WaitData0Release(0xFFFFFFFF);   // Ожидание освобождения линии data0, что означает, что текущая операция полностью завершена и sd карта может обрабатывать новые команды
    if(result) return RES_ERROR;
    return RES_OK;
  }
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
  if (!SD_InitOK) return RES_NOTRDY;  // Если SD карта не была проинициализированна, то возвращаем такой результат

  switch (cmd)
  {
    /* Make sure that no pending write process */
    case CTRL_SYNC :
                    return RES_OK;

    /* Get number of sectors on the disk (DWORD) */
    case GET_SECTOR_COUNT :
                    *(DWORD*)buff = SD_BlockNumber;
                    return RES_OK;

    /* Get R/W sector size (WORD) */
    case GET_SECTOR_SIZE :
                        *(WORD*)buff = SDCARD_BLOCK_SIZE;
                        return RES_OK;

    /* Get erase block size in unit of sector (DWORD) */
    case GET_BLOCK_SIZE :
                        *(DWORD*)buff = 1;
                        return RES_OK;
    case GET_SD_SIZE :
                        *(uint64_t*)buff = SD_Size;
                        return RES_OK;
  }

  return RES_PARERR;
}

/**
  * @brief  Gets Time from RTC
  * @param  None
  * @retval Time in DWORD
  */
__attribute__((weak)) DWORD get_fattime (void)
{
  // return 0; Можно возвращать и 0, но тогда все файлы будут с датой 01.01.1980

//  Бит 31..25 — Год (от 1980): 0..127 → годы 1980..2107
//  Бит 24..21 — Месяц: 1..12
//  Бит 20..16 — День месяца: 1..31
//  Бит 15..11 — Час: 0..23
//  Бит 10..5  — Минута: 0..59
//  Бит 4..0   — Секунда / 2: 0..29 → секунды 0..58 (шаг 2 секунды)

  // 2026-07-18 23:43:00
  return ((DWORD)(2026 - 1980) << 25) | (7 << 21) | (18 << 16) | (23<<11) | (43<<5);
}


