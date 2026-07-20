/*
 * sdio.c
 *
 *  Created on: 20 июн. 2026 г.
 *      Author: user
 */

#include "stm32h743xx.h"
#include "gpio.h"
#include "sdio.h"
#include "arrayoftypes.h"



// В данном файле для работы SDIO используются следующие ноги:
// C12 - SDIO_CLK
// D2  - SDIO_CMD
// C8  - SDIO_D0
// C9  - SDIO_D1
// C10 - SDIO_D2
// C11 - SDIO_D3

#define SDMMC_STATIC_CMD_FLAGS            ((uint32_t)(SDMMC_ICR_CCRCFAILC | SDMMC_ICR_CTIMEOUTC | SDMMC_ICR_CMDRENDC | SDMMC_ICR_CMDSENTC | SDMMC_ICR_BUSYD0ENDC))
#define SDMMC_STATIC_DATA_FLAGS           ((uint32_t)(SDMMC_ICR_DCRCFAILC | SDMMC_ICR_DTIMEOUTC | SDMMC_ICR_TXUNDERRC | SDMMC_ICR_RXOVERRC  | SDMMC_ICR_DATAENDC  | SDMMC_ICR_DHOLDC | SDMMC_ICR_DBCKENDC | SDMMC_ICR_DABORTC | SDMMC_ICR_IDMATEC | SDMMC_ICR_IDMABTCC))

// Константы возможных результатов ответа R6
#define SDMMC_R6_GENERAL_UNKNOWN_ERROR     ((uint32_t)0x00002000U)
#define SDMMC_R6_ILLEGAL_CMD               ((uint32_t)0x00004000U)
#define SDMMC_R6_COM_CRC_FAILED            ((uint32_t)0x00008000U)

uint32_t SD_BlockNumber = 0;      // Кол-во блоков в SD карте (обычно размер блока это 512 байт, т.о. объём карты памяти это кол-во блоков * на 512 байт)
uint64_t SD_Size = 0;             // Объём SD карты
uint32_t SD_InitOK = 0;           // Если карта была проинициализированна, то эта переменная будет равна 1


static int32_t SDCardVer=0;
static value16b_t SD_CID = {0};
static value16b_t SD_CSD = {0};
static int32_t SD_RelAddr = 0;
static int32_t SD_RelAddrShift16 = 0;    // Адрес сдвинутый на 16 бит влево, т.к. именно это значение нужно передавать в качестве аргумента при отправке команд


static void SDIO_Init_Pins(void)
{
  GPIOConfig(GPIOD, 2,  MODE_ALTER_FUNC, OTYPE_PUSH_PULL, SPEED_VERY_HI, PULL_UP, 12);  // SDIO_CMD
  GPIOConfig(GPIOC, 8,  MODE_ALTER_FUNC, OTYPE_PUSH_PULL, SPEED_VERY_HI, PULL_UP, 12);  // SDIO_D0
  GPIOConfig(GPIOC, 9,  MODE_ALTER_FUNC, OTYPE_PUSH_PULL, SPEED_VERY_HI, PULL_UP, 12);  // SDIO_D1
  GPIOConfig(GPIOC, 10, MODE_ALTER_FUNC, OTYPE_PUSH_PULL, SPEED_VERY_HI, PULL_UP, 12);  // SDIO_D2
  GPIOConfig(GPIOC, 11, MODE_ALTER_FUNC, OTYPE_PUSH_PULL, SPEED_VERY_HI, PULL_UP, 12);  // SDIO_D3
  GPIOConfig(GPIOC, 12, MODE_ALTER_FUNC, OTYPE_PUSH_PULL, SPEED_VERY_HI, PULL_UP, 12);  // SDIO_CLK
}


static int32_t SDIO_SendCMD(uint32_t cmd, uint32_t arg)
{
  uint32_t tmpreg = 0;

  uint32_t count = SystemCoreClock>>8;  // Значение счётчика для цикла, чтобы выдержать некоторую паузу

  SET_BIT(tmpreg, cmd | SDMMC_CMD_CPSMEN);

  SDMMC1->ARG = arg;
  SDMMC1->CMD = tmpreg;

  // После записи нужно проверить состояние флага SDMMC_STA_CMDSENT (т.е. команда отправлена)
  while(1)
  {
    if (count-- == 0U) return -1;                        // Вышло время ожидания программное
    if(READ_BIT(SDMMC1->STA, SDMMC_STA_CMDSENT)) break;
  }

  SET_BIT(SDMMC1->ICR, SDMMC_ICR_CMDSENTC);    // Далее флаг SDMMC_STA_CMDSENT нужно сбросить записью в регистр SDMMC1->ICR

  return 0;
}

