# Devices

| Name | MAC |
| --- | --- |
| `nrf9160dk_nrf52840` - SN `960050029` | `FA:76:2E:BD:9C:BF` |
| `nrf52840dk_nrf52840` - SN `683339521` | `D3:5F:A1:2E:37:92` |
| `nrf52840dk_nrf52840` - SN `683624946` | Sniffer |
| Xiaomi LYWSD03MMC - `??` | `A4:C1:38:68:05:63` |
| Xiaomi LYWSD03MMC - `Lucas` | `A4:C1:38:A7:30:C4` |

## Comments

- `GATT Declarations 0x2800 Primary Service`

## Expected output

```
*** Booting Zephyr OS build zephyr-v2.7.1  ***
Starting Observer
Bluetooth initialized

Scanning...
[00:00:00.258,636] <inf> bt_hci_core: HW Platform: Nordic Semiconductor (0x0002)
[00:00:00.258,636] <inf> bt_hci_core: HW Variant: nRF52x (0x0002)
[00:00:00.258,666] <inf> bt_hci_core: Firmware: Standard Bluetooth controller (0x00) Version 2.7 Build 1
[00:00:00.259,277] <inf> bt_hci_core: Identity: D3:5F:A1:2E:37:92 (random)
[00:00:00.259,277] <inf> bt_hci_core: HCI: version 5.3 (0x0c) revision 0x0000, manufacturer 0x05f1
[00:00:00.259,307] <inf> bt_hci_core: LMP: version 5.3 (0x0c) subver 0xffff
[ CLOSE ]Device found: FA:76:2E:BD:9C:BF (random) (RSSI -36) ad len = 11
        02 01 06 07 03 0d 18 0f 18 0a 18
Device found: 51:6C:39:9F:25:31 (random) (RSSI -56) ad len = 29
        02 01 1a 03 03 64 fd 15 16 64 fd 67 08 6e 4f fe f9 91 14 98 cf 63 8f 17 07 05 6c 02 e7
Device found: A4:C1:38:8D:BA:B4 (public) (RSSI -83) ad len = 21
        02 01 06 11 16 95 fe 30 58 5b 05 01 b4 ba 8d 38 c1 a4 28 01 00
Device found: A4:C1:38:68:05:63 (public) (RSSI -62) ad len = 21
        02 01 06 11 16 95 fe 30 58 5b 05 01 63 05 68 38 c1 a4 28 01 00
[ CLOSE ]Device found: A4:C1:38:A7:30:C4 (public) (RSSI -35) ad len = 19
        02 01 06 0f 16 95 fe 30 58 5b 05 fb c4 30 a7 38 c1 a4 08
bt_le_scan_stop = 0
Found 5 devices
[ATTRIBUTE] handle 0x1 : uuid 2800 perm = 0
[ATTRIBUTE] handle 0x2 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x3 : uuid 2a00 perm = 0
[ATTRIBUTE] handle 0x4 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x5 : uuid 2a01 perm = 0
[ATTRIBUTE] handle 0x6 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x7 : uuid 2a04 perm = 0
[ATTRIBUTE] handle 0x8 : uuid 2800 perm = 0
[ATTRIBUTE] handle 0x9 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0xa : uuid 2a05 perm = 0
[ATTRIBUTE] handle 0xb : uuid 2902 perm = 0
[ATTRIBUTE] handle 0xc : uuid 2800 perm = 0
[ATTRIBUTE] handle 0xd : uuid 2803 perm = 0
[ATTRIBUTE] handle 0xe : uuid 2a24 perm = 0
[ATTRIBUTE] handle 0xf : uuid 2803 perm = 0
[00:00:05.266,876] <dbg> main.process_target: bt_conn_le_create = 0
[00:00:05.931,640] <inf> main: Connected: A4:C1:38:A7:30:C4 (public)
[ATTRIBUTE] handle 0x10 : uuid 2a25 perm = 0
[ATTRIBUTE] handle 0x11 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x12 : uuid 2a26 perm = 0
[ATTRIBUTE] handle 0x13 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x14 : uuid 2a27 perm = 0
[ATTRIBUTE] handle 0x15 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x16 : uuid 2a28 perm = 0
[ATTRIBUTE] handle 0x17 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x18 : uuid 2a29 perm = 0
[ATTRIBUTE] handle 0x19 : uuid 2800 perm = 0
[ATTRIBUTE] handle 0x1a : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x1b : uuid 2a19 perm = 0
[ATTRIBUTE] handle 0x1c : uuid 2902 perm = 0
[ATTRIBUTE] handle 0x1d : uuid 2800 perm = 0
[ATTRIBUTE] handle 0x1e : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x1f : uuid 00010203-0405-0607-0809-0a0b0c0d2b12 perm = 0
[ATTRIBUTE] handle 0x20 : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x21 : uuid 2800 perm = 0
[ATTRIBUTE] handle 0x22 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x23 : uuid ebe0ccb7-7a0a-4b0c-8a1a-6ff2997da3a6 perm = 0
[ATTRIBUTE] handle 0x24 : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x25 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x26 : uuid ebe0ccb9-7a0a-4b0c-8a1a-6ff2997da3a6 perm = 0
[ATTRIBUTE] handle 0x27 : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x28 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x29 : uuid ebe0ccba-7a0a-4b0c-8a1a-6ff2997da3a6 perm = 0
[ATTRIBUTE] handle 0x2a : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x2b : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x2c : uuid ebe0ccbb-7a0a-4b0c-8a1a-6ff2997da3a6 perm = 0
[ATTRIBUTE] handle 0x2d : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x2e : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x2f : uuid ebe0ccbc-7a0a-4b0c-8a1a-6ff2997da3a6 perm = 0
[ATTRIBUTE] handle 0x30 : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x31 : uuid 2902 perm = 0
[ATTRIBUTE] handle 0x32 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x33 : uuid ebe0ccbe-7a0a-4b0c-8a1a-6ff2997da3a6 perm = 0
[ATTRIBUTE] handle 0x34 : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x35 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x36 : uuid ebe0ccc1-7a0a-4b0c-8a1a-6ff2997da3a6 perm = 0
[ATTRIBUTE] handle 0x37 : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x38 : uuid 2902 perm = 0
[ATTRIBUTE] handle 0x39 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x3a : uuid ebe0ccc4-7a0a-4b0c-8a1a-6ff2997da3a6 perm = 0
[ATTRIBUTE] handle 0x3b : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x3c : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x3d : uuid ebe0ccc8-7a0a-4b0c-8a1a-6ff2997da3a6 perm = 0
[ATTRIBUTE] handle 0x3e : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x3f : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x40 : uuid ebe0ccd1-7a0a-4b0c-8a1a-6ff2997da3a6 perm = 0
[ATTRIBUTE] handle 0x41 : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x42 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x43 : uuid ebe0ccd7-7a0a-4b0c-8a1a-6ff2997da3a6 perm = 0
[ATTRIBUTE] handle 0x44 : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x45 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x46 : uuid ebe0ccd8-7a0a-4b0c-8a1a-6ff2997da3a6 perm = 0
[ATTRIBUTE] handle 0x47 : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x48 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x49 : uuid ebe0ccd9-7a0a-4b0c-8a1a-6ff2997da3a6 perm = 0
[ATTRIBUTE] handle 0x4a : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x4b : uuid 2902 perm = 0
[ATTRIBUTE] handle 0x4c : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x4d : uuid ebe0cff1-7a0a-4b0c-8a1a-6ff2997da3a6 perm = 0
[ATTRIBUTE] handle 0x4e : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x4f : uuid 2800 perm = 0
[ATTRIBUTE] handle 0x50 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x51 : uuid 8edffff1-3d1b-9c37-4623-ad7265f14076 perm = 0
[ATTRIBUTE] handle 0x52 : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x53 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x54 : uuid 8edffff3-3d1b-9c37-4623-ad7265f14076 perm = 0
[ATTRIBUTE] handle 0x55 : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x56 : uuid 2902 perm = 0
[ATTRIBUTE] handle 0x57 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x58 : uuid 8edffff4-3d1b-9c37-4623-ad7265f14076 perm = 0
[ATTRIBUTE] handle 0x59 : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x5a : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x5b : uuid 8edfffef-3d1b-9c37-4623-ad7265f14076 perm = 0
[ATTRIBUTE] handle 0x5c : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x5d : uuid 2902 perm = 0
[ATTRIBUTE] handle 0x5e : uuid 2800 perm = 0
[ATTRIBUTE] handle 0x5f : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x60 : uuid 0004 perm = 0
[ATTRIBUTE] handle 0x61 : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x62 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x63 : uuid 0010 perm = 0
[ATTRIBUTE] handle 0x64 : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x65 : uuid 2902 perm = 0
[ATTRIBUTE] handle 0x66 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x67 : uuid 0017 perm = 0
[ATTRIBUTE] handle 0x68 : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x69 : uuid 2902 perm = 0
[ATTRIBUTE] handle 0x6a : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x6b : uuid 0018 perm = 0
[ATTRIBUTE] handle 0x6c : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x6d : uuid 2902 perm = 0
[ATTRIBUTE] handle 0x6e : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x6f : uuid 0019 perm = 0
[ATTRIBUTE] handle 0x70 : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x71 : uuid 2902 perm = 0
[ATTRIBUTE] handle 0x72 : uuid 2800 perm = 0
[ATTRIBUTE] handle 0x73 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x74 : uuid 00000101-0065-6c62-2e74-6f696d2e696d perm = 0
[ATTRIBUTE] handle 0x75 : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x76 : uuid 2902 perm = 0
[ATTRIBUTE] handle 0x77 : uuid 2803 perm = 0
[ATTRIBUTE] handle 0x78 : uuid 00000102-0065-6c62-2e74-6f696d2e696d perm = 0
[ATTRIBUTE] handle 0x79 : uuid 2901 perm = 0
[ATTRIBUTE] handle 0x7a : uuid 2902 perm = 0
Discover complete
```