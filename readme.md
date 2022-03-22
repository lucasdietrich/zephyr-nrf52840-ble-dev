# Application

## Development

Generated files :
- [./build/zephyr/zephyr.dts](./build/zephyr/zephyr.dts)
- [./build/zephyr/include/generated/autoconf.h](./build/zephyr/include/generated/autoconf.h)


## UART

Demo with [**github.com/lucasdietrich/AVRTOS : drivers/UART**](https://github.com/lucasdietrich/AVRTOS/tree/2b78c34723f1e4804400c19f88b854b5bdb1cdef)

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

## BLE

- [**Xiaomi Mijia LYWSD03MMC : Récupérer les données du capteur sur un Raspberry Pi avec gatttool**](https://www.fanjoe.be/?p=3911)

### Devices

| Name | MAC |
| --- | --- |
| `nrf9160dk_nrf52840` - SN `960050029` | `FA:76:2E:BD:9C:BF` |
| `nrf52840dk_nrf52840` - SN `683339521` | `D3:5F:A1:2E:37:92` |
| `nrf52840dk_nrf52840` - SN `683624946` | Sniffer |
| Xiaomi LYWSD03MMC - `??` | `A4:C1:38:68:05:63` |
| Xiaomi LYWSD03MMC - `Lucas` | `A4:C1:38:A7:30:C4` |

### Comments

- `GATT Declarations 0x2800 Primary Service`

### Expected output

```
*** Booting Zephyr OS build zephyr-v2.7.1  ***
[00:00:00.397,644] <inf> main: Starting Observer 0
[00:00:00.399,322] <inf> bt_hci_core: HW Platform: Nordic Semiconductor (0x0002)
[00:00:00.399,353] <inf> bt_hci_core: HW Variant: nRF52x (0x0002)
[00:00:00.399,353] <inf> bt_hci_core: Firmware: Standard Bluetooth controller (0x00) Version 2.7 Build 1
[00:00:00.399,993] <inf> bt_hci_core: Identity: D3:5F:A1:2E:37:92 (random)
[00:00:00.399,993] <inf> bt_hci_core: HCI: version 5.3 (0x0c) revision 0x0000, manufacturer 0x05f1
[00:00:00.399,993] <inf> bt_hci_core: LMP: version 5.3 (0x0c) subver 0xffff
[00:00:00.399,993] <inf> main: Bluetooth initialized 0
[00:00:00.400,390] <inf> main: Scanning... 0
[00:00:00.419,494] <inf> main: [ CLOSE ] 0
[00:00:00.419,647] <inf> main: Device found: FA:76:2E:BD:9C:BF (random) (RSSI -36) ad len = 11
[00:00:00.536,895] <inf> main: Device found: A4:C1:38:0A:1E:38 (public) (RSSI -60) ad len = 21
[00:00:00.772,552] <inf> main: [ CLOSE ] 0
[00:00:00.772,705] <inf> main: Device found: 7F:DB:8C:8A:99:4A (random) (RSSI -47) ad len = 29
[00:00:02.818,481] <inf> main: [ CLOSE ] 0
[00:00:02.818,634] <inf> main: Device found: A4:C1:38:A7:30:C4 (public) (RSSI -33) ad len = 19
[00:00:04.374,938] <inf> main: [ CLOSE ] 0
[00:00:04.375,122] <inf> main: Device found: 5E:FB:2C:4D:5A:7C (random) (RSSI -47) ad len = 29
[00:00:05.400,665] <inf> main: bt_le_scan_stop = 0
[00:00:05.400,665] <inf> main: Found 5 devices
[00:00:05.401,245] <inf> main: Main wait ... 0
[00:00:06.185,729] <inf> main: Connected: A4:C1:38:A7:30:C4 (public)
[00:00:09.948,272] <inf> main: Discover complete 0
[00:00:10.008,270] <inf> main: data
                               43 09 27 56 0a                                   |C.'V.
[00:00:10.008,270] <inf> main: T : 23.71 °C [ 2371 ], H 39 %, bat 2646 mV
```
