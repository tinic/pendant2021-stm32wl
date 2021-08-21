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
#include "./user_app.h"
#include "./bitstream.h"
#include "./i2c.h"
#include "./murmurhash3.h"
#include "./ics43434.h"
#include "./ens210.h"
#include "./bq25895.h"
#include "./mmc5633njl.h"
#include "./lsm6dsm.h"

#include "main.h"
#include "app_lorawan.h"
#include "secure-element.h"
#include "Region.h"
#include "RegionUS915.h"
#include "LmHandler.h"

#include <stdio.h>
#include <memory.h>
#include <algorithm>

const uint8_t *lora_encode_packet(uint8_t *len, uint8_t *port) {

    static OutBitStream<128> bitstream;

    int8_t dataRate = DR_0;
    LmHandlerGetTxDatarate(&dataRate);

    bitstream.Reset();

    auto f2u8 = [](auto v, auto min, auto max) {
        static_assert(std::is_same<decltype(v), float>::value, "v must be float");
        static_assert(std::is_same<decltype(min), float>::value, "min must be float");
        static_assert(std::is_same<decltype(max), float>::value, "max must be float");
        return static_cast<uint8_t>(std::clamp( ( (v - min) / (max - min) ) * 255.0f, 0.0f, 255.0f));;
    };

    if (dataRate == DR_0) {
        *port = 1;

        bitstream.PutUint16LE(i2c2::instance().SystemTime());
        bitstream.PutUint8(i2c2::instance().EffectN());

        bitstream.PutUint8(f2u8(ENS210::instance().Temperature(),0.0f, 50.0f));
        bitstream.PutUint8(f2u8(ENS210::instance().Humidity(), 0.0f, 1.0f));
        bitstream.PutUint8(f2u8(BQ25895::instance().BatteryVoltage(), 2.7f, 4.2f));

    } else { // > DR_0
        *port = 2;

        bitstream.PutUint16LE(i2c2::instance().SystemTime());
        bitstream.PutUint8(i2c2::instance().Status());
        bitstream.PutUint8(i2c2::instance().EffectN());
        bitstream.PutUint8(i2c2::instance().Brightness());

        bitstream.PutUint8(f2u8(BQ25895::instance().BatteryVoltage(), 2.7f, 4.2f));
        bitstream.PutUint8(f2u8(BQ25895::instance().SystemVoltage(), 2.7f, 4.2f));
        bitstream.PutUint8(f2u8(BQ25895::instance().VBUSVoltage(), 0.0f, 5.5f));
        bitstream.PutUint8(f2u8(BQ25895::instance().ChargeCurrent(), 0.0f, 1000.0f));
        bitstream.PutUint8(f2u8(ENS210::instance().Temperature(), 0.0f, 50.0f));
        bitstream.PutUint8(f2u8(ENS210::instance().Humidity(), 0.0f, 1.0f));

        static uint32_t counter = 0;

        if ((++counter % 16) == 0) {
            *port = 3;

            bitstream.PutUint8(i2c2::instance().RingColor(0));
            bitstream.PutUint8(i2c2::instance().RingColor(1));
            bitstream.PutUint8(i2c2::instance().RingColor(2));
            bitstream.PutUint8(i2c2::instance().RingColor(3));

            bitstream.PutUint8(i2c2::instance().BirdColor(0));
            bitstream.PutUint8(i2c2::instance().BirdColor(1));
            bitstream.PutUint8(i2c2::instance().BirdColor(2));
            bitstream.PutUint8(i2c2::instance().BirdColor(3));

            bitstream.PutUint16LE(i2c2::instance().Switch1Count());
            bitstream.PutUint16LE(i2c2::instance().Switch2Count());
            bitstream.PutUint16LE(i2c2::instance().Switch3Count());
            bitstream.PutUint16LE(i2c2::instance().BootCount());
            bitstream.PutUint16LE(i2c2::instance().IntCount());
            bitstream.PutUint16LE(i2c2::instance().DSelCount());
        }
    }

    bitstream.FlushBits();

    *len = uint8_t(bitstream.Position());

    printf("Time: %d Data Size: %d Port: %d BatteryVoltage %f\r\n", int(i2c2::instance().SystemTime()), int(*len), int(*port), double(BQ25895::instance().BatteryVoltage()));

    return bitstream.Buffer();
}

void lora_decode_packet(const uint8_t *packet, size_t len) {

    if (len < 7) {
        return;
    }

    InBitStream bitstream(packet, len);

    bitstream.GetBits(8);
}

const uint8_t *lora_app_key() {
    static uint8_t devEui[8] = { };
    static uint8_t appKey[16] = { };
    if (memcmp(devEui, SecureElementGetDevEui(), 8) != 0) {
        memcpy(devEui, SecureElementGetDevEui(), 8);
        MurmurHash3_128(devEui,8,0xDEAD,appKey);
    }
    return appKey;
}

void system_init() {
    // Init mic
    ICS43434::instance();

    // Init i2c devices
    i2c2::instance();

    printf("\r\n**************************************************************");
    printf("\r\nDevEUI: ");
    for(size_t c = 0; c < 8; c++) {
        printf("%02x ",SecureElementGetDevEui()[c]);
    }
    printf("\r\nAppKey: ");
    for(size_t c = 0; c < 16; c++) {
        printf("%02x ",lora_app_key()[c]);
    }
    printf("\r\n");
}

void system_process() {
    ICS43434::instance().update();
    i2c1::instance().update();
}
