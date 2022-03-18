# UART

Demo with [**github.com/lucasdietrich/AVRTOS : drivers/UART**](https://github.com/lucasdietrich/AVRTOS/tree/2b78c34723f1e4804400c19f88b854b5bdb1cdef)

Generated files :
- [./build/zephyr/zephyr.dts](./build/zephyr/zephyr.dts)
- [./build/zephyr/include/generated/autoconf.h](./build/zephyr/include/generated/autoconf.h)

Sources :
- [Zephyr RTOS - API Reference - UART](https://docs.zephyrproject.org/latest/reference/peripherals/uart.html#uart-async-api)
- [too1 / ncs-uart-async-count-rx/src/main.c](https://github.com/too1/ncs-uart-async-count-rx/blob/master/src/main.c)

Expected output :
```
[00:02:53.037,017] <dbg> ipc.ipc_uart_cb: evt->type = 2
[00:02:53.057,647] <dbg> ipc.ipc_uart_cb: evt->type = 4
[00:02:53.078,277] <dbg> ipc.ipc_uart_cb: evt->type = 3
[00:02:53.098,907] <dbg> ipc.IPC message received
                             68 69 6a 6b 6c 6d 6e 6f  70 71 72 73 74 75 76 77 |hijklmno pqrstuvw
                             78 79 7a 61 62 63 64 65                          |xyzabcde
[00:02:53.339,202] <dbg> ipc.ipc_uart_cb: evt->type = 2
[00:02:53.359,863] <dbg> ipc.IPC message received
                             66 67                                            |fg
```