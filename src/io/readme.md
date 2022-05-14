²# IO

## Button

- Pushing the button forces an active scan. 

## LED

3 LEDs:
- BLE led (blue) : pulse when receiving advertising packets, brightness indicates strength of signal (RSSI)
- IPC led :
  - RX (orange) : pulse when receiving an IPC packet, pulse length indicates received packet length
  - TX (green) : pulse when sending an IPC packets, pulse length indicates sent packet length

Use 10kHz PWM to control brightness

Use a hardware timer or RTC to stop pulses (via counter driver)