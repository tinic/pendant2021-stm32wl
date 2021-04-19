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

#include <stdint.h>

extern "C" {
    void i2c_slave_ev_irq_handler(void);
    void i2c_slave_err_irq_handler(void);
};

class i2c {
public:
	static i2c &instance();

	void update();

    void set(uint8_t _i2cReg, uint8_t data) { i2cRegBank[_i2cReg] = data; }
    uint8_t get(uint8_t _i2cReg) const { return i2cRegBank[_i2cReg]; }

private:

    enum State : uint8_t {
        Stop,
        WaitAddr,
        HaveAddr,
        WaitForStop
    } i2cStatus = Stop;

    static constexpr uint32_t i2c_addr = 0x33;

	void init();

    uint8_t i2cReg;
    uint8_t i2cRegBank[256];

    int slave_process_addr_match(int rw);
    void slave_process_rx_byte(uint8_t val);
    void slave_process_rx_end(void);
    uint8_t slave_process_tx_byte(void);
    void slave_process_tx_end(void);

    friend void i2c_slave_ev_irq_handler(void);
    friend void i2c_slave_err_irq_handler(void);

    void slave_ev_irq_handler(void);
    void slave_err_irq_handler(void);
};

#endif  // #ifndef _I2C_H_