static void SDIO_SendCMD_ResponseShort(uint32_t cmd, uint32_t arg)
{
  uint32_t tmpreg = 0;

  // В регистр CMD будет записана команда, признак запуска машины состояния процесса обработки команды и признак ответа (короткий ответ)
  SET_BIT(tmpreg, cmd | SDMMC_CMD_CPSMEN | 1<<SDMMC_CMD_WAITRESP_Pos);

  SDMMC1->ARG = arg;
  SDMMC1->CMD = tmpreg;
}

static void SDIO_SendCMD_ResponseLong(uint32_t cmd, uint32_t arg)
{
  uint32_t tmpreg = 0;

  // В регистр CMD будет записана команда, признак запуска машины состояния процесса обработки команды и признак ответа (длинный ответ)
  SET_BIT(tmpreg, cmd | SDMMC_CMD_CPSMEN | 3<<SDMMC_CMD_WAITRESP_Pos);

  SDMMC1->ARG = arg;
  SDMMC1->CMD = tmpreg;
}

static int32_t SDIO_GetCmdResp1(uint8_t answ)
{
  int err = 0;
  uint32_t sta_reg;

  uint32_t count = SystemCoreClock>>8;  // Значение счётчика для цикла, чтобы выдержать некоторую паузу

  do
  {
    sta_reg = SDMMC1->STA;
    if (count-- == 0U) return -1;  // Вышло время ожидания программное
    // Цикл прервётся при следующих условиях:
    // SDMMC_STA_CCRCFAIL   - Ошибка контрольной суммы
    // SDMMC_STA_CMDREND    - Был получен ответ
    // SDMMC_STA_CTIMEOUT   - Вышло время ожидания ответа ( 64 периода тактового сигнала (так в документации написано см. регистр SDMMC_STAR CTIMEOUT) )
    // SDMMC_STA_BUSYD0END  - Какой-то флаг знанятости по линии Data0
    // SDMMC_STA_CPSMACT    - Машина состояния перестала работать (если в регистре состояние нет этого флага, значит что-то пошло не так)
  } while (((sta_reg & (SDMMC_STA_CCRCFAIL | SDMMC_STA_CMDREND | SDMMC_STA_CTIMEOUT | SDMMC_STA_BUSYD0END)) == 0U) || (sta_reg & SDMMC_STA_CPSMACT));

  // Сброс флагов, которые установлены

  if(READ_BIT(SDMMC1->STA, SDMMC_STA_CTIMEOUT)) err=-2;   // Вышло время ожидания аппаратное
  if(READ_BIT(SDMMC1->STA, SDMMC_STA_CCRCFAIL)) err=-3;   // Не верная контрльная сумма в ответе

  SDMMC1->ICR = SDMMC_STATIC_CMD_FLAGS;

  if(SDMMC1->RESPCMD != answ) return -4;                  // Ответ не соответствует команде

  // В предыдущих версиях тут было чтение регистра SDMMC1->RESP1, и анализ ошибок, но т.к. эти ошибки не использовались, то это было убрано,
  // если вдруг это понадобится снова, то можно вытащить обработку ошибок из предыдущих версий.

  return err;
}

static int32_t SDIO_GetCmdResp2(void)
{
  int err = 0;
  uint32_t sta_reg;

  uint32_t count = SystemCoreClock>>8;  // Значение счётчика для цикла, чтобы выдержать некоторую паузу

  do
  {
    if (count-- == 0U) return -1;  // Вышло время ожидания программное
    sta_reg = SDMMC1->STA;
    // Цикл прервётся при следующих условиях:
    // SDMMC_STA_CCRCFAIL   - Ошибка контрольной суммы
    // SDMMC_STA_CMDREND    - Был получен ответ
    // SDMMC_STA_CTIMEOUT   - Вышло время ожидания ответа ( 64 периода тактового сигнала (так в документации написано см. регистр SDMMC_STAR CTIMEOUT) )
    // SDMMC_STA_BUSYD0END  - Какой-то флаг знанятости по линии Data0
    // SDMMC_STA_CPSMACT    - Машина состояния перестала работать (если в регистре состояние нет этого флага, значит что-то пошло не так)
  } while (((sta_reg & (SDMMC_STA_CCRCFAIL | SDMMC_STA_CMDREND | SDMMC_STA_CTIMEOUT)) == 0U) || (sta_reg & SDMMC_STA_CPSMACT));

  // Сброс флагов, которые установлены

  if(READ_BIT(SDMMC1->STA, SDMMC_STA_CTIMEOUT)) err=-2;   // Вышло время ожидания аппаратное
  if(READ_BIT(SDMMC1->STA, SDMMC_STA_CCRCFAIL)) err=-3;   // Не верная контрльная сумма в ответе

  SDMMC1->ICR = SDMMC_STATIC_CMD_FLAGS;

  return err;
}

