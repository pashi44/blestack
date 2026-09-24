/* SPDX-License-Identifier: Apache-2.0 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include "imu_packet.h"

static const struct device *const target = DEVICE_DT_GET(DT_NODELABEL(i2c0));
static const struct device *const imu = DEVICE_DT_GET(DT_ALIAS(accel0));
static const struct pwm_dt_spec rgb[] = {
    PWM_DT_SPEC_GET(DT_ALIAS(red_pwm_led)),
    PWM_DT_SPEC_GET(DT_ALIAS(green_pwm_led)),
    PWM_DT_SPEC_GET(DT_ALIAS(blue_pwm_led)),
};
static const struct gpio_dt_spec heartbeat = GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios);
static uint8_t published[IMU_PACKET_SIZE];
static uint8_t snapshot[IMU_PACKET_SIZE];
static struct k_spinlock packet_lock;
static atomic_t read_requests;
static atomic_t ignored_writes;

/* Nordic's target driver uses buffer callbacks, NOT byte-at-a-time callbacks.
 * This runs in interrupt context: no sleeping, sensor access or printk here.
 * The driver copies snapshot into its own EasyDMA buffer before returning. */
static int read_requested(struct i2c_target_config *config, uint8_t **ptr, uint32_t *len)
{
    ARG_UNUSED(config);
    k_spinlock_key_t key = k_spin_lock(&packet_lock);
    memcpy(snapshot, published, sizeof(snapshot));
    k_spin_unlock(&packet_lock, key);
    *ptr = snapshot;
    *len = sizeof(snapshot);
    atomic_inc(&read_requests);
    return 0;
}

static void write_received(struct i2c_target_config *config, uint8_t *ptr, uint32_t len)
{
    /* Our protocol is a plain READ, without a register-pointer write. */
    ARG_UNUSED(config); ARG_UNUSED(ptr); ARG_UNUSED(len);
    atomic_inc(&ignored_writes);
}

static const struct i2c_target_callbacks callbacks = {
    .buf_read_requested = read_requested,
    .buf_write_received = write_received,
};
static struct i2c_target_config target_config = {
    .address = IMU_ADDR,
    .callbacks = &callbacks,
};

static void publish(uint32_t sequence, const int32_t acc[3], const int32_t gyro[3], int err)
{
    uint8_t p[IMU_PACKET_SIZE] = {'I', 'M', 1, err ? IMU_SENSOR_ERROR : IMU_VALID};
    imu_put32(p + 4, sequence);
    for (unsigned i = 0; i < 3; ++i) {
        imu_put32(p + 8 + 4*i, (uint32_t)acc[i]);
        imu_put32(p + 20 + 4*i, (uint32_t)gyro[i]);
    }
    imu_seal(p);
    k_spinlock_key_t key = k_spin_lock(&packet_lock);
    memcpy(published, p, sizeof(p));
    k_spin_unlock(&packet_lock, key);
}

static int rgb_set(unsigned channel, unsigned permille)
{
    /* DT supplies active-low polarity. pulse=0 means OFF, period means ON. */
    return pwm_set_pulse_dt(&rgb[channel],
                            (uint64_t)rgb[channel].period * permille / 1000);
}

static int rgb_show(const unsigned levels[3])
{
    int first_error = 0;
    for (unsigned i = 0; i < 3; ++i) {
        int err = rgb_set(i, levels[i]);
        if (err && !first_error) { first_error = err; }
    }
    return first_error;
}

static int rgb_self_test(void)
{
    for (unsigned i = 0; i < 3; ++i) {
        if (!pwm_is_ready_dt(&rgb[i])) { return -ENODEV; }
    }
    /* Visible proof main() is running, independent of sensor and master. */
    for (unsigned i = 0; i < 3; ++i) {
        unsigned levels[3] = {0};
        levels[i] = 500;
        int err = rgb_show(levels);
        if (err) { return err; }
        k_msleep(300);
    }
    const unsigned off[3] = {0};
    return rgb_show(off);
}

