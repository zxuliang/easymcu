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
