#include <stdint.h>
#include <stm32f10x.h>
#include <libutils/libutils.h>


static void bsp_busy_wait(uint32_t ticks);
static void bsp_clock_init(void);
static void bsp_led_init(void);
static void bsp_putc(char c);
static void bsp_puts(const char *str);
static void bsp_init(void);
static int get_value(void);

/* override weak Function */
void hw_console_output(const char *str);

/***********************APP***********************/

int main(void)
{

	bsp_init();

	while (1)
	{
		GPIO_ResetBits(GPIOD, GPIO_Pin_2);
		GPIO_SetBits(GPIOA, GPIO_Pin_8);
		bsp_busy_wait(60);
		GPIO_SetBits(GPIOD, GPIO_Pin_2);
		GPIO_ResetBits(GPIOA, GPIO_Pin_8);
		bsp_busy_wait(60);
		kprint("This is just testB %#x \n", get_value());
	}

	return 0;
}



void bsp_busy_wait(uint32_t ticks)
{
	uint32_t current_time = jiffies;
	while ((uint32_t)(jiffies - current_time) < ticks) {
	}
}

void bsp_putc(char c)
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

void bsp_puts(const char *str)
{
	while (*str) {
		bsp_putc(*str);
		str++;
	}
}

void hw_console_output(const char *str)
{
	uint32_t flags;
	flags = local_irq_save();
	bsp_puts(str);
	local_irq_restore(flags);
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

void bsp_hw_console_init(void)
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
	bsp_hw_console_init();
}

int get_value(void)
{
	int value = 0;
	__asm__ volatile ("mov %0, #8":"=r"(value)::"memory");
	return value;
}
