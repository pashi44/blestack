# Nano 33 BLE Sense IMU → nRF9151 DK

Two independently linked Zephyr applications under the existing `blestack` tree.
The original application is unchanged (its current I2C implementation is a stub).
Build and flash **one image per board**; they communicate through I2C, not a
linker operation or BLE. Your confirmed hardware is **Nano Sense Rev2**.

## Refactored implementation (Zephyr APIs, firmware banner v2)

The observed DK output was `-5` / `ERRORSRC=0x2`: **address NACK**. This means
no target acknowledged 0x42; it is not a CRC failure or an IMU measurement error.
A successful compile alone cannot establish whether the Nano was flashed/running
or whether wiring and voltage levels are correct.

- Nano external I2C uses `i2c_target_register()` with Nordic's **buffer-mode**
  target callbacks. The `nordic,nrf-twis` driver owns TWIS0, interrupts, clock
  stretching and EasyDMA. The overlay explicitly selects **TWIS** pin functions.
- Internal sensor access stays on TWIM1, P0.15 SCL/P0.14 SDA, using Zephyr's
  BMI270 sensor API for the confirmed Rev2 board. Sensor power and internal
  pull-ups are initialized by the board support. No raw MMIO remains in either app.
- Onboard RGB uses `pwm_set_pulse_dt()`, with the board's active-low PWM polarity.
  Startup lights **red, then green, then blue**, 300 ms each, independently of
  whether the sensor or master works. A separate yellow LED heartbeat toggles
  every 500 ms. SPI2 is disabled because it otherwise owns that LED's P0.13 pin.
- In normal operation X/Y/Z acceleration magnitude controls R/G/B brightness.
  It is intentionally steady when stationary, not continuous RGB flashing.
  Full brightness corresponds to 1g; tilting redistributes brightness. This
  measures acceleration/gravity direction, not absolute position or yaw.
- Blinking **red** means sensor setup/read failure. Blinking **magenta** means
  target registration failed. UART logs report exact return codes and read counts.
- Master uses `i2c_read()` on TWIM2, P0.30 SCL/P0.31 SDA at 100 kHz, every 200 ms.
  It separates bus errors, invalid packets, sensor errors and stale samples.
  Transfers have a 100 ms driver timeout and repeated errors trigger
  `i2c_recover_bus()`. Recovery cannot repair a missing target or wrong wiring.
- Data publication is protected by a spinlock; the target ISR snapshots 36 bytes
  and the Zephyr driver copies them to its own DMA buffer. No sensor access,
  sleeping, or logging occurs inside target callbacks.
- BMM150 is disabled: the packet contains accelerometer + gyro (six axes), not
  magnetometer data. External breadboard LEDs are not controlled by this example.

Source comments explain driver ownership, callback lifetime and GPIO/PWM mapping.
Use **pristine rebuilds** and flash both boards to replace the old firmware.

## Wiring (power off while connecting)

The numbers 30/31 mean **nRF9151 GPIO P0.30/P0.31**, not breadboard rows or Nano
header positions. The assignment below follows your requested SCL/SDA order;
ignore any default Arduino-header I2C labels on the DK.

| Signal | nRF9151 DK | Nano 33 BLE Sense Rev2 |
|---|---|---|
| SCL | P0.30 | A5 / SCL / nRF52840 P0.02 |
| SDA | P0.31 | A4 / SDA / nRF52840 P0.31 |
| Ground | GND | GND |

**Check VDD_GPIO before connecting the signal wires.** Nano logic is 3.3 V;
DK GPIO voltage depends on its configuration. When voltage domains differ, use a
bidirectional open-drain I2C level shifter: DK-side pull-ups to DK VDD_GPIO,
Nano-side pull-ups to Nano 3.3 V, common ground. This is the recommended wiring.
Do not pull a 1.8 V DK GPIO directly up to 3.3 V or use 5 V pull-ups.
For equal, verified-compatible GPIO domains, a direct bus can use a pair of
approximately 4.7 kΩ pull-ups to the common logic rail. Account for pull-ups
already on a level-shifter module. Internal weak pull-ups alone are insufficient
for predictable bus timing. Use short wires and power each board via its own USB;
do not tie their supply outputs together.

