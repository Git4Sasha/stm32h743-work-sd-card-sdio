/*
 * main.c
 *
 *  Created on: 13 мар. 2024 г.
 *      Author: user
 */

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "stm32h743xx.h"
#include "system_stm32h7xx.h"
#include "systimer.h"
#include "gpio.h"
#include "usart1.h"
#include "sdio.h"
#include "ff.h"

static void SDMMC_SDCard_Test(void);
static void SDMMC_SDCard_WriteBigFile(uint32_t len);    // Тест на запись файла большого объёма

FATFS FatFs;
FIL Fil;
FRESULT FR_Status;
FATFS *FS_Ptr;
UINT RWC, WWC; // Read/Write Word Counter


uint8_t buf4file[8192];

int main(void)
{
  int i=0,pwrc=0;

  // Настройка питания (тонкость в том, что на ядро можно подавать меньшее напряжение, если частота снижена, соответственно будет и меньше потребление)
  pwrc = PWRConfigSupply();

  i = SetMyClockFreqency();

  SystemCoreClockUpdate();

  //SET_BIT(i, 1);     эти определения находятся в stm32h743xx.h (там есть и другие)
  //CLEAR_BIT(i, 2);

  SysTimerInit(1000);

  GPIOConfig(GPIOA, 1, MODE_DIGITAL_Out, OTYPE_PUSH_PULL, SPEED_LOW, PULL_NO_UP_NO_DOWN, 0);
  GPIOLow(GPIOA, 1);

  USART1_Init(1000000, 0);

  USART1_printf("\n");
  USART1_printf("PWRConfigSupply=%d\n", pwrc);
  USART1_printf("SetMyClockFreqency=%d\n", i);

  USART1_printf("SystemCoreClock=%d\n", SystemCoreClock);
  USART1_printf("SysClkD2=%d\n", SystemD2Clock);
  USART1_printf("SysClkD1=%d\n", SysClkD1C);
  USART1_printf("SysClk4Bus=%d\n", SysClk4Bus);
  USART1_printf("APB1Freq=%d\n", APB1Freq);
  USART1_printf("APB2Freq=%d\n", APB2Freq);

  USART1_printf("CommonPLLSFreq = %d\n", CommonPLLSFreq);        // Частота, которая подаётся на все умножители PLL1, PLL2, PLL3
  USART1_printf("PLL1InFreq     = %d\n", PLL1InFreq    );        // Входная частота для множителя PLL1 (эта частота потом будет умножаться на N1 и делиться на P1, Q1, R1)
  USART1_printf("PLL2InFreq     = %d\n", PLL2InFreq    );        // Входная частота для множителя PLL2 (эта частота потом будет умножаться на N2 и делиться на P1, Q1, R1)
  USART1_printf("PLL3InFreq     = %d\n", PLL3InFreq    );        // Входная частота для множителя PLL3 (эта частота потом будет умножаться на N3 и делиться на P1, Q1, R1)
  USART1_printf("PLL1PFreq      = %d\n", PLL1PFreq     );        // Частота на выходе блока PLL1, которая получается после дилителя DIVP1
  USART1_printf("PLL1QFreq      = %d\n", PLL1QFreq     );        // Частота на выходе блока PLL1, которая получается после дилителя DIVQ1
  USART1_printf("PLL1RFreq      = %d\n", PLL1RFreq     );        // Частота на выходе блока PLL1, которая получается после дилителя DIVR1
  USART1_printf("PLL2PFreq      = %d\n", PLL2PFreq     );        // Частота на выходе блока PLL2, которая получается после дилителя DIVP2
  USART1_printf("PLL2QFreq      = %d\n", PLL2QFreq     );        // Частота на выходе блока PLL2, которая получается после дилителя DIVQ2
  USART1_printf("PLL2RFreq      = %d\n", PLL2RFreq     );        // Частота на выходе блока PLL2, которая получается после дилителя DIVR2
  USART1_printf("PLL3PFreq      = %d\n", PLL3PFreq     );        // Частота на выходе блока PLL3, которая получается после дилителя DIVP3
  USART1_printf("PLL3QFreq      = %d\n", PLL3QFreq     );        // Частота на выходе блока PLL3, которая получается после дилителя DIVQ3
  USART1_printf("PLL3RFreq      = %d\n", PLL3RFreq     );        // Частота на выходе блока PLL3, которая получается после дилителя DIVR3

  USART1_printf("В этой версии fatfs настроен только на короткие имена в формате 8.3\n");
  USART1_printf("Чтобы были длинные имена и файлы с русскими названиями смотри следующие версии\n");


  SDMMC_SDCard_Test();              // Проверка того, что карта работает, читаются/пишуться файлы.

  //for(i=0;i<sizeof(buf4file);i++) buf4file[i] = i;
  //SDMMC_SDCard_WriteBigFile(2*1024*1024);      // Тест на запись файла большого объёма

  FR_Status = f_mount(&FatFs, "", 1);
  if (FR_Status != FR_OK)
  {
    USART1_printf("Error! While Mounting SD Card, Error Code: (%i)\r\n", FR_Status);
  }
  USART1_printf("SD Card Mounted Successfully! \r\n\n");

  FR_Status = f_open(&Fil, "Long file name 11.txt", FA_WRITE | FA_CREATE_ALWAYS);
  if(FR_Status != FR_OK)
  {
    USART1_printf("Error! While Creating/Opening A Файл.txt, Error Code: (%i)\r\n", FR_Status);
  }


  uint32_t tm = GetTickCount();

  for(i=0;i<1000;i++)
  {
    sprintf((char *)buf4file, "Линия в файле %.8d\r\n", i);
    f_write(&Fil, buf4file, strlen((char *)buf4file), &WWC);
  }

  f_close(&Fil);

  tm = GetTickCount() - tm;


  FR_Status = f_mount(NULL, "", 0);
  if (FR_Status != FR_OK)
  {
      USART1_printf("\r\nError! While Un-mounting SD Card, Error Code: (%i)\r\n", FR_Status);
  } else{
      USART1_printf("\r\nSD Card Un-mounted Successfully! \r\n");
  }


  USART1_printf("DelT=%d\n", tm);

  while(1)
  {
    GPIOInv(GPIOA, 1);
    //USART1_printf("#%d\n", random()/(RAND_MAX>>2));
    Delay(1000);
  }

  return 0;
}


