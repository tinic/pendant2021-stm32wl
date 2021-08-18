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
#include "./bq25895.h"
#include "./ens210.h"
#include "./lsm6dsm.h"
#include "./mmc5633njl.h"

#include <stdint.h>
#include <type_traits>

#include "main.h"
#include "app_lorawan.h"
#include "secure-element.h"
#include "user_app.h"
#include "Region.h"
#include "RegionUS915.h"
#include "LmHandler.h"

#include <memory.h>
#include <stdio.h>
#include <algorithm>


extern "C" {

extern I2C_HandleTypeDef hi2c1;

void i2c2_peripheral_ev_irq_handler(void) {
    i2c2::instance().peripheral_ev_irq_handler();
}

void i2c2_peripheral_err_irq_handler() {
    i2c2::instance().peripheral_err_irq_handler();
}
   
};

i2c1 &i2c1::instance() {
    static i2c1 i;
    static bool init = false;
    if (!init) {
        init = true;
        i.init();
    }
    return i;
}

void i2c1::init() {
    memset(this, 0, sizeof(i2c1));

    if (!BQ25895::devicePresent) {
        BQ25895::devicePresent = HAL_I2C_IsDeviceReady(&hi2c1, BQ25895::i2c_addr<<1, 8, HAL_MAX_DELAY) == HAL_OK;
        printf("BQ25895 is ready.\r\n");
    }

    if (!ENS210::devicePresent) {
        ENS210::devicePresent = HAL_I2C_IsDeviceReady(&hi2c1, ENS210::i2c_addr<<1, 8, HAL_MAX_DELAY) == HAL_OK;
        printf("ENS210 is ready.\r\n");
    }

    if (!LSM6DSM::devicePresent) {
        LSM6DSM::devicePresent = HAL_I2C_IsDeviceReady(&hi2c1, LSM6DSM::i2c_addr<<1, 8, HAL_MAX_DELAY) == HAL_OK;
        printf("LSM6DSM is ready.\r\n");
    }

    if (!MMC5633NJL::devicePresent) {
        MMC5633NJL::devicePresent = HAL_I2C_IsDeviceReady(&hi2c1, MMC5633NJL::i2c_addr<<1, 8, HAL_MAX_DELAY) == HAL_OK;
        printf("MMC5633NJL is ready.\r\n");
    }

    update();
}

void i2c1::update() {

    if (!BQ25895::devicePresent) {
        BQ25895::devicePresent = HAL_I2C_IsDeviceReady(&hi2c1, BQ25895::i2c_addr<<1, 8, HAL_MAX_DELAY) == HAL_OK;
        printf("BQ25895 is ready on reprobe.\r\n");
    }

    if (!ENS210::devicePresent) {
        ENS210::devicePresent = HAL_I2C_IsDeviceReady(&hi2c1, ENS210::i2c_addr<<1, 8, HAL_MAX_DELAY) == HAL_OK;
        printf("ENS210 is ready on reprobe.\r\n");
    }

    if (!LSM6DSM::devicePresent) {
        LSM6DSM::devicePresent = HAL_I2C_IsDeviceReady(&hi2c1, LSM6DSM::i2c_addr<<1, 8, HAL_MAX_DELAY) == HAL_OK;
        printf("LSM6DSM is ready on reprobe.\r\n");
    }

    if (!MMC5633NJL::devicePresent) {
        MMC5633NJL::devicePresent = HAL_I2C_IsDeviceReady(&hi2c1, MMC5633NJL::i2c_addr<<1, 8, HAL_MAX_DELAY) == HAL_OK;
        printf("MMC5633NJL is ready on reprobe.\r\n");
    }

    if (BQ25895::devicePresent) {
        BQ25895::instance().update();
    }

    if (ENS210::devicePresent) {
        ENS210::instance().update();
    }

    if (LSM6DSM::devicePresent) {
        LSM6DSM::instance().update();
    }

    if (MMC5633NJL::devicePresent) {
        MMC5633NJL::instance().update();
    }
}