static int32_t SDIO_GetCmdResp3(void)
{
  int err = 0;
  uint32_t sta_reg;

  uint32_t count = SystemCoreClock>>8;  // Значение счётчика для цикла, чтобы выдержать некоторую паузу

  do
  {
    if (count-- == 0U) return -1;  // Вышло время ожидания программное
    sta_reg = SDMMC1->STA;
    // Цикл прервётся при следующих условиях:
    // SDMMC_STA_CCRCFAIL   - Ошибка контрольной суммы
    // SDMMC_STA_CMDREND    - Был получен ответ
    // SDMMC_STA_CTIMEOUT   - Вышло время ожидания ответа ( 64 периода тактового сигнала (так в документации написано см. регистр SDMMC_STAR CTIMEOUT) )
    // SDMMC_STA_CPSMACT    - Машина состояния перестала работать (если в регистре состояние нет этого флага, значит что-то пошло не так)
  } while (((sta_reg & (SDMMC_STA_CCRCFAIL | SDMMC_STA_CMDREND | SDMMC_STA_CTIMEOUT)) == 0U) || (sta_reg & SDMMC_STA_CPSMACT));

  if(READ_BIT(SDMMC1->STA, SDMMC_STA_CTIMEOUT)) return -2;   // Вышло время ожидания аппаратное

  SDMMC1->ICR = SDMMC_STATIC_CMD_FLAGS;

  return err;
}

static int32_t SDIO_GetCmdResp6(uint8_t answ)
{
  int32_t err=0;
  uint32_t resp1;

  // После записи нужно проверить состояние флага SDMMC_STA_CMDSENT (т.е. команда отправлена)
  while(1)
  {
    uint32_t sta = SDMMC1->STA;
    if(READ_BIT(sta, SDMMC_STA_CMDREND)) break;               // Признак того,  что получен ответ
    if(READ_BIT(sta, SDMMC_STA_CCRCFAIL)) {err=-1; break;}    // Не совпала контрольная сумма
    if(READ_BIT(sta, SDMMC_STA_CTIMEOUT)) {err=-2; break;}    // Вышло время ожидания ответа
  }

  // Контрольная сумма не сошлась
  if(err==-1) SET_BIT(SDMMC1->ICR, SDMMC_ICR_CCRCFAILC);

  // Если SD карта не отвечает в течении некоторого времени (64 периода тактового сигнала (так в документации написано см. регистр SDMMC_STAR CTIMEOUT)
  if(err==-2) SET_BIT(SDMMC1->ICR, SDMMC_ICR_CTIMEOUTC);

  SDMMC1->ICR = SDMMC_STATIC_CMD_FLAGS;

  if(SDMMC1->RESPCMD != answ) return -4;                  // Ответ не соответствует команде

  resp1 = SDMMC1->RESP1;

  if(resp1 & SDMMC_R6_GENERAL_UNKNOWN_ERROR)  return -5;
  if(resp1 & SDMMC_R6_ILLEGAL_CMD)            return -6;
  if(resp1 & SDMMC_R6_COM_CRC_FAILED)         return -7;

  err = (uint16_t)(resp1 >> 16);    // Если всё хорошо, то будет возвращён адрес карты, который будет использоваться в дальнейшем

  return err;
}

static int32_t SDIO_GetCmdResp7(void)
{
  int32_t err=0;

  // После записи нужно проверить состояние флага SDMMC_STA_CMDSENT (т.е. команда отправлена)
  while(1)
  {
    uint32_t sta = SDMMC1->STA;
    if(READ_BIT(sta, SDMMC_STA_CMDREND)) break;               // Признак того,  что получен ответ
    if(READ_BIT(sta, SDMMC_STA_CCRCFAIL)) {err=-1; break;}    // Не совпала контрольная сумма
    if(READ_BIT(sta, SDMMC_STA_CTIMEOUT)) {err=-2; break;}    // Вышло время ожидания ответа
  }

  // Контрольная сумма не сошлась
  if(err==-1) SET_BIT(SDMMC1->ICR, SDMMC_ICR_CCRCFAILC);

  // Если SD карта не отвечает в течении некоторого времени (64 периода тактового сигнала (так в документации написано см. регистр SDMMC_STAR CTIMEOUT)
  if(err==-2) SET_BIT(SDMMC1->ICR, SDMMC_ICR_CTIMEOUTC);

  // Далее флаг SDMMC_STA_CMDSENT нужно сбросить записью в регистр SDMMC1->ICR
  if(err==0) SET_BIT(SDMMC1->ICR, SDMMC_ICR_CMDRENDC);

  return err;
}

