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

#include "main.h"
#include "app_lorawan.h"
#include "secure-element.h"

#include <stdio.h>

const uint8_t *lora_encode_packet(uint8_t *len) {
	static OutBitStream bitstream;

	bitstream.Reset();

	bitstream.PutBits(0, 8);

	bitstream.FlushBits();

	*len = uint8_t(bitstream.Position());
	return bitstream.Buffer();
}

void lora_decode_packet(const uint8_t *packet, size_t len) {

	if (len < 7) {
		return;
	}

	InBitStream bitstream(packet, len);

	bitstream.GetBits(8);
}

void lora_get_network_key(uint8_t *key, size_t len) {
	if (len == 16) {
		const uint8_t _key[16] = { 0xbc, 0x60, 0x99, 0xa3, 0x0a, 0xf2, 0x3d, 0xdb, 0xae, 0xeb, 0xa5, 0xe4, 0xb4, 0x45, 0x52, 0x59 };
		memcpy(key, _key, len);
	}
}

void lora_get_join_eui(uint8_t *eui, size_t len) {
	if (len == 8) {
		memset(eui, 0, len);
	}
}

void system_init() {
  i2c::instance();
  printf("\r\n**************************************************************");
  printf("\r\nDevEUI: ");
  for(size_t c = 0; c < 8; c++) {
	  printf("%02x ",SecureElementGetDevEui()[c]);
  }
  printf("\r\n");
}

void system_process() {
	i2c::instance().update();
}
