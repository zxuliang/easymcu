#include <stdint.h>
#include <stm32f10x.h>
#include <ushell/ushell.h>
#include <libutils/utils.h>

static int bsp_clock_init(void);
static int bsp_led_init(void);
static int bsp_intx_init(void);
static int bsp_spi_init(void);
static void bsp_init(void);

#define PACKET_SIZE	(64)
static uint8_t txbuf[PACKET_SIZE] ALIGN(4);
static uint8_t rxbuf[PACKET_SIZE] ALIGN(4);
#define HEAD_MAGIC (0xc3)


/*************************debug control*******************/

#define DEBUG_LEVEL_VERBOSE (3)
#define DEBUG_LEVEL_ERROR     (1)
#define DEBUG_LEVEL_DISABLE (0)

/* only error then print message */
static unsigned int debug_level = DEBUG_LEVEL_ERROR + 1;



static GPIO_InitTypeDef GPIO_InitStructure;
static SPI_InitTypeDef SPI_InitStructure;
static DMA_InitTypeDef DMA_InitStructure;
static NVIC_InitTypeDef NVIC_InitStructure;
static RCC_ClocksTypeDef RCC_ClocksStructure;
/***********************APP***********************/

int main(void)
{
	int len = 0;
	memset(txbuf, 0, sizeof(txbuf));
	memset(rxbuf, 0, sizeof(rxbuf));

	bsp_init();

	for (;;) {
		len = readline ("easymcu $ ");
		if(len > 0){
			run_command (console_buffer, 0);
		}
	}

	return 0;
}

void bsp_init(void)
{
	bsp_clock_init();
	bsp_led_init();
	console_init();
//	bsp_intx_init();
	bsp_spi_init();
}

int bsp_clock_init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);

	/* configure systick period 10ms */
	RCC_GetClocksFreq(&RCC_ClocksStructure);
	SysTick_Config(RCC_ClocksStructure.HCLK_Frequency/100);

	return 0;
}

int bsp_intx_init(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

	/* USART1_IRQ */
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 14;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* DMA1_chan2_IRQ */
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	return 0;
}

int bsp_led_init(void)
{
	/* led R */
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* led B */
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_2;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	return 0;
}

int bsp_spi_init(void)
{
	/* SPI1_CS: GPIO_Pin_2
	 * SPI1_SCLK: GPIO_Pin_5
	 * SPI1_MISO: GPIO_Pin_6
	 * SPI1_MOSI: GPIO_Pin_7
	 */

	/* CS: CS=1 init */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_SetBits(GPIOA, GPIO_Pin_2);

	/* SCLK, MOSI: as AF_PP */
	GPIO_InitStructure.GPIO_Pin =   GPIO_Pin_5 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* MISO: In_floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*
	 * spi_tx_rx_fullduplex
	 * spi-bit-per-word = 8bit
	 * spi-mode: SPI_MODE_3
	 * spi_clk: 9MHHZ
	 * no_crc check.
	*/
	SPI_Cmd(SPI1, DISABLE);
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;	/* must be */
	SPI_InitStructure.SPI_BaudRatePrescaler =SPI_BaudRatePrescaler_8;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI1, &SPI_InitStructure);
	SPI_CalculateCRC(SPI1, DISABLE);
	SPI_NSSInternalSoftwareConfig(SPI1, SPI_NSSInternalSoft_Set); /* must be */
	SPI_I2S_ReceiveData(SPI1);	/* dummy read to clean */

	/* spi dma config */
	DMA_DeInit(DMA1_Channel2);
	DMA_DeInit(DMA1_Channel3);
	DMA_InitStructure.DMA_PeripheralBaseAddr =  (uint32_t)(&SPI1->DR);
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_InitStructure.DMA_BufferSize = PACKET_SIZE;

	/* SPI_DMA_TX : MEM->Peripheral_as_DST, on DMA1-chan3 */
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)txbuf;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_Init(DMA1_Channel3, &DMA_InitStructure);

	/* SPI_DMA_RX: Peripheral_as_SRC->MEM, on DMA1-chan2 */
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)rxbuf;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_Init(DMA1_Channel2, &DMA_InitStructure);

