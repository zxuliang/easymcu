#include <stdint.h>
#include <stm32f10x.h>
#include <ushell/ushell.h>
#include <libutils/utils.h>

/* override the weak stub */

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
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
#endif

	USART_Cmd(USART1, ENABLE);
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
	while (*str) {
		console_putchar(*str);
		str++;
	}
}

void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
		USART_ReceiveData(USART1);
		USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
	}
}
