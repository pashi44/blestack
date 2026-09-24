/* SPDX-License-Identifier: Apache-2.0 */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "imu_packet.h"
int main(void)
{
    assert(imu_crc((const uint8_t *)"123456789", 9) == 0x29b1);
    uint8_t p[IMU_PACKET_SIZE] = {'I', 'M', 1, IMU_VALID};
    const int32_t values[] = {INT32_MIN, -9807, -1, 0, 9807, INT32_MAX};
    imu_put32(p + 4, 0x12345678);
    assert(p[4] == 0x78 && p[5] == 0x56 && p[6] == 0x34 && p[7] == 0x12);
    for (unsigned i = 0; i < 6; ++i) {
        imu_put32(p + 8 + 4*i, (uint32_t)values[i]);
        assert(imu_get_signed(p + 8 + 4*i) == values[i]);
    }
    imu_seal(p);
    assert(imu_check(p));
    /* Every single-bit corruption, including in the CRC, is rejected. */
    for (unsigned bit = 0; bit < sizeof(p)*8; ++bit) {
        p[bit/8] ^= 1u << (bit%8);
        assert(!imu_check(p));
        p[bit/8] ^= 1u << (bit%8);
    }
    p[2] = 2;
    imu_seal(p);
    assert(!imu_check(p)); /* Unknown version, even with valid CRC. */
    puts("packet tests passed");
}