static void SDIO_StartTransfer(uint32_t cmd, uint32_t sn)
{
  uint32_t tmpreg = 0;

  // Значение команды + Разрешение работы машины состояни +  Ожидание короткого ответа + Установка бита DTEN в регистре SDMMC1->DCTRL
  SET_BIT(tmpreg, cmd | SDMMC_CMD_CPSMEN | 1<<SDMMC_CMD_WAITRESP_Pos | SDMMC_CMD_CMDTRANS);

  SDMMC1->ARG = sn;
  SDMMC1->CMD = tmpreg;
}

static int32_t SDIO_StopTransfer(void)
{
  int32_t err = 0;
  uint32_t tmpreg = 0;

  // cmd12 команда для остановки передачи данных
  // SDMMC_CMD_CPSMEN - Запуск машины состояния
  // 1<<SDMMC_CMD_WAITRESP_Pos - Ожидание короткого ответа
  // SDMMC_CMD_CMDSTOP - Необходимо выставить этот бит при отправке команды остановки

  SET_BIT(tmpreg, 12 | SDMMC_CMD_CPSMEN | 1<<SDMMC_CMD_WAITRESP_Pos | SDMMC_CMD_CMDSTOP);

  SDMMC1->ARG = 0;
  SDMMC1->CMD = tmpreg;

  err = SDIO_GetCmdResp1(12);

  CLEAR_BIT(SDMMC1->CMD, SDMMC_CMD_CMDSTOP);

  return err;
}

