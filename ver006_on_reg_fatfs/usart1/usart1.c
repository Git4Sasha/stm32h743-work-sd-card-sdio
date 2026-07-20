#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "stm32h7xx.h"
#include "system_stm32h7xx.h"
#include "usart1.h"
#include "gpio.h"

// здесь USART1 настраивается так, чтобы отправка выполнялись через DMA
// приём выполняется в кольцевой буфер

static unsigned char BufForInData[USART1_INBUFLEN];   // Буфер для данных поступающих из вне
static unsigned char InBufHead = 0;                   // Номер ячейки в которую будут сохраняться, поступающие данные (голова буфера)
static unsigned char InBufTail = 0;                   // Номер ячейки из которой можно считывать данные (хвост буфера)
static unsigned int InLostDataCount = 0;              // Счётчик потерянных при приёме байт

static unsigned char BufForOutData[USART1_OUTBUFLEN*2]; // Буфер для передачи данных во вне  (*2 это для двойной буферизации)
static unsigned char *CurBufForOutData=BufForOutData;   // Это адрес того буфера из которого можно считывать данные
//static unsigned int IndexForOutData=0;                  // Этот индек изпользуется в таких функция как USART2_AddUIntxx


static void InitGPIOA() // Настройка ножек порта A для работы в качестве Rx и Tx
{
  // Конфигурация ножек в режиме альтернативной функции, а так же устанавливаются в режим тяни/толкай
  // PA9 - Tx;  PA10 - Rx

  // Режим в котором необходимо конфигурировать ноги для работю какой-либо перефирии, написанно в документации в разделе GPOI
  GPIOConfig(GPIOA, 9, 2, 0, 1, 0, 7);   // PA9 - Tx;
  GPIOConfig(GPIOA, 10, 2, 0, 1, 0, 7);  // PA10 - Rx
}

static void InitGPIOB() // Настройка ножек порта B для работы в качестве Rx и Tx
{
  // Конфигурация ножек в режиме альтернативной функции, а так же устанавливаются в режим тяни/толкай
  // PB6 - Tx;  PB7 - Rx

  // Режим в котором необходимо конфигурировать ноги для работю какой-либо перефирии, написанно в документации в разделе GPOI
  GPIOConfig(GPIOB, 6, 2, 0, 1, 0, 7);    // PB6 - Tx;
  GPIOConfig(GPIOB, 7, 2, 0, 1, 0, 7);    // PB7 - Rx
}

void USART1_Init(uint32_t br, uint32_t remap)
{
  // br - Скорость работы порта 4300, 9600, 115200 ...
  // remap - Признак переназначения ног (0 - изпользуются ноги PA9-Tx, PA10-Rx; 1 - изпользуются ноги PB6-Tx, PB7-Rx)

  int32_t divider, fraction;

  // После сброса USART1 тактируется от pclk2 - сигнал тактирования шины APB2

  RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;     // Включение тактирования контролёра DMA1
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;   // Разрешение тактирования последовательного порта 1 ( USART1 )

  if(remap)
    InitGPIOB(); // Настройка ножек порта A для работы в качестве Rx и Tx
  else
    InitGPIOA(); // Настройка ножек порта A для работы в качестве Rx и Tx

  RCC->APB2RSTR |= RCC_APB2RSTR_USART1RST; // Сброс последовательного порта 1
  RCC->APB2RSTR &= ~RCC_APB2RSTR_USART1RST; // Сброс бита сброса (ну и каламбур) последовательного порта 1

  USART1->CR1 = 0; // Сброс контрольного регистра
  //USART1->CR1 |= USART_CR1_OVER8;   // Разрешение работы по 8-ми отсчётам для определения наличия низкого или высокого уровня на входе RX ноги
  USART1->CR1 |= USART_CR1_TE;        // Разрешение работы передатчика            (закоментированно т.к. разрешение будет выполняться непосредственно перед отправкой данных)
  USART1->CR1 |= USART_CR1_RE;        // Разрешение работы приёмнка
  USART1->CR1 |= USART_CR1_RXNEIE;    // Разрешаем прерывания при приходе данных в полседовательный порт

  USART1->CR2 = 0;                    // Нулевое значение регистра соответствует всем нужным установкам
  
  USART1->CR3 = 0;                    // Сброс регистра
  USART1->CR3 |= USART_CR3_DMAT;      // Включаем отправку данных от USART`а с помощью DMA

  // для USART1 необходимо брать частоту APB2 (т.к. это устройство подключено к этой шине)
  if(USART1->CR1 & USART_CR1_OVER8) // Если есть разрешение работы по 8-ми отсчётам для определения наличия низкого или высокого уровня на входе RX ноги
  {
    // Получение делителя частоты для заданной скорости передачи данных
    divider = APB2Freq >> 3; // Делим частоту на 8
    // Дробная часть делителя
    fraction = (divider%br)<<7; // Дробную часть умножаем на 128 для "вытягивания" дробной части
    fraction /= br; // Дробная часть умноженная на 128
    fraction >>= 4; // Одновременное умножение на 8 и деление на 128 ( это равносильно просто делению на 16 )
  }
  else
  {
    // Получение делителя частоты для заданной скорости передачи данных
    divider = APB2Freq >> 4; // Делим частоту на 16
    // Дробная часть делителя
    fraction = (divider%br)<<7; // Дробную часть умножаем на 128 для "вытягивания" дробной части
    fraction /= br; // Дробная часть умноженная на 128
    fraction >>= 3; // Одновременное умножение на 16 и деление на 128 ( это равносильно просто делению на 8 )
  }

  // Окончательное значение целой части делителя
  divider /= br; // Делим оставшуюся частоту на скорость передачи

  USART1->BRR = (uint16_t)(divider<<4 | fraction); // Формирования регистра скорости работы USART

  // Настройка любого потока, потом он через DMAMUX будет подключён к USART1->TDR
  DMA1_Stream0->PAR = (uint32_t)&USART1->TDR; // Адрес в который будут передаваться данные ( адрес регистра переферии )

  DMA1_Stream0->CR = 0;
  DMA1_Stream0->CR |= DMA_SxCR_MINC;      // Увеличение адреса памяти
  DMA1_Stream0->CR |= DMA_SxCR_DIR_0;     // Направление передачи из памяти в перефирию
  DMA1_Stream0->CR |= DMA_SxCR_TCIE;      // Будет возникать прерывание по окончанию передачи

  NVIC_EnableIRQ(DMA1_Stream0_IRQn);      // Разрешаем прерывания от DMA1_Stream0
  NVIC_EnableIRQ(USART1_IRQn);            // Глобальное разрешение прерывания для USART1

  DMAMUX1_Channel0->CCR = 42;            // смотри 17.3.2 DMAMUX1 mapping


  InBufHead = 0;                   // Номер ячейки в которую будут сохраняться, поступающие данные (голова буфера)
  InBufTail = 0;                   // Номер ячейки из которой можно считывать данные (хвост буфера)

  USART1->CR1 |= USART_CR1_UE; // Включение последовательного порта
}

