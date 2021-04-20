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
#ifndef _BITSTREAM_H_
#define _BITSTREAM_H_

#include <stdint.h>
#include <memory.h>
#include <array>

class InBitStream {
public:
    InBitStream(const uint8_t *_buf, size_t _len)
    : m_buf(_buf),
        m_len(_len),
        m_pos(0),
        m_bitBuf(0),
        m_bitPos(0) {
    }

    void SeekToStart() { m_pos = 0; }
    void SeekTo(size_t pos) { m_pos = pos; }

    void SkipBytes(size_t size) {
        if (m_pos + size < m_len) {
            m_pos += size;
        }
    }

    const uint8_t *Buffer() const { return m_buf; }

    size_t Position() const { return m_pos - (uint32_t(m_bitPos) >> 3); }
    size_t Length() const { return m_len; }

    bool IsEOF() const { return m_pos >= m_len; }

    uint8_t GetUint8() {
        if (m_pos < m_len) {
            return m_buf[m_pos++];
        }
        m_pos++;
        return 0;
    }

    uint16_t GetUint16() {
        return  uint16_t((GetUint8() <<  8)|
                        GetUint8() <<  0);
    }

    uint32_t GetUint32() {
        return  uint32_t((GetUint8() << 24)|
                        (GetUint8() << 16)|
                        (GetUint8() <<  8)|
                        GetUint8() <<  0);
    }

    uint8_t PeekUint8(size_t off) const {
        if (m_pos + off < m_len) {
            return m_buf[m_pos + off];
        }
        return 0;
    }

    void InitBits() {
        m_bitPos = 0;
        m_bitBuf = 0;
    }

    void FlushBits() {
        for (; m_bitPos >= 8; m_bitPos -= 8) {
            m_pos--;
        }
        m_bitBuf = 0;
    }

    uint32_t GetBit() {
        int32_t bPos = m_bitPos;
        uint32_t bBuf = m_bitBuf;
        if (bPos == 0) {
            bBuf = GetUint32();
            bPos = 32;
        }
        uint32_t v = bBuf >> 31;
        bBuf <<= 1;
        bPos -= 1;
        m_bitPos = bPos;
        m_bitBuf = bBuf;
        return v;
    }

    uint32_t GetBits(uint32_t n) {
        if (n > 0 && n <= 32) {
            uint32_t v = 0;
            int32_t bPos = m_bitPos;
            uint32_t bBuf = m_bitBuf;
            if (bPos < int32_t(n)) {
                v   = bBuf >> (32 - bPos);
                n  -= uint32_t(bPos);
                v <<= n;
                bBuf = GetUint32();
                bPos = 32;
            }
            v |= bBuf >> (32 - n);
            if (n != 32)
                bBuf <<= n;
            else
                bBuf = 0;
            bPos -= n;
            m_bitPos = bPos;
            m_bitBuf = bBuf;
            return v;
        } else {
            return 0;
        }
    }

    int32_t GetSBits(uint32_t n) {
        if (n > 0 && n <= 32) {
            uint32_t v = 0;
            int32_t bPos = m_bitPos;
            uint32_t bBuf = m_bitBuf;
            int32_t nSav = int32_t(n);
            if (bPos < int32_t(n)) {
                v   = bBuf >> (32 - bPos);
                n  -= uint32_t(bPos);
                v <<= n;
                bBuf = GetUint32();
                bPos = 32;
            }
            v |= bBuf >> (32-n);
            v |= uint32_t(int32_t((v << (32 - nSav)) &
                        (1L << 31)) >> (32 - nSav));
            if (n != 32)
                bBuf <<= n;
            else
                bBuf = 0;
            bPos  -= n;
            m_bitPos = bPos;
            m_bitBuf = bBuf;
            return int32_t(v);
        } else {
            return 0;
        }
    }

    uint32_t GetExpGolomb() {
        uint32_t b = 0;
        uint32_t v = 0;
        for (; (v = GetBit()) == 0 && b < 32; b++) { }
        v <<= b;
        v |= GetBits(b);
        return v-1;
    }

