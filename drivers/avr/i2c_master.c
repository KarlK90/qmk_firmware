/*  Copyright (C) 2019 Elia Ritterbusch
 +
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
/* Library made by: g4lvanix
 * GitHub repository: https://github.com/g4lvanix/I2C-master-lib
 */

#include <avr/io.h>
#include <util/twi.h>

#include "debug.h"
#include "i2c_master.h"
#include "timer.h"
#include "wait.h"

#ifndef F_SCL
#    define F_SCL 400000UL  // SCL frequency
#endif

#define TWBR_val (((F_CPU / F_SCL) - 16) / 2)

void i2c_init(void) {
    TWSR = 0; /* no prescaler */
    TWBR = (uint8_t)TWBR_val;

#ifdef __AVR_ATmega32A__
    // set pull-up resistors on I2C bus pins
    PORTC |= 0b11;

    // enable TWI (two-wire interface)
    TWCR |= (1 << TWEN);

    // enable TWI interrupt and slave address ACK
    TWCR |= (1 << TWIE);
    TWCR |= (1 << TWEA);
#endif
}

i2c_status_t i2c_start(uint8_t address, uint16_t timeout) {
    // reset TWI control register
    TWCR = 0;
    // transmit START condition
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

    uint16_t timeout_timer = timer_read();
    while (!(TWCR & (1 << TWINT))) {
        if ((timeout != I2C_TIMEOUT_INFINITE) && ((timer_read() - timeout_timer) >= timeout)) {
            return I2C_STATUS_TIMEOUT;
        }
    }

    // check if the start condition was successfully transmitted
    if (((TW_STATUS & 0xF8) != TW_START) && ((TW_STATUS & 0xF8) != TW_REP_START)) {
        return I2C_STATUS_ERROR;
    }

    // load slave address into data register
    TWDR = address;
    // start transmission of address
    TWCR = (1 << TWINT) | (1 << TWEN);

    timeout_timer = timer_read();
    while (!(TWCR & (1 << TWINT))) {
        if ((timeout != I2C_TIMEOUT_INFINITE) && ((timer_read() - timeout_timer) >= timeout)) {
            return I2C_STATUS_TIMEOUT;
        }
    }

    // check if the device has acknowledged the READ / WRITE mode
    uint8_t twst = TW_STATUS & 0xF8;
    if ((twst != TW_MT_SLA_ACK) && (twst != TW_MR_SLA_ACK)) {
        return I2C_STATUS_ERROR;
    }

    return I2C_STATUS_SUCCESS;
}

i2c_status_t i2c_write(uint8_t data, uint16_t timeout) {
    // load data into data register
    TWDR = data;
    // start transmission of data
    TWCR = (1 << TWINT) | (1 << TWEN);

    uint16_t timeout_timer = timer_read();
    while (!(TWCR & (1 << TWINT))) {
        if ((timeout != I2C_TIMEOUT_INFINITE) && ((timer_read() - timeout_timer) >= timeout)) {
            return I2C_STATUS_TIMEOUT;
        }
    }

    if ((TW_STATUS & 0xF8) != TW_MT_DATA_ACK) {
        return I2C_STATUS_ERROR;
    }

    return I2C_STATUS_SUCCESS;
}

int16_t i2c_read_ack(uint16_t timeout) {
    // start TWI module and acknowledge data after reception
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);

    uint16_t timeout_timer = timer_read();
    while (!(TWCR & (1 << TWINT))) {
        if ((timeout != I2C_TIMEOUT_INFINITE) && ((timer_read() - timeout_timer) >= timeout)) {
            return I2C_STATUS_TIMEOUT;
        }
    }

    // return received data from TWDR
    return TWDR;
}

int16_t i2c_read_nack(uint16_t timeout) {
    // start receiving without acknowledging reception
    TWCR = (1 << TWINT) | (1 << TWEN);

    uint16_t timeout_timer = timer_read();
    while (!(TWCR & (1 << TWINT))) {
        if ((timeout != I2C_TIMEOUT_INFINITE) && ((timer_read() - timeout_timer) >= timeout)) {
            return I2C_STATUS_TIMEOUT;
        }
    }

    // return received data from TWDR
    return TWDR;
}

i2c_status_t i2c_transmit(uint8_t address, const uint8_t* data, uint16_t length, uint16_t timeout) {
    i2c_status_t status = i2c_start(address | I2C_WRITE, timeout);

    for (uint16_t i = 0; i < length && status >= 0; i++) {
        status = i2c_write(data[i], timeout);
    }

    i2c_stop();

    return status;
}

i2c_status_t i2c_receive(uint8_t address, uint8_t* data, uint16_t length, uint16_t timeout) {
    i2c_status_t status = i2c_start(address | I2C_READ, timeout);

    for (uint16_t i = 0; i < (length - 1) && status >= 0; i++) {
        status = i2c_read_ack(timeout);
        if (status >= 0) {
            data[i] = status;
        }
    }

    if (status >= 0) {
        status = i2c_read_nack(timeout);
        if (status >= 0) {
            data[(length - 1)] = status;
        }
    }

    i2c_stop();

    return (status < 0) ? status : I2C_STATUS_SUCCESS;
}

i2c_status_t i2c_writeReg(uint8_t devaddr, uint8_t regaddr, const uint8_t* data, uint16_t length, uint16_t timeout) {
    uint16_t     timeout_timer = timer_read();
    i2c_status_t status;
    do {
        status = i2c_start(devaddr, (1 + timeout / 20));
    } while (status < 0 && timer_elapsed(timeout_timer) < timeout);

    if (status >= 0) {
        status = i2c_write(regaddr, timeout);

        for (uint16_t i = 0; i < length && status >= 0; i++) {
            status = i2c_write(data[i], timeout);
        }
    }

    i2c_stop();

    return status;
}

i2c_status_t i2c_readReg(uint8_t devaddr, uint8_t regaddr, uint8_t* data, uint16_t length, uint16_t timeout) {
    uint16_t     timeout_timer = timer_read();
    i2c_status_t status;
    do {
        status = i2c_start(devaddr, (1 + timeout / 20));
    } while (status < 0 && timer_elapsed(timeout_timer) < timeout);
    if (status < 0) {
        goto error;
    }

    status = i2c_write(regaddr, timeout);
    if (status < 0) {
        goto error;
    }

    do {
        status = i2c_start(devaddr | 0x01, (1 + timeout / 20));
    } while (status < 0 && timer_elapsed(timeout_timer) < timeout);

    for (uint16_t i = 0; i < (length - 1) && status >= 0; i++) {
        status = i2c_read_ack(timeout);
        if (status >= 0) {
            data[i] = status;
        }
    }

    if (status >= 0) {
        status = i2c_read_nack(timeout);
        if (status >= 0) {
            data[(length - 1)] = status;
        }
    }

error:
    i2c_stop();

    return (status < 0) ? status : I2C_STATUS_SUCCESS;
}

void i2c_stop(void) {
    // transmit STOP condition
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}
