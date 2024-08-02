// Copyright 2025 Stefan Kerkmann (@karlk90)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define WAIT_US_TIMER GPTD3

#define WS2812_PWM_DRIVER PWMD1
#define WS2812_PWM_CHANNEL 1
#define WS2812_PWM_DMA_STREAM STM32_DMA1_STREAM2
#define WS2812_PWM_DMA_CHANNEL 2
#define WS2812_PWM_DMAMUX_ID STM32_DMAMUX1_TIM1_UP
