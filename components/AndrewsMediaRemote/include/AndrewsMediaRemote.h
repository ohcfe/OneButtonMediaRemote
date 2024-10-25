
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_err.h"
#include "driver/gpio.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "hal/gpio_types.h"
#include "portmacro.h"

#if CONFIG_BT_NIMBLE_ENABLED
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#else
#include "esp_bt_defs.h"
#if CONFIG_BT_BLE_ENABLED
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#endif
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#endif

#include "esp_hidd.h"
#include "esp_hid_gap.h"
#define GPIO_INPUT_PIN 23
#define LED_GPIO 2
#define ESP_INTR_FLAG_DEFAULT 0
// STRUCT DEFINITIONS

typedef struct
{
    TaskHandle_t task_hdl;
    esp_hidd_dev_t *hid_dev;
    uint8_t protocol_mode;
    uint8_t *buffer;
} local_param_t;

struct button_t{
    uint32_t pinID;
    int debounce_ms;
    int long_press_ms;
    TickType_t longPressTicks;
    TickType_t lastTicks;
    TimerHandle_t DebounceTimer;
//    TimerHandle_t PressTimer;
    TaskHandle_t gpioTask;
};
typedef struct button_t button_t;
typedef struct button_t *button_handle_t;
// GLOBAL VARIABLES
extern local_param_t s_ble_hid_param;
extern const unsigned char mediaReportMap[];
//extern esp_hid_raw_report_map_t ble_report_maps[];
extern esp_hid_device_config_t ble_hid_config;
extern button_t button;
extern QueueHandle_t gpio_evt_queue;
extern const char *TAG;

// DEFINED CONSTANTS
// **************PASTED FROM esp_hid_device_main.c****************************
#define HID_CC_RPT_MUTE                 1
#define HID_CC_RPT_POWER                2
#define HID_CC_RPT_LAST                 3
#define HID_CC_RPT_ASSIGN_SEL           4
#define HID_CC_RPT_PLAY                 5
#define HID_CC_RPT_PAUSE                6
#define HID_CC_RPT_RECORD               7
#define HID_CC_RPT_FAST_FWD             8
#define HID_CC_RPT_REWIND               9
#define HID_CC_RPT_SCAN_NEXT_TRK        10
#define HID_CC_RPT_SCAN_PREV_TRK        11
#define HID_CC_RPT_STOP                 12

#define HID_CC_RPT_CHANNEL_UP           0x10
#define HID_CC_RPT_CHANNEL_DOWN         0x30
#define HID_CC_RPT_VOLUME_UP            0x40
#define HID_CC_RPT_VOLUME_DOWN          0x80

// HID Consumer Control report bitmasks
#define HID_CC_RPT_NUMERIC_BITS         0xF0
#define HID_CC_RPT_CHANNEL_BITS         0xCF
#define HID_CC_RPT_VOLUME_BITS          0x3F
#define HID_CC_RPT_BUTTON_BITS          0xF0
#define HID_CC_RPT_SELECTION_BITS       0xCF

// Macros for the HID Consumer Control 2-byte report
#define HID_CC_RPT_SET_NUMERIC(s, x)    (s)[0] &= HID_CC_RPT_NUMERIC_BITS;\
                                                  (s)[0] = (x)
#define HID_CC_RPT_SET_CHANNEL(s, x)    (s)[0] &= HID_CC_RPT_CHANNEL_BITS;\
                                                  (s)[0] |= ((x) & 0x03) << 4
#define HID_CC_RPT_SET_VOLUME_UP(s)     (s)[0] &= HID_CC_RPT_VOLUME_BITS;\
                                                  (s)[0] |= 0x40
#define HID_CC_RPT_SET_VOLUME_DOWN(s)   (s)[0] &= HID_CC_RPT_VOLUME_BITS;\
                                                  (s)[0] |= 0x80
#define HID_CC_RPT_SET_BUTTON(s, x)     (s)[1] &= HID_CC_RPT_BUTTON_BITS;\
                                                  (s)[1] |= (x)
#define HID_CC_RPT_SET_SELECTION(s, x)  (s)[1] &= HID_CC_RPT_SELECTION_BITS;\
                                                  (s)[1] |= ((x) & 0x03) << 4

// HID Consumer Usage IDs (subset of the codes available in the USB HID Usage Tables spec)
#define HID_CONSUMER_POWER          48  // Power
#define HID_CONSUMER_RESET          49  // Reset
#define HID_CONSUMER_SLEEP          50  // Sleep

#define HID_CONSUMER_MENU           64  // Menu
#define HID_CONSUMER_SELECTION      128 // Selection
#define HID_CONSUMER_ASSIGN_SEL     129 // Assign Selection
#define HID_CONSUMER_MODE_STEP      130 // Mode Step
#define HID_CONSUMER_RECALL_LAST    131 // Recall Last
#define HID_CONSUMER_QUIT           148 // Quit
#define HID_CONSUMER_HELP           149 // Help
#define HID_CONSUMER_CHANNEL_UP     156 // Channel Increment
#define HID_CONSUMER_CHANNEL_DOWN   157 // Channel Decrement

#define HID_CONSUMER_PLAY           176 // Play
#define HID_CONSUMER_PAUSE          177 // Pause
#define HID_CONSUMER_RECORD         178 // Record
#define HID_CONSUMER_FAST_FORWARD   179 // Fast Forward
#define HID_CONSUMER_REWIND         180 // Rewind
#define HID_CONSUMER_SCAN_NEXT_TRK  181 // Scan Next Track
#define HID_CONSUMER_SCAN_PREV_TRK  182 // Scan Previous Track
#define HID_CONSUMER_STOP           183 // Stop
#define HID_CONSUMER_EJECT          184 // Eject
#define HID_CONSUMER_RANDOM_PLAY    185 // Random Play
#define HID_CONSUMER_SELECT_DISC    186 // Select Disk
#define HID_CONSUMER_ENTER_DISC     187 // Enter Disc
#define HID_CONSUMER_REPEAT         188 // Repeat
#define HID_CONSUMER_STOP_EJECT     204 // Stop/Eject
#define HID_CONSUMER_PLAY_PAUSE     205 // Play/Pause
#define HID_CONSUMER_PLAY_SKIP      206 // Play/Skip

#define HID_CONSUMER_VOLUME         224 // Volume
#define HID_CONSUMER_BALANCE        225 // Balance
#define HID_CONSUMER_MUTE           226 // Mute
#define HID_CONSUMER_BASS           227 // Bass
#define HID_CONSUMER_VOLUME_UP      233 // Volume Increment
#define HID_CONSUMER_VOLUME_DOWN    234 // Volume Decrement

#define HID_RPT_ID_CC_IN        3   // Consumer Control input report ID
#define HID_CC_IN_RPT_LEN       2   // Consumer Control input report Len
// **************END PASTE****************************

// Function prototypes
static void debounceTimerCallback(TimerHandle_t xTimer);
static void pressTimerCallback(TimerHandle_t xTimer);
static void gpioTask(void *args);
void ble_hid_demo_task(void *pvParameters);
void setupGPIO(void * args);
static void gpio_isr_handler(void* arg);
void esp_hidd_send_consumer_value(uint8_t key_cmd, bool key_pressed);
static void ble_hidd_event_callback(void *handler_args,
        esp_event_base_t base, int32_t id, void *event_data);
void ble_hid_demo_task(void *pvParameters);
void ble_hid_task_start_up(void);
void ble_hid_task_shut_down(void);
void ble_hid_device_host_task(void *param);
void ble_store_config_init(void);