void i2c1::write(uint8_t peripheralAddr, uint8_t data[], size_t len) {
    HAL_I2C_Master_Transmit(&hi2c1,peripheralAddr<<1, data, len, HAL_MAX_DELAY);
}

uint8_t i2c1::read(uint8_t peripheralAddr, uint8_t rdata[], size_t len) {
    HAL_I2C_Master_Receive(&hi2c1, peripheralAddr<<1, rdata, len, HAL_MAX_DELAY);
    return len;
}

uint8_t i2c1::writeRead(uint8_t peripheralAddr, uint8_t writeData[], size_t writeLen, uint8_t readData[], size_t readLen) {
    HAL_I2C_Master_Transmit(&hi2c1,peripheralAddr<<1, writeData, writeLen, HAL_MAX_DELAY);
    HAL_I2C_Master_Receive(&hi2c1, peripheralAddr<<1, readData, readLen, HAL_MAX_DELAY);
    return readLen;
}

void i2c1::setReg8(uint8_t peripheralAddr, uint8_t dataAddr, uint8_t data) {
    uint8_t set[2];
    set[0] = dataAddr;
    set[1] = data;
    HAL_I2C_Master_Transmit(&hi2c1, peripheralAddr<<1, set, 2, HAL_MAX_DELAY);
}

void i2c1::setReg8Bits(uint8_t peripheralAddr, uint8_t reg, uint8_t mask) {
    uint8_t value = getReg8(peripheralAddr, reg);
    value |= mask;
    setReg8(peripheralAddr, reg, value);
}

void i2c1::clearReg8Bits(uint8_t peripheralAddr, uint8_t reg, uint8_t mask) {
    uint8_t value = getReg8(peripheralAddr, reg);
    value &= ~mask;
    setReg8(peripheralAddr, reg, value);
}

uint8_t i2c1::getReg8(uint8_t peripheralAddr, uint8_t dataAddr) {
    uint8_t value = 0x0;
    if (HAL_I2C_Master_Transmit(&hi2c1, peripheralAddr<<1, &dataAddr, 1, HAL_MAX_DELAY) == HAL_OK &&
        HAL_I2C_Master_Receive(&hi2c1, peripheralAddr<<1, &value, 1, HAL_MAX_DELAY) == HAL_OK) {
        return value;
    }
    return 0;
}

i2c2 &i2c2::instance() {
    static i2c2 i;
    static bool init = false;
    if (!init) {
        init = true;
        i.init();
    }
    return i;
}

void i2c2::init() {
    memset(this, 0, sizeof(i2c2));

    I2C2->CR1 = I2C_CR1_STOPIE | I2C_CR1_ADDRIE | I2C_CR1_RXIE | I2C_CR1_TXIE;
    I2C2->CR2 = 0;
    I2C2->OAR1 = I2C_OAR1_OA1EN | i2c_addr << 1;
    I2C2->OAR2 = 0;
    I2C2->CR1 |= I2C_CR1_PE;

    i2cReg = 0;
    i2cStatus = Stop;

    NVIC_SetPriority(I2C2_EV_IRQn, 0);
    NVIC_EnableIRQ(I2C2_EV_IRQn);

    NVIC_SetPriority(I2C2_ER_IRQn, 0);
    NVIC_EnableIRQ(I2C2_ER_IRQn);
}

