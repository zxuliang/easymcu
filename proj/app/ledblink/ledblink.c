#include <stdint.h>
#include <stm32f10x.h>
#include <ushell/ushell.h>
#include <libutils/utils.h>


static void bsp_busy_wait(uint32_t ticks);
static void bsp_clock_init(void);
static void bsp_led_init(void);
static void bsp_init(void);
static int get_value(void);

/* override weak Function */
void console_init(void);
void console_puts(const char *str);
void console_putchar(char c);
unsigned char console_getchar(void);

#define PACKET_SIZE (64)
static uint8_t txbuf[PACKET_SIZE] ALIGN(4);
static uint8_t rxbuf[PACKET_SIZE] ALIGN(4);

static SPI_InitTypeDef SPI_InitStructure;
static DMA_InitTypeDef DMA_InitStructure;
static GPIO_InitTypeDef GPIO_InitStructure;


static void bspi_spi_init(void)
{
	uint16_t temp = 0;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* NSS */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);


	SPI_Cmd(SPI1, DISABLE);
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler =SPI_BaudRatePrescaler_8;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI1, &SPI_InitStructure);
	SPI_CalculateCRC(SPI1, DISABLE);
	SPI_NSSInternalSoftwareConfig(SPI1, SPI_NSSInternalSoft_Set);
	SPI_Cmd(SPI1, ENABLE);


	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&SPI1->DR);
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_InitStructure.DMA_BufferSize = PACKET_SIZE;

	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)txbuf;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_Init(DMA1_Channel3, &DMA_InitStructure);

	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)rxbuf;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_Init(DMA1_Channel2, &DMA_InitStructure);

	/* Enable DMA TX_RX Channel */
	DMA_Cmd(DMA1_Channel2, ENABLE);
	DMA_Cmd(DMA1_Channel3, ENABLE);

	SPI_I2S_ReceiveData(SPI1);
}


/**************************cmd_spi*********************************/
#define HEAD_MAGIC 0xc3

uint16_t bsp_fill_data(uint8_t *pbuf, uint32_t len)
{
 unsigned int i = 0;
 unsigned short chksum = 0;

 if ((len < 2))
 {
  printk("At least 3 bytes need transfer \n");
  return -1;
 }

 for (i = 0; i < (len - 2); i++)
 {
  pbuf[i] = HEAD_MAGIC + (i & 0xFF);
  chksum += pbuf[i];
 }
 pbuf[len - 2] = (chksum & 0xff); /* low bytes */
 pbuf[len - 1] = (chksum >> 8); /* high bytes */

 return chksum;
}

void bsp_dump_data(uint8_t *pbuf, uint32_t len)
{
 uint32_t i = 0;
 for (i = 0; i < len; i++)
 {
  if (i && ((i & 7) == 0))
  {
   printk(" \n");
  }
  printk("0x%02x ", pbuf[i]);
 }
 printk(" \n");
}

uint16_t bsp_check_data(unsigned char *pbuf, unsigned int len)
{
 uint32_t i = 0;
 uint32_t calsum = 0;

 for (i = 0; i < (len - 2); i++)
 {
  calsum += pbuf[i];
 }
 calsum &= 0xFFFF;
 return calsum;
}

int do_spi(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
 uint16_t tx_chksum = 0;
 uint16_t rx_chksum = 0;
 uint32_t cnt = 1;

 if (argc > 1) {
  cnt = simple_strtoul (argv[1], NULL, 10);
 }

 tx_chksum = bsp_fill_data(txbuf, PACKET_SIZE);
 printk("fill with cheksum: 0x%x \n", tx_chksum);

 /* Trigger SPI_DMA_TX/RX Req */


 for (; cnt > 0; cnt--) {
  SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, ENABLE);
  while(DMA_GetCurrDataCounter(DMA1_Channel2) !=0) {
  		printk(" DMA_CH2_RSVD = %u bytes \n", DMA_GetCurrDataCounter(DMA1_Channel2));
  }
  SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, DISABLE);

  DMA_Cmd(DMA1_Channel2, DISABLE);
  DMA_SetCurrDataCounter(DMA1_Channel2, PACKET_SIZE);
  DMA1_Channel2->CPAR = (uint32_t )&SPI1->DR;
  DMA1_Channel2->CMAR = (uint32_t)rxbuf;
  DMA_Cmd(DMA1_Channel2, ENABLE);

  DMA_Cmd(DMA1_Channel3, DISABLE );
  DMA_SetCurrDataCounter(DMA1_Channel3,PACKET_SIZE);
  DMA1_Channel2->CPAR = (uint32_t )&SPI1->DR;
  DMA1_Channel2->CMAR = (uint32_t)txbuf;
  DMA_Cmd(DMA1_Channel3, ENABLE);

  printk("spi test done %u \n", cnt);
 }

 printk("spi test done \n");

 return 0;
}