The Nano internal IMU bus is separate: do not wire the DK to internal sensor pins.
No sensor data travels through the Nano USB connector in this default build.

## Build

These commands were checked in the existing Zephyr checkout, version 4.4.99
(`v4.4.0-13726-gc38f3a9de419`), using Zephyr SDK 1.0.1. Use that workspace's
installed Python dependencies and `west`. Older Zephyr releases may lack Rev2
board definitions; copying just this application does not add board support.

```sh
cd /home/hadoop/zephyrworkspace/zephyr/samples/userspace/blestack/imu_link

# Your Rev2 board; app.overlay is loaded automatically, rev2.overlay is additive.
west build -p always -b arduino_nano_33_ble@rev2/nrf52840/sense nano_slave \
  -d build-nano-rev2 -- -DEXTRA_DTC_OVERLAY_FILE=rev2.overlay

# Non-secure application plus TF-M secure firmware.
west build -p always -b nrf9151dk/nrf9151/ns nrf9151_master -d build-master
```

The master explicitly enables `CONFIG_BUILD_WITH_TFM`. The checked TF-M platform
configuration grants TWIM2 and GPIO0 to non-secure firmware. Flash the generated
combined image, **not only `zephyr.hex`**, so startup and security attribution
match the application. Custom secure firmware must independently grant serial
instance 2, P0.30/P0.31 and the EasyDMA RAM region to the non-secure application.
The master does not attempt to modify secure SPU registers from non-secure code.

Optional original-board support (LSM9DS1, only if the physical board is Rev1):

```sh
west build -p always -b arduino_nano_33_ble@rev1/nrf52840/sense nano_slave \
  -d build-nano-rev1 -- -DEXTRA_DTC_OVERLAY_FILE=rev1.overlay
```

Always use separate build directories or `west build -p always` when changing
board/revision. The photograph does not show a readable revision label; use your
confirmed Rev2 selection, and check the board's printed revision if IMU startup
reports unavailable.

## Flash and view readings

1. Connect the Nano by USB and double-tap RESET to enter its existing Arduino
   bootloader (pulsing orange LED). Find its bootloader port, e.g. `/dev/ttyACM0`.
2. Use the **Arduino variant of bossac**, installed with Arduino's board package;
   the generic SDK bossac is not suitable. Replace the tool path/port below with
   the ones installed on your machine.

```sh
west flash -d build-nano-rev2 -r bossac \
  --bossac="$HOME/.arduino15/packages/arduino/tools/bossac/1.9.1-arduino2/bossac" \
  --bossac-port=/dev/ttyACM0

# DK uses its onboard debugger. Install Nordic nrfutil device tooling if needed.
west flash -d build-master -r nrfutil
# Alternative if J-Link is installed:
# west flash -d build-master -r jlink
```

`build-master/zephyr/runners.yaml` selects `tfm_merged.hex` automatically. The Nano
build reserves the first 64 KiB for its bootloader; avoid a full-chip erase.
If more than one debugger is attached, select the correct device using the
runner's device-ID option (`west flash -d build-master --context`).

3. Open the **DK application's** virtual COM port at **115200, 8N1**, no flow
   control. There can be multiple ports; the application banner identifies the
   right one (TF-M may print on another). For example:

```sh
python3 -m serial.tools.list_ports
python3 -m serial.tools.miniterm /dev/ttyACM1 115200
```

The port names above are examples, not fixed assignments. The Nano application
uses UART0 on D1/TX (P1.03) for its optional local diagnostics, at 115200; use a
3.3 V USB-UART adapter RX plus GND to see them. Its bootloader USB serial port
may disappear after boot because this application does not enable USB CDC.
The master USB serial display works without a Nano console adapter.

Illustrative master output (actual axes/signs depend on orientation):

```text
IMU master: P0.30=SCL P0.31=SDA addr=0x42 100kHz
seq=100 acc[mm/s2]=20,-15,9790 gyro[mrad/s]=0,1,-2
```

## Protocol

The master initiates a plain 36-byte READ from `0x42` with STOP, without a
register-pointer write. All multi-byte fields are little-endian.