int32_t SDIO_Init(void)
{
  int err = 0;
  int count;
  uint32_t clkdiv;

  SDIO_Init_Pins();   // Настройка ног для работы с SD картой

  // В первую очередь необходимо настроить источник тактирования для модуля SDIO
  MODIFY_REG(RCC->D1CCIPR, RCC_D1CCIPR_SDMMCSEL_Msk, 0<<RCC_D1CCIPR_SDMMCSEL_Pos);  // Выбрано тактирование от PLL1_q_ck
  MODIFY_REG(RCC->AHB3ENR, RCC_AHB3ENR_SDMMC1EN_Msk, 1<<RCC_AHB3ENR_SDMMC1EN_Pos);  // Включение тактирования модуля SDMMC

  SET_BIT(RCC->AHB3RSTR, RCC_AHB3RSTR_SDMMC1RST);                                   // Сброс блока SDMMC
  __NOP(); __NOP(); __NOP(); __NOP();                                               // Это для паузы
  CLEAR_BIT(RCC->AHB3RSTR, RCC_AHB3RSTR_SDMMC1RST);                                   // Сброс блока SDMMC


  // т.к. источник тактирования модуля SDMMC выбран PLL1Q1, то и значение делителя нужно считать на основании частоты PLL1QFreq
  clkdiv = PLL1QFreq / 400000 / 2;                              // Делитель, который надо установить на этапе начальной настройки (частота должна быть не более 400кГц)

  SDMMC1->CLKCR = 0;                                            // Значение по умолчанию
  SET_BIT(SDMMC1->CLKCR, SDMMC_CLKCR_NEGEDGE);                  // Когда будут меняться данные на линия cmd и data0..3 (в данном случае на спаде тактового сигнала, т.е. считывание картой будет происходить на возрастании тактового сигнала)
  SET_BIT(SDMMC1->CLKCR, 1<<SDMMC_CLKCR_WIDBUS_Pos);            // Ширина шины 4 бита
  SET_BIT(SDMMC1->CLKCR, clkdiv<<SDMMC_CLKCR_CLKDIV_Pos);       // Установка делителя тактового сигнала

  // После подачи питания на SD карту необходимо подождать 74 такта прежде чем начинать работать с картой
  SET_BIT(SDMMC1->POWER, 3<<SDMMC_POWER_PWRCTRL_Pos);           // Так включается питание модуля SDMMC

  // Эта задержка, которая обеспечивает ожидение 74+ тактов
  count = SystemCoreClock / 8000;
  while(count--);


  SD_InitOK = 0;           // Если карта была проинициализированна, то эта переменная будет равна 1

  // После того как прошло 74 или более циклов, можно продолжать настройку SD карты
  err = SDIO_SendCMD(0, 0);           // Отправка cmd0, для сброса карты
  if(err) return SDIO_Init_err_cmd0;

  //CMD8 — это команда из протокола SD‑карт, которую отправляют на этапе инициализации в SPI‑режиме.
  // Её цель — проверить, поддерживает ли карта нужный диапазон напряжений, и определить, карта ли это SD версии 2.0+ (SDHC/SDXC) или старая (SDSC) / MMC.
  // Аргумент: обычно 0x000001AA (где 0xAA — контрольное значение, которое карта должна вернуть).
  // Ответ: R7 (4 байта), где последние 4 бита должны совпадать с контрольным значением (0xAA). Если нет — карта не поддерживает CMD8 (значит, это старая SDSC или MMC).
  SDIO_SendCMD_ResponseShort(8, 0x000001AA);        // 1 в 8-м бита означает, напряжение хоста от 2.7 до 3.6 вольта
  err = SDIO_GetCmdResp7();                         // Ожидание ответа R7
  if(err) return SDIO_Init_err_cmd8;
  SDCardVer = 2;                           // Если ошибок нет, значит карта версии 2.0 (только с такими и будем работать)

  // Команда CMD55 в работе с SD-картами служит своеобразным «префиксом»:
  // она сообщает карте, что следующая за ней команда будет нестандартной (так называемой прикладной, или ACMD).
  // Это ключевой шаг для работы с современными картами SDHC/SDXC, которые поддерживают расширенный набор команд.

  // После отправки CMD55 карта должна ответить.
  // Ожидаемый ответ — тип R1 (подтверждение выполнения команды) со значением 0x01.
  // Если ответ отличается, это сигнализирует об ошибке.

  // Только после успешного выполнения CMD55 и получения корректного R1 можно отправлять саму прикладную команду (например, ACMD41).
  // Эта команда, в свою очередь, сообщает карте, что она поддерживает определённый тип карт (например, SDSC — стандартная SD-карта), и запускает процесс её инициализации.

  // Далее в цикле необходимо отправлять команды CMD55 и ACMD41 до тех пор, пока карта не проинициализируется
  // После подачи питания карта не готова к работе: ей нужно выполнить самотестирование, настроить внутренние блоки, проверить целостность прошивки и т. п.
  // ACMD41 говорит карте: «Начинай инициализацию».

  // Отправка команды ACMD41 с аргументом 0x80100000
  count = 2000;                   // Предел кол-ва попыток ожидания установки нужного напряжения
  uint32_t validvoltage = 0;
  uint32_t response;

  while(count--)   // Пока не исчерпаны попытки ( count!=0 ) надо продолжать
  {
    // // Только после успешного выполнения CMD55 и получения корректного R1 можно отправлять саму прикладную команду (например, ACMD41).
    SDIO_SendCMD_ResponseShort(55, 0);
    err = SDIO_GetCmdResp1(55);                       // Ожидание ответа R1 на команду 55
    if(err) return SDIO_Init_err_cmd55_c;

    // Аргумент 0x80100000 при отправке команды ACMD41 (Send Operation Conditions) — это битовая маска, которой хост (контроллер) сообщает карте свои возможности:
    // какие диапазоны напряжений он поддерживает и что готов работать с картами высокой ёмкости (SDHC/SDXC).
    SDIO_SendCMD_ResponseShort(41, 0xc0100000);

    err = SDIO_GetCmdResp3();                     // После команды 41 надо дождаться ответа R3 от sd карты
    if(err) return SDIO_Init_err_cmd41_c;

    // Необходимо опеределить, что вернула sd карта, чтобы понять завершился ли процесс инициализации карты
    response = SDMMC1->RESP1;

    if(READ_BIT(response, 1<<31))
    {
      validvoltage = 1;
      break;
    }

    for(int i=0;i<100000;i++);    // Пауза, чтобы впустую не гонять данные по шине
  }

  if(!validvoltage) return SDIO_Init_err_volt;

  // Далее необходимо отправить команду cmd2 чтобы получить CID карты (в этой структуре хранится разная информация о sd карте (не всегда правда заполнена, поэтому надеятся на неё не стоит))
  SDIO_SendCMD_ResponseLong(2,0);             // Длинный ответ возвращается в регистры SDMMC1->RESPx (x - 1-4)
  err = SDIO_GetCmdResp2();                   // После команды cmd2 ожидается ответ R2 от sd карты
  if(err) return SDIO_Init_err_cmd2_R2;

  SD_CID.u320 = SDMMC1->RESP4;
  SD_CID.u321 = SDMMC1->RESP3;
  SD_CID.u322 = SDMMC1->RESP2;
  SD_CID.u323 = SDMMC1->RESP1;

  // После команды cmd2 отправляется команда cmd3
  // CMD3 (SEND_REL_ADDR): Хост просит карту назначить себе временный адрес (RCA — Relative Card Address).
  // Это нужно, если на шине несколько карт (мультикарточные слоты).
  // Для одной карты это всё равно делают по стандарту.
  SDIO_SendCMD_ResponseShort(3, 0);
  SD_RelAddr = SDIO_GetCmdResp6(3);                 // Запоминаем адрес карты, по которому будем к ней обращаться
  if(SD_RelAddr<0) return SDIO_Init_err_GetRelAdd;
  SD_RelAddrShift16 = SD_RelAddr<<16;               // Такое значение нужно передавать в качестве аргумента при отправке команд


  // Чтение структуры CSD карты, эта структура содержим много полезной инфармации (но определим только размер карты в байтах и кол-во блоков в карте это пригодится для FATFS)
  SDIO_SendCMD_ResponseLong(9, SD_RelAddrShift16);  // Длинный ответ возвращается в регистры SDMMC1->RESPx (x - 1-4)
  err = SDIO_GetCmdResp2();                         // После команды cmd9 ожидается длинный ответ R2 от sd карты
  if(err) return SDIO_Init_err_GetCSD;

  SD_CSD.u320 = SDMMC1->RESP4;
  SD_CSD.u321 = SDMMC1->RESP3;
  SD_CSD.u322 = SDMMC1->RESP2;
  SD_CSD.u323 = SDMMC1->RESP1;

  // Кол-во блоков находится в поле C_SIZE структуры CSD (поле C_SIZE это биты 69 - 48, всего 22 бита)
  SD_BlockNumber = (uint32_t)(SD_CSD.m[8] & 0x3f)<<16 | (uint32_t)(SD_CSD.m[7])<<8 | (uint32_t)(SD_CSD.m[6]);  // Так получаем кол-во блоков в карте памяти
  SD_BlockNumber *= 1024;               // Формула справедлива для SDHC и SDXS карт памяти
  SD_Size = (uint64_t)SD_BlockNumber * (uint64_t)512;

  // После получения временного адреса карты, нужно выбрать карту, чтобы все остальные команды отправлялись только к ней, остальные карты на шине (если они есть) переходят в режим ожидания
  SDIO_SendCMD_ResponseShort(7, SD_RelAddrShift16);    // Адрес карты нужно задать в старших 16-ти битах (чтобы не сдвигать SD_RelAddr влево на 16 сделанна переменная SD_RelAddrShift16)
  err = SDIO_GetCmdResp1(7);

  // Далее нужно задать размер блока (команда cmd16) которым будем работать с картой (512 - байт это уже почти стандарт)
  SDIO_SendCMD_ResponseShort(16, SDCARD_BLOCK_SIZE);
  err = SDIO_GetCmdResp1(16);
  if(err) return SDIO_Init_err_SetBlockSize;

  // Теперь можно переводить sd карту на работу по 4-м битам
  // Для перевода sd карты на работу по 4-м линиям нужно отправить команду ACMD6 (т.е. сначала нужно отправить команду cmd55)
  // Теперь, когда получен временный адрес карты агрумент команды 55 должен содержать адрес карты
  SDIO_SendCMD_ResponseShort(55, SD_RelAddrShift16);
  err = SDIO_GetCmdResp1(55);                       // Ожидание ответа R1 на команду 55
  if(err) return SDIO_Init_err_cmd55_cmd6;

  SDIO_SendCMD_ResponseShort(6, 2);                 // аргумент 2 означает, что хост хочет обмениваться данными с картой по 4-м линиям
  err = SDIO_GetCmdResp1(6);                        // Ожидание ответа R1
  if(err) return SDIO_Init_err_cmd6_R1;


  // Теперь можно поднять частоту тактового сигнала
  // т.к. источник тактирования модуля SDMMC выбран PLL1Q1, то и значение делителя нужно считать на основании частоты PLL1QFreq
  clkdiv = PLL1QFreq / SD_WORK_FREQ / 2;                              // Делитель, который надо установить для частоты SD_WORK_FREQ
  MODIFY_REG(SDMMC1->CLKCR, SDMMC_CLKCR_CLKDIV_Msk, clkdiv<<SDMMC_CLKCR_CLKDIV_Pos);

  SD_InitOK = 1;      // Карта проинициализированна

  return err;
}

