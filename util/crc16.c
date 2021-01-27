
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "../src/cfg.h"

//#include "util/crc16.h"

// provide the crc8 function that is provided by the avr-libc library so
// that the module-under-test can resolve the link. Also this is used by
// test cases here to generate the 8-bit crc for test packets.
uint8_t _crc8_ccitt_update(uint8_t inCrc, uint8_t inData)
{
    uint8_t i;
    uint8_t data;

    data = inCrc ^ inData;

    for (i = 0; i < 8; i++)
    {
        if ((data & 0x80) != 0)
        {
            data <<= 1;
            data ^= 0x07;
        }
        else
        {
            data <<= 1;
        }
    }
    return data;
}

static config_t testcfg =
{
    26, 2, 99,
    1234, 5678, 4321, 7865, 5555, -9000,
    32767, 32768, 65535, 120, -100, 10000
};

int main(void)
{
    uint8_t crc = 0;
    uint8_t *pkt = (uint8_t *)&testcfg;
    for (int i = 0; i < sizeof(config_t) - 1; ++i)
    {
        crc = _crc8_ccitt_update(crc, pkt[i]);
    }
    printf("0x%02X\n", crc);
    return 0;
}