    int32_t GetSExpGolomb() {
        uint32_t u = GetExpGolomb();
        uint32_t pos = u & 1;
        int32_t s = int32_t((u + 1) >> 1);
        if (pos) {
            return s;
        }
        return -s;
    }

private:
    const uint8_t *m_buf;
    size_t m_len;
    size_t m_pos;

    uint32_t m_bitBuf;
    int32_t m_bitPos;
};

template<size_t Size>
class OutBitStream {

public:
    OutBitStream()
    : m_len(sizeof(m_buf)),
        m_pos(0),
        m_bitBuf(0),
        m_bitPos(0) {
    }

    OutBitStream(const OutBitStream &from)
    : m_len(sizeof(m_buf)),
        m_pos(0),
        m_bitBuf(0),
        m_bitPos(0) {
        PutBytes(from.Buffer(), from.Length());
    }

    ~OutBitStream() {
    }

    void Reset() { m_pos = 0; m_len = 0; InitBits(); }

    size_t Position() const { return m_pos; }
    size_t Length() const { return m_len; }
    const uint8_t *Buffer() const { return m_buf.data(); }

    void SetPosition(size_t pos) {
        m_pos = pos;
    }

    void PutUint8(uint8_t v) {
        m_buf[m_pos++] = v;
    }
    void PutUint16(uint16_t v) {
        PutUint8(uint8_t((v>>8)&0xFF));
        PutUint8(uint8_t((v>>0)&0xFF));
    }
    void PutUint32(uint32_t v) {
        PutUint8((v>>24)&0xFF);
        PutUint8((v>>16)&0xFF);
        PutUint8((v>> 8)&0xFF);
        PutUint8((v>> 0)&0xFF);
    }

    void PutBytes(const uint8_t *data, size_t dataLen) {
        for (size_t c = 0; c < dataLen; c++) {
            PutUint8(data[c]);
        }
    }

    void InitBits() {
        m_bitPos = 8;
        m_bitBuf = 0;
    }

    void FlushBits() {
        if (m_bitPos < 8) {
            PutUint8(uint8_t(m_bitBuf));
        }
    }

    void PutBit(uint32_t v) {
        PutBits(uint32_t(v), 1);
    }

    void PutBits(uint32_t v, uint32_t n) {
        if (n <= 0)
            return;
        for (;;) {
            v &= (uint32_t)0xFFFFFFFF >> (32 - n);
            int32_t s = int32_t(n) - m_bitPos;
            if (s <= 0) {
                m_bitBuf |= v << -s;
                m_bitPos -= n;
                return;
            } else {
                m_bitBuf |= v >> s;
                n -= m_bitPos;
                PutUint8(uint8_t(m_bitBuf));
                m_bitBuf = 0;
                m_bitPos = 8;
            }
        }
    }

    void PutSBits(int32_t v, uint32_t n) {
        PutBits(uint32_t(v), n);
    }

    void PutExpGolomb(uint32_t v) {
        v++;
        uint32_t b = 0;
        for (uint32_t g = v; g >>= 1 ; b++) { }
        PutBits(0, b);
        PutBits(v, b+1);
    }

    void PutSExpGolomb(int32_t v) {
        v = 2 * v - 1;
        if (v < 0) {
            v ^= -1;
        }
        PutExpGolomb(uint32_t(v));
    }

    OutBitStream &operator=(const OutBitStream &from) {
        if (&from == this) {
            return *this;
        }
        m_bitBuf = 0;
        m_bitPos = 0;
        Reset();
        PutBytes(from.Buffer(), from.Length());
        return *this;
    }

private:

    std::array<uint8_t, Size> m_buf; // Max LoraWAN packet size

    size_t m_len;
    size_t m_pos;

    uint32_t m_bitBuf;
    int32_t m_bitPos;
};

#endif  // #ifndef _BITSTREAM_H_