| Bytes | Meaning |
|---|---|
| 0–1 | ASCII `IM` |
| 2 | Version 1 |
| 3 | 1 = valid sample; 2 = IMU unavailable/read failed |
| 4–7 | Monotonic sample-attempt sequence, wraps at 32 bits |
| 8–19 | X/Y/Z acceleration, signed int32, mm/s² |
| 20–31 | X/Y/Z angular velocity, signed int32, mrad/s |
| 32–33 | Reserved, zero |
| 34–35 | CRC-16/CCITT-FALSE over bytes 0–33 |

Each read gets an atomic snapshot. Sensor failure sends zero axes with an error
flag and blinks red; it never reports old readings as valid.
A sequence unchanged for more than one second is marked `STALE`. The target
consumes/discards unexpected writes. Only the specified
single-master, fixed-length read protocol is intended for normal operation.

## Diagnose the current failure

1. Flash the Nano first. After resetting, look for the red → green → blue test
   and yellow heartbeat. If neither appears, solve boot/flashing/power first;
   changing the master packet decoder cannot fix that.
2. If only red blinks, check the Nano log (`IMU setup`, `imu=`) and the actual
   board revision. The photograph does not show a readable revision label.
   Rev2 uses BMI270; original Sense uses LSM9DS1. Use the matching build.
3. If the startup test passes but the master still reports read errors, verify
   common ground, level shifting, pull-ups, and actual GPIO/header mapping.
   A bus-recovery return of 0 only means lines were released, not an ACK.
4. The Nano's `reads=` count should increase when the master polls. If it stays
   zero, requests are not reaching the target callback. If it increases while
   the master rejects packets, compare firmware versions and inspect the bus.
5. Once valid, the master displays `seq=... acc[mm/s2]=... gyro[mrad/s]=...`.

The DK's existing firmware was read through its J-Link virtual serial port and
confirmed address NACKs. The Nano is not currently enumerated on USB; this is
also normal when its default Zephyr application has no USB CDC, so it does not
by itself prove that it is unpowered. Double-tap RESET to enumerate its bootloader.

## Validation and hardware bring-up

All three configurations (Rev2, optional Rev1, nRF9151 non-secure + TF-M) build
successfully. A host test covers known CRC vectors, signed boundary values,
little-endian serialization, every single-bit packet corruption, and version
rejection:

```sh
cc -std=c11 -Wall -Wextra -Werror -Icommon tests/packet_test.c -o /tmp/imu-packet-test
/tmp/imu-packet-test
```

No physical flashing or end-to-end electrical verification was performed here.
After flashing, check that the banner appears, sequence advances, and tilting
changes the axis readings and onboard RGB intensity. Disconnecting the Nano
should produce a bounded I2C error; reconnecting/powering it should restore
readings. If no ACK, check level shifting, pull-ups, shared ground, A4/A5 and
P0.30/P0.31 mapping. If address ACKs but IMU is unavailable, inspect Nano's UART
startup log and confirm Rev2 versus Rev1. A logic analyzer should show 100 kHz,
address 0x42 read, 36 bytes, a final NACK and STOP.

## References

- [Nano Rev2 pinout](https://docs.arduino.cc/resources/pinouts/ABX00069-full-pinout.pdf)
- [Arduino sensor revision differences](https://support.arduino.cc/hc/en-us/articles/11729186296476-Use-the-new-sensor-libraries-for-Nano-33-BLE-Rev2-and-Nano-BLE-Sense-Rev2)
- [Zephyr Nano board and Arduino bossac instructions](https://docs.zephyrproject.org/latest/boards/arduino/nano_33_ble/doc/index.html)
- [Zephyr Nordic TWIS binding](https://docs.zephyrproject.org/latest/build/dts/api/bindings/i2c/nordic%2Cnrf-twis.html)
- [Zephyr PWM API](https://docs.zephyrproject.org/latest/hardware/peripherals/pwm.html)
- [nRF52840 product specification: TWIS, EasyDMA and PWM](https://docs-be.nordicsemi.com/bundle/nRF52840_PS_v1.8/raw/resource/enus/nRF52840_PS_v1.8.pdf)
- [nRF9151 GPIO voltage domain](https://docs.nordicsemi.com/r/bundle/nwp_056/page/wp/nwp_054/vdd_gpio.html)
