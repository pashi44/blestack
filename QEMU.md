# Board-selected implementations

Your existing `Gpios` and `Registers::I2c` declarations are shared. `src/CMakeLists.txt` selects one implementation using the board's Kconfig symbol. No runtime board detection is needed.

- nRF9151 DK: `src/gpio/` contains your original memory-mapped implementation. Only its include dependency changed. Security mode follows the board target; Nordic interrupt ownership is in `boards/*.conf`.
- QEMU Cortex-M3: `src/qemu/` prints simulated LED changes and schedules a simulated button press after 2 seconds, then every 5 seconds. LEDs turn off after 1 second. It never includes the Nordic register map.
- Other boards: configuration stops with an explanatory error instead of silently choosing incompatible hardware.

From the application directory, with your Zephyr environment activated:

```sh
west build -p always -b nrf9151dk/nrf9151/ns -d build-nrf9151 .
west build -p always -b qemu_cortex_m3 -d build-qemu .
west build -d build-qemu -t run
```

Use separate build directories when switching boards. The first commands use a pristine configuration so new board configuration files are discovered; omit `-p always` for subsequent incremental builds. QEMU virtual time can advance faster than wall-clock time, so the printed sequence may repeat quickly. Exit QEMU with Ctrl+A, then X. No flashing is needed for QEMU. Board-named overlay wrappers select your existing Nordic overlay only for the nRF9151; no Nordic overlay is supplied to QEMU.

This simulates the application behavior, not Nordic register semantics or electrical signals. The button callback runs on a Zephyr workqueue, not in an interrupt. Both I2C implementations still return the learning placeholder `0xff`; neither tests the bus, sensor readings, or target/controller communication. Use the real board to validate GPIO/GPIOTE addresses, interrupt behavior and wiring.

For learning, follow a call from `main.cpp` to its header declaration, then inspect the board-selected definition. You can modify the QEMU timing or printed state to experiment without changing the real-board implementation.

References:
- https://docs.zephyrproject.org/latest/develop/west/build-flash-debug.html
- https://docs.zephyrproject.org/latest/boards/qemu/cortex_m3/doc/index.html