#if 0
	/* interrupt: SPI_RX_DMA_TC <-> dma1_chn2 */
	DMA_ITConfig(DMA1_Channel2, DMA_IT_TC, ENABLE);
#endif
	return 0;
}

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
	pbuf[len - 1] = (chksum >> 8);   /* high bytes */

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

int bsp_check_data(unsigned char *pbuf, unsigned int len)
{
	uint32_t i = 0;
	uint32_t calsum = 0;
	uint16_t chksum = 0;

	for (i = 0; i < (len - 2); i++)
	{
		calsum += pbuf[i];
	}

	calsum &= 0xFFFF;
	chksum = ((pbuf[len - 1]) << 8) + pbuf[len - 2];

	if (calsum != chksum) {
		return -1;
	} else {
		return 0;
	}
}



/*********************setting debuglevel****************************/
int do_debug(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	if (argc > 1) {
		debug_level = simple_strtoul (argv[1], NULL, 10);
	}

	printk("setting debug level %u \n", debug_level);
	return 0;
}

U_BOOT_CMD(
	debug,	CFG_MAXARGS,	1,	do_debug,
	"debug level --- set debug level \n",
	"0 disable all print during spi test \n"
	"2 enable print when error happend spi test \n"
	"4 enable verbose during spi test \n"
);

/**************************cmd_spi*********************************/
int do_spi(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	int32_t err = 0;
	uint32_t cnt = 1;
	uint32_t tx_chksum = 0;


	if (argc > 1) {
		cnt = simple_strtoul (argv[1], NULL, 10);
	}

	tx_chksum = bsp_fill_data(txbuf, PACKET_SIZE);
	printk("fill with cheksum: 0x%x \n", tx_chksum);

	SPI_Cmd(SPI1, ENABLE);
	SPI_I2S_ReceiveData(SPI1);		/* dummy read to clean */
	GPIO_ResetBits(GPIOA, GPIO_Pin_2);	/* CS = 0 */
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, ENABLE);

	for (; cnt > 0; cnt--) {
		GPIO_ResetBits(GPIOA, GPIO_Pin_2);		/* CS = 0 */
		DMA_Cmd(DMA1_Channel2, ENABLE);
		DMA_Cmd(DMA1_Channel3, ENABLE);

		while(DMA_GetCurrDataCounter(DMA1_Channel2) > 0);

		GPIO_SetBits(GPIOA, GPIO_Pin_2); 		/* CS = 1 */

		err = bsp_check_data(rxbuf, PACKET_SIZE);
		if (err) {
			if (debug_level > DEBUG_LEVEL_ERROR) {
				printk("master receive data mismatch \n");
			}
		}

		DMA_Cmd(DMA1_Channel3, DISABLE);
		DMA_Cmd(DMA1_Channel2, DISABLE);
		DMA_SetCurrDataCounter(DMA1_Channel3, PACKET_SIZE);
		DMA_SetCurrDataCounter(DMA1_Channel2, PACKET_SIZE);

		if (debug_level > DEBUG_LEVEL_VERBOSE) {
			bsp_dump_data(rxbuf, PACKET_SIZE);
		}
	}

	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, DISABLE);
	SPI_Cmd(SPI1, DISABLE);
	GPIO_SetBits(GPIOA, GPIO_Pin_2);

	printk("spi test done \n");

	return 0;
}


U_BOOT_CMD(
	spi,	CFG_MAXARGS,	1,	do_spi,
	"spi loopcnt --- spi transfer loopcnt times \n",
	"use spi to tranfer data"
);

/**************************cmd_led*********************************/
void ledblink(void)
{
	GPIO_ResetBits(GPIOD, GPIO_Pin_2);
	GPIO_SetBits(GPIOA, GPIO_Pin_8);
	delayms(60);
	GPIO_SetBits(GPIOD, GPIO_Pin_2);
	GPIO_ResetBits(GPIOA, GPIO_Pin_8);
	delayms(60);
}

int do_ledblink (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	uint32_t cnt = 1;
	if (argc > 1) {
		cnt = simple_strtoul (argv[1], NULL, 10);
	}

	while (cnt--) {
		ledblink();
	}

	printk("ledblink done \n");

	return 0;
}


U_BOOT_CMD(
	ledblink,	CFG_MAXARGS,	1,	do_ledblink,
	"help    - blink led \n",
);
