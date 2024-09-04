#include "esp_err.h"
#include "driver/gpio.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "hal/gpio_types.h"
#include "portmacro.h"

#define GPIO_INPUT_PIN 23
#define LED_GPIO 2
#define ESP_INTR_FLAG_DEFAULT 0


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

static QueueHandle_t gpio_evt_queue = NULL;
static void debounceTimerCallback(TimerHandle_t xTimer);
static void pressTimerCallback(TimerHandle_t xTimer);
static void gpioTask(void *args);
void setupGPIO(void * args);
void setupBluetooth(void * args);
static void gpio_isr_handler(void* arg);

typedef struct button_t *button_handle_t;

typedef enum {
    BUTTON_EVENT_DOWN, /*!< Key is pressed down */
    BUTTON_EVENT_UP    /*!< Key is released */
} button_event_id_t;
/*
typedef esp_err_t (*ButtonEventHandler)(
        button_handle_t mkbd_handle,
        button_event_id_t event,
        void *event_data,
        void *handler_args) ;
        */
