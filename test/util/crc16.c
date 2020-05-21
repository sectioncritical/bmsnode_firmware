
#include <stdint.h>

#include "util/crc16.h"

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

#ifdef TEST_MAIN

// this can be used to generate crc for a config with test values.
// whenever the unit test is updated and test config values are changed,
// you need to know what the correct crc is so it can be tested.
// To use this:
// - update the testcfg below to whatever values you need
// - build this with gcc like this:
//     gcc -DTEST_MAIN -I.. -o crc16 crc16.c
// - run the test program ./crc16 to get the new CRC value

#include <stdbool.h>
#include <stdio.h>
#include "../../src/cfg.h"

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

#endif
