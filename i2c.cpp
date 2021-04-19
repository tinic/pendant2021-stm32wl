/*
Copyright 2021 Tinic Uro
Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:
The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "./i2c.h"

#include <stdint.h>

#include "main.h"
#include "app_lorawan.h"

#include <memory.h>
#include <stdio.h>


extern "C" {

void i2c_slave_ev_irq_handler(void) {
    i2c::instance().slave_ev_irq_handler();
}

void i2c_slave_err_irq_handler() {
    i2c::instance().slave_err_irq_handler();
}
   
};

i2c &i2c::instance() {
	static i2c i;
	static bool init = false;
	if (!init) {
		init = true;
		i.init();
	}
	return i;
}

void i2c::init() {
	memset(this, 0, sizeof(i2c));
	update();

    I2C2->CR1 = I2C_CR1_STOPIE | I2C_CR1_ADDRIE | I2C_CR1_RXIE | I2C_CR1_TXIE;
    I2C2->CR2 = 0;
    I2C2->OAR1 = I2C_OAR1_OA1EN | i2c_addr << 1;
    I2C2->OAR2 = 0;
    I2C2->CR1 |= I2C_CR1_PE;

    NVIC_SetPriority(I2C2_EV_IRQn, 0);
    NVIC_EnableIRQ(I2C2_EV_IRQn);

    NVIC_SetPriority(I2C2_ER_IRQn, 0);
    NVIC_EnableIRQ(I2C2_ER_IRQn);
}

void i2c::update() {
}

int i2c::slave_process_addr_match(int) {
    switch(i2cStatus) {
        case Stop: {
            i2cStatus = WaitAddr;
        } break;
        default: {
        } break;
    }
    return 0;
}

void i2c::slave_process_rx_byte(uint8_t val) {
    switch(i2cStatus) {
        case Stop:
        case WaitAddr: {
            i2cStatus = HaveAddr;
            i2cReg = val;
        } break;
        case HaveAddr: {
            i2cStatus = WaitForStop;
            i2cRegBank[i2cReg] = val;
        } break;
        default: {
        } break;
    }
}

void i2c::slave_process_rx_end(void) {
    i2cStatus = Stop;
}

uint8_t i2c::slave_process_tx_byte(void) {
    switch(i2cStatus) {
        case Stop:
        case WaitAddr:
        case HaveAddr: {
            static_assert(sizeof(i2cRegBank) == 256 && 
                          sizeof(i2cReg) == 1);
            return i2cRegBank[i2cReg++];
        } break;
        default: {
        } break;
    }
    return 0;
}

void i2c::slave_process_tx_end(void) {
    i2cStatus = Stop;
}

void i2c::slave_ev_irq_handler() {
    uint32_t isr = I2C2->ISR;
    if ((isr & I2C_ISR_ADDR) != 0) {
        I2C2->ISR = I2C_ISR_TXE;
        I2C2->ICR = I2C_ICR_ADDRCF;
        slave_process_addr_match(0);
    }
    if ((isr & I2C_ISR_TXIS) != 0) {
        I2C2->TXDR = slave_process_tx_byte();
    }
    if ((isr & I2C_ISR_RXNE) != 0) {
        slave_process_rx_byte(I2C2->RXDR);
    }
    if ((isr & I2C_ISR_ARLO) != 0) {
        I2C2->ICR = I2C_ICR_ARLOCF;
    }
    if ((isr & I2C_ISR_NACKF) != 0) {
        I2C2->ICR = I2C_ICR_NACKCF;
    }
    if ((isr & I2C_ICR_BERRCF) != 0) {
        I2C2->ICR = I2C_ICR_BERRCF;
    }
    if ((isr & I2C_ICR_OVRCF) != 0) {
        I2C2->ICR = I2C_ICR_OVRCF;
    }
    if ((isr & I2C_ISR_STOPF) != 0) {
        I2C2->ICR = I2C_ICR_STOPCF;
        I2C2->OAR1 &= ~I2C_OAR1_OA1EN;
        if (I2C2->ISR & I2C_ISR_DIR) {
            slave_process_tx_end();
        } else {
            slave_process_rx_end();
        }
        I2C2->OAR1 |= I2C_OAR1_OA1EN;
    }
}

void i2c::slave_err_irq_handler() {
    printf("slave_err_irq_handler!\r\n");
}
