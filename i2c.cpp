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

extern "C" I2C_HandleTypeDef hi2c2;

extern "C" {

__attribute__((used)) void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    i2c::instance().slaveTxCallback(hi2c);
}

__attribute__((used)) void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    i2c::instance().slaveRxCallback(hi2c);
}

__attribute__((used)) void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t direction, uint16_t addrMatchCode) {
    i2c::instance().addrCallback(hi2c, direction, addrMatchCode);
}

__attribute__((used)) void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c) {
    i2c::instance().listenCallback(hi2c);
}

__attribute__((used)) void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
    i2c::instance().errorCallback(hi2c);
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

    for (size_t c = 0; c < sizeof(i2cRegBank); c++) {
        i2cRegBank[c] = c; 
    }

    HAL_I2C_EnableListen_IT(&hi2c2);
    i2cStatus = State::Wait;
}

void i2c::update() {
}

void i2c::slaveTxCallback(I2C_HandleTypeDef *hi2c) {
    switch (i2cStatus) {
        case State::GetReg: {
            if (HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &i2cRegBank[i2cReg], 1, I2C_FIRST_FRAME) != HAL_OK) {
                i2cError = Error::transmitError;     
            }
        } break;
        case State::SendStat: {
            if (HAL_I2C_Slave_Seq_Transmit_IT(hi2c, reinterpret_cast<uint8_t *>(i2cError), sizeof(i2cError), I2C_FIRST_FRAME) != HAL_OK) {
                i2cError = Error::transmitError;                
            }
        } break;
        default: {
            i2cError = Error::statusError;                
        } break;
    }
}

void i2c::slaveRxCallback(I2C_HandleTypeDef *hi2c) {
    switch (i2cStatus) {
        case State::WaitAddr: {
            i2cStatus = State::HaveAddr;
            if (HAL_I2C_Slave_Seq_Receive_IT(hi2c, &i2cRegBank[i2cReg], 1, I2C_FIRST_AND_NEXT_FRAME) != HAL_OK) {
                i2cError = Error::receiveError;                
            }
        } break;
        case State::HaveAddr: {
            i2cStatus = State::SetReg;
            if (HAL_I2C_Slave_Seq_Receive_IT(hi2c, &i2cRegBank[i2cReg], 1, I2C_FIRST_FRAME) != HAL_OK) {
                i2cError = Error::receiveError;                
            }
        } break;
        case State::SetReg: {
            if (HAL_I2C_Slave_Seq_Receive_IT(hi2c, &i2cRegBank[i2cReg], 1, I2C_FIRST_FRAME) != HAL_OK) {
                i2cError = Error::receiveError;                
            }
        } break;
        default: {
            i2cError = Error::statusError;                
        } break;
    }
}

void i2c::addrCallback(I2C_HandleTypeDef *hi2c, uint8_t direction, uint16_t) {
    switch (i2cStatus) {
        case State::Wait: {
            switch (direction) {
                case I2C_DIRECTION_TRANSMIT: {
                    i2cStatus = State::WaitAddr;
                    if (HAL_I2C_Slave_Seq_Receive_IT(hi2c, &i2cReg, sizeof(i2cReg), I2C_NEXT_FRAME) != HAL_OK) {
                        i2cError = Error::receiveError;                
                    }
                } break;
                case I2C_DIRECTION_RECEIVE: {
                    i2cStatus = State::SendStat;
                    if (HAL_I2C_Slave_Seq_Transmit_IT(hi2c, reinterpret_cast<uint8_t *>(&i2cError), sizeof(i2cError), I2C_FIRST_FRAME) != HAL_OK) {
                        i2cError = Error::transmitError;                
                    }
                } break;
                default: {
                    i2cError = Error::unknownError;                
                } break;
            }
        } break;
        case State::HaveAddr: {
            switch (direction) {
                case I2C_DIRECTION_TRANSMIT: {
                    i2cStatus = State::SetReg;
                    if (HAL_I2C_Slave_Seq_Receive_IT(hi2c, &i2cRegBank[i2cReg], 1, I2C_FIRST_FRAME) != HAL_OK) {
                        i2cError = Error::receiveError;                
                    }
                } break;
                case I2C_DIRECTION_RECEIVE: {
                    i2cStatus = State::GetReg;
                    if (HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &i2cRegBank[i2cReg], 1, I2C_FIRST_FRAME) != HAL_OK) {
                        i2cError = Error::transmitError;                
                    }
                } break;
                default: {
                    i2cError = Error::unknownError;                
                } break;
            }
        } break;
        default: {
            i2cError = Error::statusError;                
        } break;
    }
}

void i2c::listenCallback(I2C_HandleTypeDef *hi2c) {
	HAL_I2C_EnableListen_IT(hi2c);
    i2cStatus = State::Wait;
}


void i2c::errorCallback(I2C_HandleTypeDef *hi2c) {
	if (HAL_I2C_GetError(hi2c) != HAL_I2C_ERROR_AF) {
        return;
	}
    HAL_I2C_EnableListen_IT(hi2c);
    i2cStatus = State::Wait;
}