int32_t SDIO_GetCardState(uint32_t *CardSatatus)
{
  int32_t err = 0;

  SDIO_SendCMD_ResponseShort(13, SD_RelAddrShift16);
  err = SDIO_GetCmdResp1(13);                        // Ожидание ответа R1
  *CardSatatus = SDMMC1->RESP1;                      // В ответе нас интересует 8-й бит, если он равен 1, значит карта готова к операциям записи / чтению

  return err;
}

int32_t SDIO_WaitData0Release(uint32_t tout)  // Ожидание освобождения линии data0 (если ниния data0 прятянута к земле (после операции чтения/записи), это значит, что sd карта занята и не может быть использована
{
  while(READ_BIT(SDMMC1->STA, SDMMC_STA_BUSYD0))
  {
    tout--; if(!tout) return -1;
  }
  return 0;
}

int32_t SDIO_ReadSectors(uint32_t sn, uint32_t sc, uint8_t *buf)
{
  // ВНИМАНИЕ!!! перед любой операцией будь то чтение или запись нужно убедиться, что карта к этому готова для этого есть процедура SDIO_GetCardState.
  // SDIO_GetCardState - отправляет команду cmd13, чтобы получить текущее состояние карты, в ответе необходимо проверять 8-й бит (READY_FOR_DATA), если он равен 1, значит карта готова к операциям записи или чтения.

  // Чтение сектора/секторов
  // sn   - номер сектора с которого необходимо начать чтение
  // sc   - кол-во секторов, которое необходимо прочитать
  // buf  - куда скаладывать прочитанное


  int32_t err = 0;
  uint32_t tmpreg;
  uint32_t stat;
  uint32_t *pbuf32u = (uint32_t*)buf;
  uint32_t byterecv = 0;

  // Для чтения секторов с sd карты есть команды cmd17 - чтение одного сектора
  // cmd18 - непрерывное чтение, пока не будет отправлена команда cmd12 - остановить поток данных от sd карты

  byterecv = sc * SDCARD_BLOCK_SIZE;        // Кол-во байт, которое нужно принять

  // Чтобы запустить процесс чтения необходимо настроить следующие регистры
  // SDMMCx->DTIMER - предел ожидания данных
  // SDMMCx->DLEN   - кол-во считываемых байт (оно кратно размеру блока, который почти всегда 512 байт)
  // SDMMCx->DCTRL  - тут сразу несколько полей (размер одного блока, направление передачи, режим передачи и др.)

  // Запись в эти регистры должна выполняться до записи в регистр SDMMCx->DCTRL
  SDMMC1->DTIMER = 0xFFFFFFFF;                  // Время задаётся в периодах тактирования на линии CLK
  SDMMC1->DLEN = byterecv;                  // Кол-во байт для считывания

  tmpreg = 0;
  SET_BIT(tmpreg, 9<<SDMMC_DCTRL_DBLOCKSIZE_Pos);     // Размер блока 512 байт
  SET_BIT(tmpreg, SDMMC_DCTRL_DTDIR);                 // Направление передачи от карты к хосту
  SDMMC1->DCTRL = tmpreg;                             // Запись в регистр DCTRL но обмена ещё нет т.к. не установлен бит DTEN

  // После настройки регистра SDMMC1->DCTRL и установки бита SDMMC_CMD_CMDTRANS в регистре команд можно отправлять команды sd карте для чтения одного или нескольких блоков
  if(sc>1)
  {
    SDIO_StartTransfer(18, sn);
    err = SDIO_GetCmdResp1(18);
    if(err) return SDIO_ReadSectors_err_cmd18;
  }
  else   // Если читается один блок, то будет отправлена команда cmd17
  {
    SDIO_StartTransfer(17, sn);
    err = SDIO_GetCmdResp1(17);
    if(err) return SDIO_ReadSectors_err_cmd17;
  }

  // Далее процесс приёма данных
  while(1)     // Пока есть данные для считывания, будем продолжать
  {
    stat = SDMMC1->STA;
    // Цикл приёма данных будет прерван при различных проблемах
    if(stat & (SDMMC_STA_RXOVERR | SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT | SDMMC_STA_DATAEND)) break;
    if(READ_BIT(stat, SDMMC_STA_RXFIFOHF) && (byterecv>=32))      // Ожидание заполнения FIFO на половину
    {
      // Если FIFO заполнено на половину, то начинаем чтение данных (при этом модуль SDMMC будет дальше заполнять FIFO)
      for(int i=0;i<8;i++) { *pbuf32u = SDMMC1->FIFO; pbuf32u++;}
      byterecv -= 32;
    }
  }

  // После приёма данных нужно сбросить бит SDMMC_CMD_CMDTRANS
  CLEAR_BIT(SDMMC1->CMD, SDMMC_CMD_CMDTRANS);

  // Если выполнялся приём нескольких блоков, то необходимо отправить команду остановки передачи cmd12
  if(sc>1)
  {
    err = SDIO_StopTransfer();
    if(err) return SDIO_ReadSectors_err_StopTrans;
  }

  // В конец обнуляем все флаги, которые могли бы сигнализировать о проблеме
  SDMMC1->ICR = SDMMC_STATIC_CMD_FLAGS | SDMMC_STATIC_DATA_FLAGS;

  return err;
}

