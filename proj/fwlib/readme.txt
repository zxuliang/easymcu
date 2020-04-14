# place stm32 firmware here

# comment out #CFLAGS += -nostdinc

STM32F10X_HD
USE_STDPERIPH_DRIVER
USE_FULL_ASSERT

CFLAGS += -DSTM32F10X_HD
CFLAGS += -DUSE_STDPERIPH_DRIVER
CFLAGS += -DUSE_FULL_ASSERT

#define HSE_VALUE    ((uint32_t)8000000)
#define HSI_VALUE    ((uint32_t)8000000)

# app should include stm32f10x.h

stm32f10x.h
	#include "stm32f10x_conf.h"
		#include "stm32f10x_adc.h"
		#include "stm32f10x_bkp.h"
		#include "stm32f10x_can.h"
		#include "stm32f10x_cec.h"
		#include "stm32f10x_crc.h"
		#include "stm32f10x_dac.h"
		#include "stm32f10x_dbgmcu.h"
		#include "stm32f10x_dma.h"
		#include "stm32f10x_exti.h"
		#include "stm32f10x_flash.h"
		#include "stm32f10x_fsmc.h"
		#include "stm32f10x_gpio.h"
		#include "stm32f10x_i2c.h"
		#include "stm32f10x_iwdg.h"
		#include "stm32f10x_pwr.h"
		#include "stm32f10x_rcc.h"
		#include "stm32f10x_rtc.h"
		#include "stm32f10x_sdio.h"
		#include "stm32f10x_spi.h"
		#include "stm32f10x_tim.h"
		#include "stm32f10x_usart.h"
		#include "stm32f10x_wwdg.h"
		#include "misc.h"

系统时钟定义的地方：
		#define SYSCLK_FREQ_72MHz  72000000

中断向量表定义的地方
		/* #define VECT_TAB_SRAM */
		#define VECT_TAB_OFFSET  0x0

当使用“-fno-builtin”或“-ffreestanding”选项时，
你又想有选择性的使用内建（built-in）函数时，你可以定义宏（macros），例如：
#define abs(s)        __builtin_abs((n))
#define strcpy( d, s)  __builtin_strcpy( (d), (s) )


Clobber List
The clobber list should contain:
The registers modified, either explicitly or implicitly, by your code.
If your code modifies the condition code register, “cc”.
If your code modifies memory, “memory”.

char *str="Hello""linux";  <===> char *str="Hellolinux"