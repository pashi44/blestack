/* SPDX-License-Identifier: Apache-2.0 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include "imu_packet.h"

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
#error "Build for nrf9151dk/nrf9151/ns"
#endif
static const struct device *const bus = DEVICE_DT_GET(DT_NODELABEL(i2c2));

int main(void)
{
    uint8_t rx[IMU_PACKET_SIZE];
    uint32_t last = 0, failures = 0;
    bool have_last = false;
    int64_t last_change = 0;
    printk("IMU master v2 (Zephyr APIs): P0.30=SCL P0.31=SDA 0x42 100kHz\n");
    if (!device_is_ready(bus)) {
        printk("FATAL: I2C2 driver not ready; check DT/TF-M startup logs\n");
        return 0;
    }
    int err = i2c_configure(bus, I2C_MODE_CONTROLLER | I2C_SPEED_SET(I2C_SPEED_STANDARD));
    if (err) { printk("FATAL: I2C configure failed: %d\n", err); return 0; }
    while (1) {
        /* One 36-byte READ + STOP; the target does not use register offsets.
         * Zephyr owns interrupts, EasyDMA, pinctrl and the 100 ms timeout. */
        err = i2c_read(bus, rx, sizeof(rx), IMU_ADDR);
        if (err) {
            ++failures;
            if (failures == 1 || failures % 5 == 0) {
                printk("I2C READ failed: %d count=%u; check Nano startup LEDs, "
                       "A5/SCL, A4/SDA, GND, pull-ups and logic levels\n", err, failures);
            }
            if (failures % 5 == 0) {
                /* Recovery clocks can release a interrupted transaction, but
                 * cannot repair a missing/unpowered target or voltage mismatch. */
                int recovery = i2c_recover_bus(bus);
                printk("I2C bus recovery: %d (0=lines released, not proof of target ACK)\n",
                       recovery);
            }
        } else if (!imu_check(rx)) {
            failures = 0;
            printk("Target ACKed, invalid packet: magic=%02x%02x version=%u; "
                   "check both firmware versions/CRC\n", rx[0], rx[1], rx[2]);
        } else {
            if (failures) { printk("I2C link restored\n"); }
            failures = 0;
            uint32_t seq = imu_get32(rx + 4);
            if (!have_last || seq != last) { last_change = k_uptime_get(); }
            have_last = true; last = seq;
            const char *stale = k_uptime_get() - last_change > 1000 ? " STALE" : "";
            if (rx[3] != IMU_VALID) {
                printk("I2C OK; Nano IMU unavailable flags=0x%x seq=%u%s; "
                       "check Nano sensor log and Rev1/Rev2 selection\n", rx[3], seq, stale);
            } else {
                printk("seq=%u acc[mm/s2]=%d,%d,%d gyro[mrad/s]=%d,%d,%d%s\n",
                       seq, imu_get_signed(rx+8), imu_get_signed(rx+12),
                       imu_get_signed(rx+16), imu_get_signed(rx+20),
                       imu_get_signed(rx+24), imu_get_signed(rx+28), stale);
            }
        }
        k_msleep(200);
    }
}