static int configure_imu(void)
{
    if (!device_is_ready(imu)) { return -ENODEV; }
    const struct sensor_value rate = {.val1 = 50};
    int err = sensor_attr_set(imu, SENSOR_CHAN_ACCEL_XYZ,
                              SENSOR_ATTR_SAMPLING_FREQUENCY, &rate);
    if (err) { return err; }
    return sensor_attr_set(imu, SENSOR_CHAN_GYRO_XYZ,
                           SENSOR_ATTR_SAMPLING_FREQUENCY, &rate);
}

int main(void)
{
    const int32_t zero[3] = {0};
    publish(0, zero, zero, -ENODEV);
    printk("Nano IMU link v2 (Zephyr APIs): sensor=%s target=0x42 A5=SCL A4=SDA\n",
           imu->name);
    bool heartbeat_ready = gpio_is_ready_dt(&heartbeat);
    if (heartbeat_ready) {
        heartbeat_ready = gpio_pin_configure_dt(&heartbeat, GPIO_OUTPUT_INACTIVE) == 0;
    }
    int target_err = device_is_ready(target) ?
                     i2c_target_register(target, &target_config) : -ENODEV;
    printk("I2C target registration: %d\n", target_err);
    int pwm_err = rgb_self_test();
    bool pwm_ready = pwm_err == 0;
    printk("RGB startup test: %d (red -> green -> blue)\n", pwm_err);
    int setup_err = configure_imu();
    printk("IMU setup: %d; -19 means device init failed/check board revision\n", setup_err);
    uint32_t sequence = 0;
    int64_t next_report = 0, next_heartbeat = 0, next_retry = 0;
    bool heartbeat_on = false;
    while (1) {
        int64_t now = k_uptime_get();
        /* Retry registration/ODR setup after transient errors. A sensor whose
         * boot initialization failed still needs corrected wiring/revision and reset. */
        if (now >= next_retry) {
            if (target_err && device_is_ready(target)) {
                target_err = i2c_target_register(target, &target_config);
            }
            if (setup_err) { setup_err = configure_imu(); }
            next_retry = now + 2000;
        }
        struct sensor_value a[3], g[3];
        int32_t acc[3] = {0}, gyro[3] = {0};
        int err = setup_err ? setup_err : sensor_sample_fetch(imu);
        if (!err) { err = sensor_channel_get(imu, SENSOR_CHAN_ACCEL_XYZ, a); }
        if (!err) { err = sensor_channel_get(imu, SENSOR_CHAN_GYRO_XYZ, g); }
        if (!err) {
            for (unsigned i = 0; i < 3; ++i) {
                acc[i] = (int32_t)(sensor_value_to_micro(&a[i]) / 1000);
                gyro[i] = (int32_t)(sensor_value_to_micro(&g[i]) / 1000);
            }
        }
        publish(++sequence, acc, gyro, err);
        unsigned levels[3] = {0};
        if (err || target_err) {
            /* Red blink=IMU failure; magenta blink=target registration failure. */
            levels[0] = (now / 250) % 2 ? 500 : 0;
            if (target_err) { levels[2] = levels[0]; }
        } else {
            for (unsigned i = 0; i < 3; ++i) {
                int64_t mag = acc[i] < 0 ? -(int64_t)acc[i] : acc[i];
                levels[i] = MIN(mag * 1000 / 9807, 1000);
            }
        }
        if (pwm_ready) { pwm_err = rgb_show(levels); }
        if (heartbeat_ready && now >= next_heartbeat) {
            heartbeat_on = !heartbeat_on;
            int ret = gpio_pin_set_dt(&heartbeat, heartbeat_on);
            if (ret) { printk("Heartbeat GPIO error: %d\n", ret); heartbeat_ready = false; }
            next_heartbeat = now + 500;
        }
        if (now >= next_report) {
            printk("seq=%u imu=%d target=%d pwm=%d reads=%ld writes=%ld acc=%d,%d,%d\n",
                   sequence, err, target_err, pwm_err, (long)atomic_get(&read_requests),
                   (long)atomic_get(&ignored_writes), acc[0], acc[1], acc[2]);
            next_report = now + 1000;
        }
        k_msleep(20);
    }
}
