/* SPDX-License-Identifier: Apache-2.0 */
#ifndef IMU_PACKET_H
#define IMU_PACKET_H
#include <stdint.h>
#include <stddef.h>
/* Fixed wire format, independent of struct padding/host byte order. */
#define IMU_ADDR 0x42u
#define IMU_PACKET_SIZE 36u
#define IMU_VALID 1u
#define IMU_SENSOR_ERROR 2u
static inline void imu_put32(uint8_t *p, uint32_t v)
{
    for (unsigned i = 0; i < 4; ++i) { p[i] = (uint8_t)(v >> (8 * i)); }
}
static inline uint32_t imu_get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static inline int32_t imu_get_signed(const uint8_t *p)
{
    uint32_t v = imu_get32(p);
    return v <= INT32_MAX ? (int32_t)v : -1 - (int32_t)(UINT32_MAX - v);
}
/* CRC-16/CCITT-FALSE: polynomial 0x1021, initial 0xffff. */
static inline uint16_t imu_crc(const uint8_t *p, size_t n)
{
    uint16_t crc = 0xffff;
    while (n--) {
        crc ^= (uint16_t)*p++ << 8;
        for (unsigned i = 0; i < 8; ++i) {
            crc = (uint16_t)((crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0));
        }
    }
    return crc;
}
static inline void imu_seal(uint8_t *p)
{
    uint16_t crc = imu_crc(p, IMU_PACKET_SIZE - 2);
    p[34] = (uint8_t)crc; p[35] = (uint8_t)(crc >> 8);
}
static inline int imu_check(const uint8_t *p)
{
    return p[0] == 'I' && p[1] == 'M' && p[2] == 1 &&
           imu_crc(p, 34) == ((uint16_t)p[34] | (uint16_t)p[35] << 8);
}
#endif
