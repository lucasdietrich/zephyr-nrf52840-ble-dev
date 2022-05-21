# BLE Xiaomi Mijia measurements collector application

This nrf52840 application scans and retrieves measurements from Xiaomi Mijia `LYWSD03MMC` devices, which are Bluetooth Low Energy (BLE) devices.

The measurements are embedded into records which are sent over the UART1 interface.

## Getting started

You will need :
- Xiaomi Mijia `LYWSD03MMC` devices with ~~firmware version `1.0.0_0130`~~ custom firmware (https://github.com/pvvx/ATC_MiThermometer) version 3.7.
  - Firmware can be upgraded using ~~"Xiaomi Home" app~~ [Telink Flasher for Mi Thermostat](https://pvvx.github.io/ATC_MiThermometer/TelinkMiFlasher.html) (select subcategory : `OTA Flasher`).
  - Use default settings of the custom firmware.
- nrf52840 based board.
- Zephyr RTOS project + ARM toolchain.

Workspace can be created and application can be imported using `west` commands : 
- `west init -m https://github.com/lucasdietrich/zephyr-nrf52840-ble-dev --mr main myworkspace`
- `west update`
- Build project with `west build` command.
- Flash the application with `west -v flash -r nrfjprog --snr XXX` command, where XXX is the serial number of the board.

## Measurements

Following measurements are periodically collected from Xiaomi devices are :

| Symbol | Measurement     | Unit | Maximum resolution |
| ------ | --------------- | ---- | ------------------ |
| T      | Temperature     | °C   | 1,00E-02           |
| H      | Humidity        | %    | 1,00E+00           |
| Bl     | Battery Level   | %    | 1,00E+00           |
| Bv     | Battery Voltage | V    | 1,00E-02           |

## IPC Protocol (UART)

UART configuration is for board `nrf52840dk_nrf52840` :

| Option          | Value   |
| --------------- | ------- |
| Baudrate        | 1000000 |
| TX pin          | p1.02   |
| RX pin          | p1.01   |
| Parity          | NONE    |
| Stop bits       | 1       |
| Data bits       | 8       |
| RTS/CTS         | off     |
| HW flow control | off     |

And for `custom_acn52840` board :

| Option          | Value |
| --------------- | ----- |
| Baudrate        | 9600  |
| TX pin          | **TODO**  |
| RX pin          | **TODO**  |
| Parity          | NONE  |
| Stop bits       | 1     |
| Data bits       | 8     |
| RTS/CTS         | off   |
| HW flow control | off   |

The IPC protocol is based on messages of fixed length. IPC frame format is as follows :

![](./pics/ipc_frame_formats_white_bg.png)

## Zephyr application-specific configuration options

| Configuration option              | Description                                 | Unit    | Default Value |
| --------------------------------- | ------------------------------------------- | ------- | ------------- |
| CONFIG\_XIAOMI\_MAX\_DEVICES      | Size of the table containing Xiaomi Devices |         | 13            |
| CONFIG\_ACTIVE\_SCAN\_DURATION    | Active scan duration                        | Seconds | 30            |
| CONFIG\_ACTIVE\_SCAN\_PERIODICITY | Active scan frequency                       | Seconds | 3570          |
| CONFIG\_DATAFRAME\_MIN\_INTERVAL  | Minimum interval between two dataframes     | Seconds | 30            |

## Expected console output

```
[00:00:00.406,250] <inf> bt_hci_core: HW Variant: nRF52x (0x0002)
[00:00:00.406,250] <inf> bt_hci_core: Firmware: Standard Bluetooth controller (0x00) Version 2.7 Build 1
[00:00:00.407,012] <inf> bt_hci_core: Identity: D3:5F:A1:2E:37:92 (random)
[00:00:00.407,012] <inf> bt_hci_core: HCI: version 5.3 (0x0c) revision 0x0000, manufacturer 0x05f1
[00:00:00.407,012] <inf> bt_hci_core: LMP: version 5.3 (0x0c) subver 0xffff
[00:00:00.407,592] <dbg> io.button_init: Set up button at GPIO_0 pin 11
[00:00:01.303,710] <wrn> ipc: Seq gap 0 -> 101, 100 frames lost
[00:00:06.173,736] <dbg> app.df_build_dev_iterate_cb: A4:C1:38:56:5E:CE (ATC_565ECE) rssi: -65 temp: 2948 hum: 3814 bat: 3051 %: 94
[00:00:06.174,072] <dbg> app.df_build_dev_iterate_cb: A4:C1:38:EC:1C:6D (ATC_EC1C6D) rssi: -66 temp: 2952 hum: 3752 bat: 3080 %: 97
[00:00:06.174,224] <dbg> app.df_build_dev_iterate_cb: A4:C1:38:E0:18:ED (ATC_E018ED) rssi: -56 temp: 3220 hum: 3413 bat: 3038 %: 93
[00:00:06.174,407] <dbg> app.df_build_dev_iterate_cb: A4:C1:38:8D:BA:B4 (ATC_8DBAB4) rssi: -84 temp: 2725 hum: 4401 bat: 2842 %: 71
[00:00:06.174,560] <dbg> app.df_build_dev_iterate_cb: A4:C1:38:68:05:63 (ATC_680563) rssi: -59 temp: 2972 hum: 3916 bat: 2869 %: 74
[00:00:06.174,713] <dbg> app.df_build_dev_iterate_cb: A4:C1:38:0A:1E:38 (ATC_0A1E38) rssi: -63 temp: 2950 hum: 3887 bat: 2836 %: 70
[00:00:06.174,957] <dbg> app.send_data_frame: count = 6, frame_time = 6
[00:00:12.118,438] <dbg> app.df_build_dev_iterate_cb: A4:C1:38:56:5E:CE (ATC_565ECE) rssi: -61 temp: 2948 hum: 3814 bat: 3051 %: 94
[00:00:12.118,774] <dbg> app.df_build_dev_iterate_cb: A4:C1:38:EC:1C:6D (ATC_EC1C6D) rssi: -65 temp: 2952 hum: 3752 bat: 3080 %: 97
[00:00:12.118,927] <dbg> app.df_build_dev_iterate_cb: A4:C1:38:E0:18:ED (ATC_E018ED) rssi: -55 temp: 3216 hum: 3419 bat: 3037 %: 93
[00:00:12.119,079] <dbg> app.df_build_dev_iterate_cb: A4:C1:38:68:05:63 (ATC_680563) rssi: -77 temp: 2972 hum: 3916 bat: 2869 %: 74
[00:00:12.119,232] <dbg> app.df_build_dev_iterate_cb: A4:C1:38:0A:1E:38 (ATC_0A1E38) rssi: -57 temp: 2947 hum: 3884 bat: 2837 %: 70
[00:00:12.119,384] <dbg> app.df_build_dev_iterate_cb: A4:C1:38:28:17:E3 (ATC_2817E3) rssi: -66 temp: 2953 hum: 3878 bat: 3045 %: 93
[00:00:12.119,567] <dbg> app.df_build_dev_iterate_cb: A4:C1:38:D5:08:40 (ATC_D50840) rssi: -66 temp: 2592 hum: 5004 bat: 2680 %: 53
[00:00:12.119,720] <dbg> app.df_build_dev_iterate_cb: A4:C1:38:A7:30:C4 (ATC_A730C4) rssi: -65 temp: 2957 hum: 3949 bat: 2593 %: 43
[00:00:12.119,964] <dbg> app.send_data_frame: count = 8, frame_time = 12
```

## Custom board `custom_acn52840`

Check if board is found using : `west boards --board-root . | grep acn52840`

## VS Code

This project is fully supported by VS Code IDE.

You might need to change tasks environment variables to match your setup in `.vscode/tasks.json` file.
```json
    "options": {
        "env": {
            "venvPath": "../.venv",
            "westPath": "../.venv/bin/west",
            "netToolsPath": "../tools/net-tools",
            "serialNumber": "683339521"
        }
    },
```

- You might need to install `python` with required packages : 
  - `python -m virtualenv myworkspace/venv`
  - `source myworkspace/venv/bin/activate`
  - `pip install -r myworkspace/zephyr/scripts/requirements.txt`

- Monitor console `screen /dev/ttyACM0 115200`

## Sources / links :

- [lucasdietrich/AVRTOS : src/examples/zephyr-dev-usart-tool/main.c](https://github.com/lucasdietrich/AVRTOS/blob/drivers/src/examples/zephyr-dev-usart-tool/main.c)
- [./build/zephyr/zephyr.dts](./build/zephyr/zephyr.dts)
- [./build/zephyr/include/generated/autoconf.h](./build/zephyr/include/generated/autoconf.h)
- [**Xiaomi Mijia LYWSD03MMC : Récupérer les données du capteur sur un Raspberry Pi avec gatttool**](https://www.fanjoe.be/?p=3911)
- [Board Porting Guide](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html)