static void SDMMC_SDCard_Test(void)
{
  FATFS FatFs;
  FIL Fil;
  FRESULT FR_Status;
  FATFS *FS_Ptr;
  UINT RWC, WWC; // Read/Write Word Counter
  DWORD FreeClusters;
  uint32_t TotalSize, FreeSpace;
  char RW_Buffer[200];

  do
  {
    //------------------[ Mount The SD Card ]--------------------
    FR_Status = f_mount(&FatFs, "", 1);
    if (FR_Status != FR_OK)
    {
      USART1_printf("Error! While Mounting SD Card, Error Code: (%i)\r\n", FR_Status);
      break;
    }
    USART1_printf("SD Card Mounted Successfully! \r\n\n");

    //------------------[ Get & Print The SD Card Size & Free Space ]--------------------
    f_getfree("", &FreeClusters, &FS_Ptr);

    TotalSize = (uint32_t)((FS_Ptr->n_fatent - 2) * FS_Ptr->csize * SDCARD_BLOCK_SIZE);
    FreeSpace = (uint32_t)(FreeClusters * FS_Ptr->csize * SDCARD_BLOCK_SIZE);

    USART1_printf("Total SD Card Size: %lu Bytes\r\n", TotalSize);
    USART1_printf("Free SD Card Space: %lu Bytes\r\n\n", FreeSpace);

    //------------------[ Open A Text File For Write & Write Data ]--------------------
    //Open the file
    FR_Status = f_open(&Fil, "New.txt", FA_WRITE | FA_READ | FA_CREATE_ALWAYS);
    if(FR_Status != FR_OK)
    {
      USART1_printf("Error! While Creating/Opening A New Text File, Error Code: (%i)\r\n", FR_Status);
      break;
    }
    USART1_printf("Text File Created & Opened! Writing Data To The Text File..\r\n\n");

    // (2) Write Data To The Text File [ Using f_write() Function ]
    strcpy(RW_Buffer, "Hello! From STM32 To SD Card Over SDMMC, Using f_write()\r\n");
    f_write(&Fil, RW_Buffer, strlen(RW_Buffer), &WWC);
    strcpy(RW_Buffer, "Second line f_write()\r\n");
    f_write(&Fil, RW_Buffer, strlen(RW_Buffer), &WWC);


    // Close The File
    f_close(&Fil);
    //------------------[ Open A Text File For Read & Read Its Data ]--------------------
    // Open The File
    FR_Status = f_open(&Fil, "New.txt", FA_READ);
    if(FR_Status != FR_OK)
    {
      USART1_printf("Error! While Opening (New.txt) File For Read.. \r\n");
      break;
    }

    // (2) Read The Text File's Data [ Using f_read() Function ]
    f_read(&Fil, RW_Buffer, f_size(&Fil), &RWC);
    USART1_printf("Data Read From (New.txt) Using f_read():%s", RW_Buffer);

    // Close The File
    f_close(&Fil);
    USART1_printf("File Closed! \r\n\n");

    //------------------[ Open An Existing Text File, Update Its Content, Read It Back ]--------------------
    // (1) Open The Existing File For Write (Update)
    FR_Status = f_open(&Fil, "New.txt", FA_OPEN_EXISTING | FA_WRITE);
    FR_Status = f_lseek(&Fil, f_size(&Fil)); // Move The File Pointer To The EOF (End-Of-File)
    if(FR_Status != FR_OK)
    {
      USART1_printf("Error! While Opening (MyTextFile.txt) File For Update.. \r\n");
      break;
    }

    f_close(&Fil);
    memset(RW_Buffer,'\0',sizeof(RW_Buffer)); // Clear The Buffer
    // (3) Read The Contents of The Text File After The Update
    FR_Status = f_open(&Fil, "New.txt", FA_READ); // Open The File For Read
    f_read(&Fil, RW_Buffer, f_size(&Fil), &RWC);
    USART1_printf("Data Read From (New.txt) After Update:\r\n%s", RW_Buffer);
    f_close(&Fil);

    //------------------[ Delete The Text File ]--------------------
    // Delete The File
    /*
    FR_Status = f_unlink(MyTextFile.txt);
    if (FR_Status != FR_OK){
        USART1_printf("Error! While Deleting The (MyTextFile.txt) File.. \r\n");
    }
    */
  } while(0);


  //------------------[ Test Complete! Unmount The SD Card ]--------------------
  FR_Status = f_mount(NULL, "", 0);
  if (FR_Status != FR_OK)
  {
      USART1_printf("\r\nError! While Un-mounting SD Card, Error Code: (%i)\r\n", FR_Status);
  } else{
      USART1_printf("\r\nSD Card Un-mounted Successfully! \r\n");
  }
}


