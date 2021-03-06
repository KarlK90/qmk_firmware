#pragma once

/* GD32VF103 has the same API as STM32F103, but uses different names for literally the same thing.
 * As of 4.7.2021 QMK is tailored to use STM32 defines/names, for compatibility sake
 * we just redefine the GD32 names. */

// Close your eyes kids.

// Clock redefines.
#define STM32_SYSCLK GD32_SYSCLK

// GPIO redefines.
#define PAL_MODE_STM32_ALTERNATE_OPENDRAIN PAL_MODE_GD32_ALTERNATE_OPENDRAIN
#define PAL_MODE_STM32_ALTERNATE_PUSHPULL PAL_MODE_GD32_ALTERNATE_PUSHPULL

// DMA redefines.
#define STM32_DMA_STREAM(stream) GD32_DMA_STREAM(stream)
#define STM32_DMA_STREAM_ID(peripheral, channel) GD32_DMA_STREAM_ID(peripheral - 1, channel - 1)
#define STM32_DMA_CR_DIR_M2P GD32_DMA_CTL_DIR_M2P
#define STM32_DMA_CR_PSIZE_WORD GD32_DMA_CTL_PWIDTH_WORD
#define STM32_DMA_CR_MSIZE_WORD GD32_DMA_CTL_MWIDTH_WORD
#define STM32_DMA_CR_MINC GD32_DMA_CTL_MNAGA
#define STM32_DMA_CR_CIRC GD32_DMA_CTL_CMEN
#define STM32_DMA_CR_PL GD32_DMA_CTL_PRIO
#define STM32_DMA_CR_CHSEL GD32_DMA_CTL_CHSEL
#define cr1 ctl0
#define cr2 ctl1
#define cr3 ctl2

// Serial USART redefines.
#if !defined(SERIAL_USART_CR1)
#    define SERIAL_USART_CR1 (USART_CTL0_PCEN | USART_CTL0_PM | USART_CTL0_WL)  // parity enable, odd parity, 9 bit length
#endif

#if !defined(SERIAL_USART_CR2)
#    define SERIAL_USART_CR2 (USART_CTL1_STB_1)  // 2 stop bits
#endif

#if !defined(SERIAL_USART_CR3)
#    define SERIAL_USART_CR3 0x0
#endif
#define USART_CR3_HDSEL USART_CTL2_HDEN
#define CCR CHCV
#define dier dmainten

// SPI redefines.
#define SPI_CR1_LSBFIRST SPI_CTL0_LF
#define SPI_CR1_CPHA SPI_CTL0_CKPH
#define SPI_CR1_CPOL SPI_CTL0_CKPL
#define SPI_CR1_BR_0 SPI_CTL0_PSC_0
#define SPI_CR1_BR_1 SPI_CTL0_PSC_1
#define SPI_CR1_BR_2 SPI_CTL0_PSC_2