void USART1_IRQHandler(void) // Прерывание для USART1
{
  uint8_t data,hd;
  
  if(USART1->ISR & USART_ISR_RXNE_RXFNE) // Если прерывание возникло из-за того, что пришли данные, которые можно считать, то
  {
    data = USART1->RDR; // Необходимо обязательно прочитать байт из регистра данных, чтобы сбросился флаг USART_ISR_RXNE в регистре состояния
    hd = InBufHead + 1; // Это будет будущая позиция головы, если она не наедет на хвост
    if(hd==InBufTail) // Если будущее положение головы налезает на хвост, то выходим из прерывания с увеличением кол-ва потерянных байт
    {
      InLostDataCount++; // Увеличиваем счётчик потерянных байт
      return; // Если данные сохранять некуда, то выходим т.е. будет потеря данных
    }
    BufForInData[InBufHead] = data; // Пришедший байт сопируем в буфер
    InBufHead = hd;
  }
}

void DMA_STR0_IRQHandler(void) // Обработка прерывания от DMA1 поток 0 (передающий канал)
{
  // когда в УСАРТ будут переданы все IndexForOutData байт, то возникнет это прерывание
  if(DMA1->LISR & DMA_LISR_TCIF0) // Если прерывание произошло по причине окончания передачи в 0-м потоке
  {
    DMA1->LIFCR |= DMA_LIFCR_CTCIF0;  // это действие сбросит этот флаг окончания передачи в регистре DMA1->LISR, чтобы прерывание не возникало вновь
    DMA1_Stream0->CR &= ~DMA_SxCR_EN; // Выключаем DMA поток 0
  }
}

void USART1_SendData(uint32_t len)              // Отправка BufForOutData во вне
{
  if(len>USART1_OUTBUFLEN) len = USART1_OUTBUFLEN;    // Нельзя отправить больше, чем выделенный буфер
  while(DMA1_Stream0->CR & DMA_SxCR_EN); // Пока выполняется передача данных во вне придётся подождать
  // Формирование адреса для передачи с помощью DMA и адреса для буфера, в который можно будет записывать данные
  DMA1_Stream0->M0AR = (uint32_t)CurBufForOutData;  // Адрес массива из которого будут поступать данные
  DMA1_Stream0->NDTR = len;                     // Указываем количество слов для передачи
  DMA1_Stream0->CR |= DMA_SxCR_EN; // Включаем DMA1 поток 0
  if(CurBufForOutData==BufForOutData) // Если указатель текущего буфера равен началу буфера, то
    CurBufForOutData=&BufForOutData[USART1_OUTBUFLEN]; // задаём этот адрес на середину буфера (куда указывает CurBufForOutData, туда и будет выполняться запись)
  else
    CurBufForOutData=BufForOutData; // иначе текущий буфер для записи это начало буфера BufForInData
}

uint8_t USART1_GetInDataLen(void) // Получение кол-во байт во входном буфере
{
  if(InBufTail==InBufHead) return 0;
  if(InBufTail<InBufHead) return (InBufHead-InBufTail);
  // остался вариант когда хвост больше головы (это получается, когда голова достигла конца и переместилась в начало входного буфера)
  return (USART1_INBUFLEN-InBufTail+InBufHead);
}

uint8_t USART1_ReadInBuf(uint8_t *buf, uint8_t len) // Чтение данных из входного буфера (функция вернёт кол-во байт которое было прочитано)
{
  uint8_t cnt=0;
  if(!buf){InBufTail = InBufHead; return 0;} // Если буфер не задан, то просто хвост двигается к голове (типо прочли данные)
  
  while((InBufTail!=InBufHead)&&len)
  {
    buf[cnt++] = BufForInData[InBufTail++];
    len--;
  }
  return cnt;
}

void *USART1_GetCurentOutBuf(void){ return CurBufForOutData; } // Возвращаем указатель на буфер в который можно записывать данные для их передачи по Com-порту


void USART1_printf(char *format, ...) // Процедура заменяет процедуру printf, это сделанно для того, чтобы вывод оператора printf направить в последовательный порт
{
  va_list aptr;
  uint32_t len;

  va_start(aptr, format);
  vsprintf((char *)CurBufForOutData, format, aptr);
  va_end(aptr);
  len = strlen((const char *)CurBufForOutData);
  USART1_SendData(len); // Отправляем текст
}
