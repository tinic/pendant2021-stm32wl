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
#include <type_traits>

#include "main.h"
#include "app_lorawan.h"
#include "secure-element.h"
#include "user_app.h"

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

    I2C2->CR1 = I2C_CR1_STOPIE | I2C_CR1_ADDRIE | I2C_CR1_RXIE | I2C_CR1_TXIE;
    I2C2->CR2 = 0;
    I2C2->OAR1 = I2C_OAR1_OA1EN | i2c_addr << 1;
    I2C2->OAR2 = 0;
    I2C2->CR1 |= I2C_CR1_PE;

    i2cReg = 0;
    i2cStatus = Stop;

	update();

    NVIC_SetPriority(I2C2_EV_IRQn, 0);
    NVIC_EnableIRQ(I2C2_EV_IRQn);

    NVIC_SetPriority(I2C2_ER_IRQn, 0);
    NVIC_EnableIRQ(I2C2_ER_IRQn);
}

void i2c::update() {
    // populate slave driven registers
    memcpy(&i2cRegs.devEUI[0], SecureElementGetDevEui(), 8);
    memcpy(&i2cRegs.joinEUI[0], SecureElementGetJoinEui(), 8);
    memcpy(&i2cRegs.appKey[0], lora_app_key(), 16);
}

static float u82f(uint8_t v, float min, float max) {
    return ((float(v) * ( 1.0f / 255.0f ) * (max - min) ) + min);
}

uint8_t i2c::Effect() const {
    return i2cRegs.effectN;
}

float i2c::Brightness() const {
    return u82f(i2cRegs.brightness, 0.0f, 1.0f);
}

float i2c::BatteryVoltage() const {
    return u82f(i2cRegs.batteryVoltage, 2.7f, 4.2f);
}

float i2c::SystemVoltage() const {
    return u82f(i2cRegs.systemVoltage, 2.7f, 4.2f);
}

float i2c::VBUSVoltage() const {
    return u82f(i2cRegs.vbusVoltage, 0.0f, 5.5f);
}

float i2c::ChargeCurrent() const {
    return u82f(i2cRegs.chargeCurrent, 0.0f, 1000.0f);
}

float i2c::Temperature() const {
    return u82f(i2cRegs.temperature, 0.0f, 50.0f);
}

float i2c::Humidity() const {
    return u82f(i2cRegs.humidity, 0.0f, 1.0f);
}

void i2c::encodeForLora(OutBitStream &bitstream) {

	bitstream.FlushBits();

    // These are likely to be 0 or low
    bitstream.PutExpGolomb(i2cRegs.systemTime);
    bitstream.PutExpGolomb(i2cRegs.effectN);
    bitstream.PutExpGolomb(i2cRegs.chargeCurrent);
    bitstream.PutExpGolomb(i2cRegs.vbusVoltage);

	bitstream.FlushBits();

    // Unlikely to be zero or low
    bitstream.PutUint8(i2cRegs.temperature);
    bitstream.PutUint8(i2cRegs.humidity);
    bitstream.PutUint8(i2cRegs.brightness);
    bitstream.PutUint8(i2cRegs.batteryVoltage);
    bitstream.PutUint8(i2cRegs.systemVoltage);
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
            i2cRegs.regs[i2cReg] = val;
        } break;
        default: {
        } break;
    }
}

void i2c::slave_process_rx_end(void) {
    i2cReg = 0;
    i2cStatus = Stop;
}

uint8_t i2c::slave_process_tx_byte(void) {
    switch(i2cStatus) {
        case Stop: // Assume we want register 0
        case WaitAddr: // Assume we want register 0
        case HaveAddr: {
            static_assert(sizeof(i2cRegs.regs) == 256 && 
                          std::is_same<decltype(i2cReg), uint8_t>::value);
            return i2cRegs.regs[i2cReg++];
        } break;
        default: {
        } break;
    }
    return 0;
}

void i2c::slave_process_tx_end(void) {
    i2cReg = 0;
    i2cStatus = Stop;
}

void i2c::slave_ev_irq_handler() {
    uint32_t isr = I2C2->ISR;
    if ((isr & I2C_ISR_ADDR) != 0) {
        I2C2->ISR = I2C_ISR_TXE; // critical
        I2C2->ICR = I2C_ICR_ADDRCF;
        slave_process_addr_match(0);
    }
    if ((isr & I2C_ISR_TXIS) != 0) {
        I2C2->TXDR = slave_process_tx_byte();
    }
    if ((isr & I2C_ISR_RXNE) != 0) {
        slave_process_rx_byte(I2C2->RXDR);
    }
    if ((isr & I2C_ISR_NACKF) != 0) {
        I2C2->ICR = I2C_ICR_NACKCF;
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
    uint32_t isr = I2C2->ISR;
    if ((isr & I2C_ISR_ARLO) != 0) {
        I2C2->ICR = I2C_ICR_ARLOCF;
    }
    if ((isr & I2C_ISR_BERR) != 0) {
        I2C2->ICR = I2C_ICR_BERRCF;
    }
    if ((isr & I2C_ISR_OVR) != 0) {
        I2C2->ICR = I2C_ICR_OVRCF;
    }
}
