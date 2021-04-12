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

class i2c {
public:
	static i2c &instance();

	void update();

    void set(uint8_t _i2cReg, uint8_t data) { i2cRegBank[_i2cReg] = data; }
    uint8_t get(uint8_t _i2cReg) const { return i2cRegBank[_i2cReg]; }

private:

    friend void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c);
    friend void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c);
    friend void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t direction, uint16_t addrMatchCode);
    friend void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c);
    friend void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c);

	void init();

    enum State : uint8_t {
        Stop,
        Wait,
        WaitAddr,
        HaveAddr,
        SetReg,
        GetReg,
        SendStat,
    } i2cStatus = Stop;

    enum Error : uint8_t {
        fineAndDandy,
        statusError,
        transmitError,
        receiveError,
        unknownError
    } i2cError = fineAndDandy;

    uint8_t i2cReg = 0;
    uint8_t i2cRegBank[256];

    void slaveTxCallback(I2C_HandleTypeDef *hi2c);
    void slaveRxCallback(I2C_HandleTypeDef *hi2c);
    void addrCallback(I2C_HandleTypeDef *hi2c, uint8_t direction, uint16_t addrMatchCode);
    void listenCallback(I2C_HandleTypeDef *hi2c); 
    void errorCallback(I2C_HandleTypeDef *hi2c);
};

#endif  // #ifndef _I2C_H_
