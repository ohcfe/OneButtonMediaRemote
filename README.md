| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- | -------- |

# _ Andrews Media Remote _

A bluetooth media remote to be used with one button
Patterns of button presses to implement audio controls.

Code combines bluetooth HID stuff from
`$IDF_PATH/examples/bluetooth/esp_hid_device`
and the button handling approach from
`$IDF_PATH/examples/peripherals/gpio/matrix_keyboard`


## Build
[instructions from here](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/)
```bash
idf.py set-target esp32
idf.py build
```

## Deploy

```bash
idf.py -p <port> flash
```