static void SDMMC_SDCard_WriteBigFile(uint32_t len)    // Тест на запись файла большого объёма
{
  FATFS FatFs;
  FIL Fil;
  FRESULT FR_Status;
  uint32_t cnt;
  UINT wwc;
  char filename[12];


  USART1_printf("Write big file len=%d\n", len);

  do
  {
    FR_Status = f_mount(&FatFs, "", 1);
    if (FR_Status != FR_OK)
    {
      USART1_printf("Error! While Mounting SD Card, Error Code: (%i)\r\n", FR_Status);
      break;
    }
    USART1_printf("SD Card Mounted Successfully! \r\n\n");

    sprintf(filename, "%lu.bin", len/1000);

    FR_Status = f_open(&Fil, filename, FA_WRITE | FA_READ | FA_CREATE_ALWAYS);
    if(FR_Status != FR_OK)
    {
      USART1_printf("Error! While Creating/Opening File=%s, Error Code: (%i)\r\n", filename, FR_Status);
      break;
    }
    USART1_printf("File %s Created & Opened! Writing Data To The File..\r\n\n", filename);

    GPIOHi(GPIOA, 1);

    cnt = len / sizeof(buf4file);
    while(cnt)
    {
      f_write(&Fil, buf4file, sizeof(buf4file), &wwc);
      cnt--;
    }
    cnt = len % sizeof(buf4file);
    if(cnt) f_write(&Fil, buf4file, cnt, &wwc);

    f_close(&Fil);

    GPIOLow(GPIOA, 1);

  }while(0);


  FR_Status = f_mount(NULL, "", 0);
  if (FR_Status != FR_OK)
  {
      USART1_printf("\r\nError! While Un-mounting SD Card, Error Code: (%i)\r\n", FR_Status);
  } else{
      USART1_printf("\r\nSD Card Un-mounted Successfully! \r\n");
  }
}