U_BOOT_CMD(
 spi,   CFG_MAXARGS,    1,  do_spi,
 "spi loopcnt --- spi transfer loopcnt times \n",
 "use spi to tranfer data"
);




int led_blink(void)
{
	GPIO_ResetBits(GPIOD, GPIO_Pin_2);
	GPIO_SetBits(GPIOA, GPIO_Pin_8);
	bsp_busy_wait(60);
	GPIO_SetBits(GPIOD, GPIO_Pin_2);
	GPIO_ResetBits(GPIOA, GPIO_Pin_8);
	bsp_busy_wait(60);

	return 0;
}

/***********************APP***********************/

int main(void)
{
	int len = 0;
	bsp_init();

	for (;;) {
		len = readline ("easymcu $");
		if(len > 0){
			run_command (console_buffer, 0);
		}
	}

	return 0;
}

void bsp_busy_wait(uint32_t ticks)
{
	uint32_t current_time = jiffies;
	while ((uint32_t)(jiffies - current_time) < ticks) {
	}
}

void console_putchar(char c)
{
	if ('\n' == c) {
		USART_SendData(USART1, (uint16_t)'\r');
	}
	USART_SendData(USART1, (uint16_t)c);
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
	{
		/* wait empty */
	}
}

unsigned char console_getchar(void)
{
	unsigned char data = 0;
	while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);
	data = (uint8_t)USART_ReceiveData(USART1);
	return data;
}


void console_puts(const char *str)
{
//	uint32_t flags;
//	flags = local_irq_save();
	while (*str) {
		console_putchar(*str);
		str++;
	}
//	local_irq_restore(flags);
}

void bsp_clock_init(void)
{
	RCC_ClocksTypeDef RCC_ClocksStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	/* configure systick period 10ms */
	RCC_GetClocksFreq(&RCC_ClocksStructure);
	SysTick_Config(RCC_ClocksStructure.HCLK_Frequency/100);
}

void NVIC_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 14;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void console_init(void)
{
	GPIO_InitTypeDef gpio_console;
	USART_InitTypeDef USART_InitStructure;

	/* PA9 -TX */
	gpio_console.GPIO_Speed = GPIO_Speed_10MHz;
	gpio_console.GPIO_Pin =GPIO_Pin_9;
	gpio_console.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpio_console);

	/* PA10 -RX */
	gpio_console.GPIO_Pin =GPIO_Pin_10;
	gpio_console.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio_console);

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);

#if 0
/* using inerrupt to transmit data */
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
/* uisng intertupt to receive data */
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
#endif

	USART_Cmd(USART1, ENABLE);
}

void bsp_led_init(void)
{
	GPIO_InitTypeDef led;

	/* led 0 */
	led.GPIO_Speed = GPIO_Speed_10MHz;
	led.GPIO_Pin =GPIO_Pin_8;
	led.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &led);

	/* led 1 */
	led.GPIO_Pin =GPIO_Pin_2;
	GPIO_Init(GPIOD, &led);
}

void bsp_init(void)
{
	bsp_clock_init();
	bsp_led_init();
	console_init();
	NVIC_Configuration();
	bspi_spi_init();
}

void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
		USART_ReceiveData(USART1);
		USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
	}
}

int get_value(void)
{
	int value = 0;
	__asm__ volatile ("mov %0, #8":"=r"(value)::"memory");
	return value;
}


int do_ledblink (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	uint32_t cnt = 1;
	if (argc > 1) {
		cnt = simple_strtoul (argv[1], NULL, 10);
	}

	while (cnt--) {
		led_blink();
	}

	printk("ledblink done \n");

	return 0;
}


U_BOOT_CMD(
	ledblink,	CFG_MAXARGS,	1,	do_ledblink,
	"help    - blink led \n",
);
