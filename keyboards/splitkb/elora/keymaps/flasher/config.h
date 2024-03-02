/* Copyright 2023 splitkb.com <support@splitkb.com>
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

#pragma once

// Myriad boilerplate
#define MYRIAD_ENABLE

// Debounce reduces chatter (unintended double-presses) - set 0 if debouncing is not needed
#define DEBOUNCE 5

#define DEBUG_EEPROM_OUTPUT

// https://datasheet.lcsc.com/lcsc/2109141830_Zetta-ZD24C08A-STGMT_C2896640.pdf
#define EXTERNAL_EEPROM_PAGE_SIZE 16
#define EXTERNAL_EEPROM_BYTE_COUNT 8192
#define EXTERNAL_EEPROM_I2C_BASE_ADDRESS (0x50 << 1)
#define EXTERNAL_EEPROM_ADDRESS_SIZE 1
#define EXTERNAL_EEPROM_WRITE_TIME 5
