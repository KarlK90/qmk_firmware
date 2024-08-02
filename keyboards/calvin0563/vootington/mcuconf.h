// Copyright 2025 Stefan Kerkmann (@karlk90)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include_next <mcuconf.h>

#undef STM32_ADC_USE_ADC1
#define STM32_ADC_USE_ADC1 TRUE

#undef STM32_PWM_USE_TIM1
#define STM32_PWM_USE_TIM1 TRUE

#undef STM32_GPT_USE_TIM3
#define STM32_GPT_USE_TIM3 TRUE
