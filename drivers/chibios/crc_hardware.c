/* Copyright 2021 QMK
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "crc.h"

#if HAL_USE_CRC
#    if !defined(CRC_DRIVER)
#        define CRC_DRIVER CRCD1
#    endif

#    if defined(GD32VF103)
#        define CRC_RESULT_REGISTER CRC_DRIVER.crc->DATA
#    else
#        define CRC_RESULT_REGISTER CRC_DRIVER.crc->DR
#    endif

#    if !(defined(CRC_CONFIG) && ((STM32_CRC_PROGRAMMABLE == TRUE) || (GD32_CRC_PROGRAMMABLE == TRUE)))
#        define CRC_CONFIG NULL  // NULL == default config is used
#    endif

/* Prototypes for internal chibios-contrib HAL functions. */
void _crc_lld_calc_byte(CRCDriver *crcp, uint8_t data);
void _crc_lld_calc_halfword(CRCDriver *crcp, uint16_t data);
void _crc_lld_calc_word(CRCDriver *crcp, uint32_t data);

void crc_init(void) { crcStart(&CRC_DRIVER, CRC_CONFIG); }

#    if STM32_CRC_PROGRAMMABLE == TRUE

uint8_t crc8(const void *data, size_t len) {
    crcAcquireUnit(&CRC_DRIVER);
    crcReset(&CRC_DRIVER);
    uint8_t crc = crcCalc(&CRC_DRIVER, len, data);
    crcReleaseUnit(&CRC_DRIVER);
    return crc;
}

#    else

uint8_t crc8(const void *data, size_t len) {
    crcAcquireUnit(&CRC_DRIVER);
    crcReset(&CRC_DRIVER);

    while (len > 3) {
        _crc_lld_calc_word(&CRC_DRIVER, *(uint32_t *)data);
        data += 4;
        len -= 4;
    }
    while (len > 1) {
        _crc_lld_calc_halfword(&CRC_DRIVER, *(uint16_t *)data);
        data += 2;
        len -= 2;
    }
    while (len > 0) {
        _crc_lld_calc_byte(&CRC_DRIVER, *(uint8_t *)data);
        data += 1;
        len -= 1;
    }

    uint8_t crc = (uint8_t)(CRC_RESULT_REGISTER);

    crcReleaseUnit(&CRC_DRIVER);
    return crc;
}

#    endif
#endif