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
#ifndef _USER_APP_H_
#define _USER_APP_H_

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

const uint8_t *lora_encode_packet(uint8_t *len);
void lora_decode_packet(const uint8_t *packet, size_t len);

void lora_get_app_key(uint8_t *key, size_t len);

void system_init();
void system_process();

#ifdef __cplusplus
};
#endif  // #ifdef __cplusplus

#endif  // #ifndef _USER_APP_H_
