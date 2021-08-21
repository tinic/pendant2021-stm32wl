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
#ifndef _I2C_H_
#define _I2C_H_

#include "main.h"
#include "bitstream.h"

#include <stdint.h>

extern "C" {
    void i2c2_peripheral_ev_irq_handler(void);
    void i2c2_peripheral_err_irq_handler(void);
};

class i2c1 {
public:
    static i2c1 &instance();

    void write(uint8_t peripheralAddr, uint8_t data[], size_t len);
    uint8_t read(uint8_t peripheralAddr, uint8_t data[], size_t len);

    uint8_t writeRead(uint8_t peripheralAddr, uint8_t writeData[], size_t writeLen, uint8_t readData[], size_t readLen);

    void setReg8(uint8_t peripheralAddr, uint8_t reg, uint8_t dat);
    uint8_t getReg8(uint8_t peripheralAddr, uint8_t reg);

    void setReg8Bits(uint8_t peripheralAddr, uint8_t reg, uint8_t mask);
    void clearReg8Bits(uint8_t peripheralAddr, uint8_t reg, uint8_t mask);

    void update();

private:
    template<typename T> void checkReady();
    template<typename T> void checkReadyReprobe();
    template<class T> void update();

    void init();
};

class i2c2 {
public:
    static i2c2 &instance();

    uint16_t SystemTime() const { return i2cRegs.fields.systemTime; }

    uint8_t Status() const { return i2cRegs.fields.status; }
    uint8_t EffectN() const { return i2cRegs.fields.effectN; }
    uint8_t Brightness() const { return i2cRegs.fields.brightness; }

    uint8_t RingColor(size_t n) const { return i2cRegs.fields.ring_color[n]; }
    uint8_t BirdColor(size_t n) const { return i2cRegs.fields.bird_color[n]; }

    uint16_t Switch1Count() const { return i2cRegs.fields.switch1Count; }
    uint16_t Switch2Count() const { return i2cRegs.fields.switch2Count; }
    uint16_t Switch3Count() const { return i2cRegs.fields.switch3Count; }
    uint16_t BootCount() const { return i2cRegs.fields.bootCount; }
    uint16_t IntCount() const { return i2cRegs.fields.intCount; }
    uint16_t DSelCount() const { return i2cRegs.fields.dselCount; }

private:

    enum State : uint8_t {
        Stop,
        WaitAddr,
        HaveAddr,
        WaitForStop
    } i2cStatus = Stop;

    static constexpr uint32_t i2c_addr = 0x33;

    void init();

    void updateDynamicPeripheralFields();

    uint8_t i2cReg;
    union I2CRegs {
        uint8_t regs[256];
        struct  __attribute__ ((__packed__)) {
            // persistent, controller
            uint8_t effectN;
            uint8_t brightness;

            uint8_t ring_color[4];
            uint8_t bird_color[4];

            uint16_t switch1Count;
            uint16_t switch2Count;
            uint16_t switch3Count;
            uint16_t bootCount;

            // persistent, peripheral
            uint16_t intCount;
            uint16_t dselCount;

            // dynamic only, controller
            uint16_t systemTime;
            uint8_t status;

            // dynamic only, peripheral
            uint8_t devEUI[8];
            uint8_t joinEUI[8];
            uint8_t appKey[16];

            uint8_t bq25895Status;
            uint8_t bq25895FaulState;
            uint8_t bq25895BatteryVoltage;
            uint8_t bq25895SystemVoltage;
            uint8_t bq25895VbusVoltage;
            uint8_t bq25895ChargeCurrent;

            uint16_t ens210Tmp;
            uint16_t ens210Hmd;

            uint16_t lsm6dsmXG;
            uint16_t lsm6dsmYG;
            uint16_t lsm6dsmZG;
            uint16_t lsm6dsmXA;
            uint16_t lsm6dsmYA;
            uint16_t lsm6dsmZA;
            uint16_t lsm6dsmTmp;

            uint16_t mmc5633njlXG;
            uint16_t mmc5633njlYG;
            uint16_t mmc5633njlZG;
            uint8_t  mmc5633njlTmp;
        } fields;
    } i2cRegs;

    int peripheral_process_addr_match(int rw);
    void peripheral_process_rx_byte(uint8_t val);
    void peripheral_process_rx_end(void);
    uint8_t peripheral_process_tx_byte(void);
    void peripheral_process_tx_end(void);

    friend void i2c2_peripheral_ev_irq_handler(void);
    friend void i2c2_peripheral_err_irq_handler(void);

    void peripheral_ev_irq_handler(void);
    void peripheral_err_irq_handler(void);
};

#endif  // #ifndef _I2C_H_