void i2c2::updateDynamicPeripheralFields() {
    // populate peripheral driven registers
    memcpy(&i2cRegs.fields.devEUI[0], SecureElementGetDevEui(), 8);
    memcpy(&i2cRegs.fields.joinEUI[0], SecureElementGetJoinEui(), 8);
    memcpy(&i2cRegs.fields.appKey[0], lora_app_key(), 16);

    i2cRegs.fields.bq25895Status = BQ25895::instance().statusRaw;
    i2cRegs.fields.bq25895FaulState = BQ25895::instance().faultStateRaw;
    i2cRegs.fields.bq25895BatteryVoltage = BQ25895::instance().batteryVoltageRaw;
    i2cRegs.fields.bq25895SystemVoltage = BQ25895::instance().systemVoltageRaw;
    i2cRegs.fields.bq25895VbusVoltage = BQ25895::instance().vbusVoltageRaw;
    i2cRegs.fields.bq25895ChargeCurrent = BQ25895::instance().chargeCurrentRaw;

    i2cRegs.fields.ens210Tmp = ENS210::instance().temperatureRaw;
    i2cRegs.fields.ens210Hmd = ENS210::instance().humidityRaw;

    i2cRegs.fields.lsm6dsmXG = LSM6DSM::instance().lsm6dsmRegs.fields.outXG;
    i2cRegs.fields.lsm6dsmYG = LSM6DSM::instance().lsm6dsmRegs.fields.outYG;
    i2cRegs.fields.lsm6dsmZG = LSM6DSM::instance().lsm6dsmRegs.fields.outZG;
    i2cRegs.fields.lsm6dsmXA = LSM6DSM::instance().lsm6dsmRegs.fields.outXA;
    i2cRegs.fields.lsm6dsmYA = LSM6DSM::instance().lsm6dsmRegs.fields.outYA;
    i2cRegs.fields.lsm6dsmZA = LSM6DSM::instance().lsm6dsmRegs.fields.outZA;
    i2cRegs.fields.lsm6dsmTmp = LSM6DSM::instance().lsm6dsmRegs.fields.outTemp;

    i2cRegs.fields.mmc5633njlXG = MMC5633NJL::instance().XGRaw();
    i2cRegs.fields.mmc5633njlYG = MMC5633NJL::instance().YGRaw();
    i2cRegs.fields.mmc5633njlZG = MMC5633NJL::instance().ZGRaw();
    i2cRegs.fields.mmc5633njlTmp = MMC5633NJL::instance().temperatureRaw();
}

int i2c2::peripheral_process_addr_match(int) {
    switch(i2cStatus) {
        case Stop: {
            i2cStatus = WaitAddr;
        } break;
        default: {
        } break;
    }
    return 0;
}

void i2c2::peripheral_process_rx_byte(uint8_t val) {
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

void i2c2::peripheral_process_rx_end(void) {
    i2cReg = 0;
    i2cStatus = Stop;
}

uint8_t i2c2::peripheral_process_tx_byte(void) {
    switch(i2cStatus) {
        case Stop: // Assume we want register 0
        case WaitAddr: // Assume we want register 0
        case HaveAddr: {
            // Update fields when we start to read from register 0 and forward
            if (i2cReg == 0) {
                updateDynamicPeripheralFields();
            }
            static_assert(sizeof(i2cRegs.regs) == 256 && 
                          std::is_same<decltype(i2cReg), uint8_t>::value);
            return i2cRegs.regs[i2cReg++];
        } break;
        default: {
        } break;
    }
    return 0;
}

void i2c2::peripheral_process_tx_end(void) {
    i2cReg = 0;
    i2cStatus = Stop;
}

void i2c2::peripheral_ev_irq_handler() {
    uint32_t isr = I2C2->ISR;
    if ((isr & I2C_ISR_ADDR) != 0) {
        I2C2->ISR = I2C_ISR_TXE; // critical
        I2C2->ICR = I2C_ICR_ADDRCF;
        peripheral_process_addr_match(0);
    }
    if ((isr & I2C_ISR_TXIS) != 0) {
        I2C2->TXDR = peripheral_process_tx_byte();
    }
    if ((isr & I2C_ISR_RXNE) != 0) {
        peripheral_process_rx_byte(I2C2->RXDR);
    }
    if ((isr & I2C_ISR_NACKF) != 0) {
        I2C2->ICR = I2C_ICR_NACKCF;
    }
    if ((isr & I2C_ISR_STOPF) != 0) {
        I2C2->ICR = I2C_ICR_STOPCF;
        I2C2->OAR1 &= ~I2C_OAR1_OA1EN;
        if (I2C2->ISR & I2C_ISR_DIR) {
            peripheral_process_tx_end();
        } else {
            peripheral_process_rx_end();
        }
        I2C2->OAR1 |= I2C_OAR1_OA1EN;
    }
}

void i2c2::peripheral_err_irq_handler() {
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
