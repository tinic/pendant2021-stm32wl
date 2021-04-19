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

#include "main.h"
#include "app_lorawan.h"
#include "secure-element.h"
#include "Region.h"
#include "RegionUS915.h"
#include "LmHandler.h"

#include <stdio.h>
#include <memory.h>

const uint8_t *lora_encode_packet(size_t *len) {

	int8_t dataRate = DR_0;
	LmHandlerGetTxDatarate(&dataRate);

	GetPhyParams_t getPhy {};
    getPhy.Attribute = PHY_MAX_PAYLOAD;
    getPhy.Datarate = dataRate;
    PhyParam_t phyParam = RegionUS915GetPhyParam( &getPhy );

    size_t dataLimit = phyParam.Value;

	printf("Data Limit: %d\r\n", int(dataLimit));

	static OutBitStream bitstream;

	bitstream.Reset();

	i2c::instance().encodeForLora(bitstream, dataLimit);

	bitstream.FlushBits();

	*len = uint8_t(bitstream.Position());

	printf("Data Size: %d\r\n", int(*len));

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
  i2c::instance();
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
	i2c::instance().update();
}