int32_t SDIO_WriteSectors(uint32_t sn, uint32_t sc, uint8_t *buf)
{
  // ВНИМАНИЕ!!! перед любой операцией будь то чтение или запись нужно убедиться, что карта к этому готова для этого есть процедура SDIO_GetCardState.
  // SDIO_GetCardState - отправляет команду cmd13, чтобы получить текущее состояние карты, в ответе необходимо проверять 8-й бит (READY_FOR_DATA), если он равен 1, значит карта готова к операциям записи или чтения.

  // Запись сектора/секторов
  // sn   - номер сектора с которого необходимо начать запись
  // sc   - кол-во секторов, которое необходимо записать
  // buf  - откуда брать данные для записи

  int32_t err = 0;
  uint32_t tmpreg;
  uint32_t stat;
  uint32_t *pbuf32u = (uint32_t*)buf;
  uint32_t bytesend = 0;

  // Для записи секторов в sd карту есть команды cmd24 - запись одного блока
  // cmd25 - запись нескольких блоков

  bytesend = sc * SDCARD_BLOCK_SIZE;        // Кол-во байт, которое нужно записать

  // Чтобы запустить процесс чтения необходимо настроить следующие регистры
  // SDMMCx->DTIMER - предел ожидания данных
  // SDMMCx->DLEN   - кол-во считываемых байт (оно кратно размеру блока, который почти всегда 512 байт)
  // SDMMCx->DCTRL  - тут сразу несколько полей (размер одного блока, направление передачи, режим передачи и др.)

  // Запись в эти регистры должна выполняться до записи в регистр SDMMCx->DCTRL
  SDMMC1->DTIMER = 0xFFFFFFFF;                   // Время задаётся в периодах тактирования на линии CLK
  SDMMC1->DLEN = bytesend;                  // Кол-во байт для считывания

  tmpreg = 0;
  SET_BIT(tmpreg, 9<<SDMMC_DCTRL_DBLOCKSIZE_Pos);     // Размер блока 512 байт
  SDMMC1->DCTRL = tmpreg;                             // Запись в регистр DCTRL но обмена ещё нет т.к. не установлен бит DTEN

  // После настройки регистра SDMMC1->DCTRL и установки бита SDMMC_CMD_CMDTRANS в регистре команд можно отправлять команды sd карте для записи одного или нескольких блоков
  if(sc>1)
  {
    SDIO_StartTransfer(25, sn);
    err = SDIO_GetCmdResp1(25);
    if(err) return SDIO_WriteSectors_err_cmd25;
  }
  else   // Если читается один блок, то будет отправлена команда cmd17
  {
    SDIO_StartTransfer(24, sn);
    err = SDIO_GetCmdResp1(24);
    if(err) return SDIO_WriteSectors_err_cmd24;
  }

  // Далее процесс записи данных
  while(1)
  {
    stat = SDMMC1->STA;
    if(stat & (SDMMC_STA_TXUNDERR | SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT | SDMMC_STA_DATAEND)) break;
    if (READ_BIT(stat, SDMMC_STA_TXFIFOHE) && (bytesend >= 32U))
    {
      for(int i=0U;i<8;i++) { SDMMC1->FIFO = *pbuf32u; pbuf32u++;}
      bytesend -= 32U;
    }
  }

  // После приёма данных нужно сбросить бит SDMMC_CMD_CMDTRANS
  CLEAR_BIT(SDMMC1->CMD, SDMMC_CMD_CMDTRANS);

  // Если выполнялся приём нескольких блоков, то необходимо отправить команду остановки передачи cmd12
  if(sc>1)
  {
    err = SDIO_StopTransfer();
    if(err) return SDIO_ReadSectors_err_StopTrans;
  }

  // В конец обнуляем все флаги, которые могли бы сигнализировать о проблеме
  SDMMC1->ICR = SDMMC_STATIC_CMD_FLAGS | SDMMC_STATIC_DATA_FLAGS;

  return err;
}









