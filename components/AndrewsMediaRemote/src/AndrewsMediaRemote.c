#include "FreeRTOSConfig.h"
#include "driver/gpio.h"
#include "freertos/projdefs.h"
#include "hal/gpio_types.h"
#include "portmacro.h"
#include <AndrewsMediaRemote.h>
//#include "portmacro.h"

#define GPIO_INPUT_PIN_SEL (1ULL<<GPIO_INPUT_PIN)


// Bluetooth stuff:
void esp_hidd_send_consumer_value(uint8_t key_cmd, bool key_pressed)
{
    uint8_t buffer[HID_CC_IN_RPT_LEN] = {0, 0};
    if (key_pressed) {
        switch (key_cmd) {
        case HID_CONSUMER_CHANNEL_UP:
            HID_CC_RPT_SET_CHANNEL(buffer, HID_CC_RPT_CHANNEL_UP);
            break;

        case HID_CONSUMER_CHANNEL_DOWN:
            HID_CC_RPT_SET_CHANNEL(buffer, HID_CC_RPT_CHANNEL_DOWN);
            break;

        case HID_CONSUMER_VOLUME_UP:
            HID_CC_RPT_SET_VOLUME_UP(buffer);
            break;

        case HID_CONSUMER_VOLUME_DOWN:
            HID_CC_RPT_SET_VOLUME_DOWN(buffer);
            break;

        case HID_CONSUMER_MUTE:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_MUTE);
            break;

        case HID_CONSUMER_POWER:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_POWER);
            break;

        case HID_CONSUMER_RECALL_LAST:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_LAST);
            break;

        case HID_CONSUMER_ASSIGN_SEL:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_ASSIGN_SEL);
            break;

        case HID_CONSUMER_PLAY:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_PLAY);
            break;

        case HID_CONSUMER_PAUSE:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_PAUSE);
            break;

        case HID_CONSUMER_RECORD:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_RECORD);
            break;

        case HID_CONSUMER_FAST_FORWARD:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_FAST_FWD);
            break;

        case HID_CONSUMER_REWIND:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_REWIND);
            break;

        case HID_CONSUMER_SCAN_NEXT_TRK:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_SCAN_NEXT_TRK);
            break;

        case HID_CONSUMER_SCAN_PREV_TRK:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_SCAN_PREV_TRK);
            break;

        case HID_CONSUMER_STOP:
            HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_STOP);
            break;

        default:
            break;
        }
    }
    esp_hidd_dev_input_set(
            s_ble_hid_param.hid_dev, 0,
            HID_RPT_ID_CC_IN, buffer, HID_CC_IN_RPT_LEN);
    return;
}

void ble_hid_demo_task(void *pvParameters)
{
    while (1) {
    }
}

void ble_hid_task_start_up(void)
{
    if (s_ble_hid_param.task_hdl) {
        // Task already exists
        return;
    }
#if !CONFIG_BT_NIMBLE_ENABLED
    /* Executed for bluedroid */
    xTaskCreate(ble_hid_demo_task, "ble_hid_demo_task", 2 * 1024, NULL, configMAX_PRIORITIES - 3,
                &s_ble_hid_param.task_hdl);
#elif CONFIG_EXAMPLE_HID_DEVICE_ROLE == 1
    xTaskCreate(ble_hid_demo_task, "ble_hid_demo_task", 3 * 1024, NULL, configMAX_PRIORITIES - 3,
                &s_ble_hid_param.task_hdl);

#elif CONFIG_EXAMPLE_HID_DEVICE_ROLE == 2
    /* Nimble Specific */
    xTaskCreate(ble_hid_demo_task_kbd, "ble_hid_demo_task_kbd", 3 * 1024, NULL, configMAX_PRIORITIES - 3,
                &s_ble_hid_param.task_hdl);
#elif CONFIG_EXAMPLE_HID_DEVICE_ROLE == 3
    /* Nimble Specific */
    xTaskCreate(ble_hid_demo_task_mouse, "ble_hid_demo_task_mouse", 3 * 1024, NULL, configMAX_PRIORITIES - 3,
                &s_ble_hid_param.task_hdl);
#endif
}

void ble_hid_task_shut_down(void)
{
    if (s_ble_hid_param.task_hdl) {
        vTaskDelete(s_ble_hid_param.task_hdl);
        s_ble_hid_param.task_hdl = NULL;
    }
}

static void ble_hidd_event_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    esp_hidd_event_t event = (esp_hidd_event_t)id;
    esp_hidd_event_data_t *param = (esp_hidd_event_data_t *)event_data;
    static const char *TAG = "HID_DEV_BLE";

    switch (event) {
    case ESP_HIDD_START_EVENT: {
        ESP_LOGI(TAG, "START");
        esp_hid_ble_gap_adv_start();
        break;
    }
    case ESP_HIDD_CONNECT_EVENT: {
        ESP_LOGI(TAG, "CONNECT");
        break;
    }
    case ESP_HIDD_PROTOCOL_MODE_EVENT: {
        ESP_LOGI(TAG, "PROTOCOL MODE[%u]: %s", param->protocol_mode.map_index, param->protocol_mode.protocol_mode ? "REPORT" : "BOOT");
        break;
    }
    case ESP_HIDD_CONTROL_EVENT: {
        ESP_LOGI(TAG, "CONTROL[%u]: %sSUSPEND", param->control.map_index, param->control.control ? "EXIT_" : "");
        if (param->control.control)
        {
            // exit suspend
            ble_hid_task_start_up();
        } else {
            // suspend
            ble_hid_task_shut_down();
        }
    break;
    }
    case ESP_HIDD_OUTPUT_EVENT: {
        ESP_LOGI(TAG, "OUTPUT[%u]: %8s ID: %2u, Len: %d, Data:", param->output.map_index, esp_hid_usage_str(param->output.usage), param->output.report_id, param->output.length);
        ESP_LOG_BUFFER_HEX(TAG, param->output.data, param->output.length);
        break;
    }
    case ESP_HIDD_FEATURE_EVENT: {
        ESP_LOGI(TAG, "FEATURE[%u]: %8s ID: %2u, Len: %d, Data:", param->feature.map_index, esp_hid_usage_str(param->feature.usage), param->feature.report_id, param->feature.length);
        ESP_LOG_BUFFER_HEX(TAG, param->feature.data, param->feature.length);
        break;
    }
    case ESP_HIDD_DISCONNECT_EVENT: {
        ESP_LOGI(TAG, "DISCONNECT: %s", esp_hid_disconnect_reason_str(esp_hidd_dev_transport_get(param->disconnect.dev), param->disconnect.reason));
        ble_hid_task_shut_down();
        esp_hid_ble_gap_adv_start();
        break;
    }
    case ESP_HIDD_STOP_EVENT: {
        ESP_LOGI(TAG, "STOP");
        break;
    }
    default:
        break;
    }
    return;
}

#if CONFIG_BT_NIMBLE_ENABLED
void ble_hid_device_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();

    nimble_port_freertos_deinit();
}
void ble_store_config_init(void);
#endif

// **************END PASTE****************************
// GPIO stuff:


void setupGPIO(void * args){
    button_t *B = (button_t *)args;
    B->lastTicks = xTaskGetTickCountFromISR();
    //B->longPressTicks = pdMS_TO_TICKS(B->longPressTicks);
    B->longPressTicks = pdMS_TO_TICKS(B->long_press_ms);
    /*
    printf("in setupGPIO: GPIO_INPUT_PIN=%d, B->pinID=%d, debounce_ms = %d, longPressMS = %d, longPressTicks = %d\n",
            (int)GPIO_INPUT_PIN,
            (int)B->pinID,
            (int)B->debounce_ms,
            (int)B->long_press_ms,
            (int)B->longPressTicks);
    printf("args is pointing to %p\n", args);
    */
    //B.pinID = GPIO_INPUT_PIN;
    B->DebounceTimer = xTimerCreate(
            "debounce",
            pdMS_TO_TICKS(B->debounce_ms),
            pdFALSE, B, debounceTimerCallback);
    //vTimerSetTimerID(B->DebounceTimer, args);
    /*
    B->PressTimer = xTimerCreate(
            "longPress",
            pdMS_TO_TICKS(B->long_press_ms),
            pdFALSE, &B, pressTimerCallback);
            */
    /*
    B.debounceTimerNeg = xTimerCreate(
            "debounce",
            pdMS_TO_TICKS(B.debounce_ms),
            pdFALSE, &B, debounceTimerNegCallback);
            */
    gpio_evt_queue = xQueueCreate(8, sizeof(uint32_t));
    if (gpio_evt_queue == NULL){
        printf("ERROR: xQueueCreate failed!");
    }
    //gpio_evt_queue = xQueueCreate(8, sizeof(struct button_t *));
    //gpio_evt_queue = xQueueCreate(8, sizeof(struct button_t));
    gpio_config_t ioConf = {};
    ioConf.intr_type = GPIO_INTR_ANYEDGE;
    ioConf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    ioConf.mode = GPIO_MODE_INPUT;
    ioConf.pull_up_en = 0;
    ioConf.pull_down_en = 1;
    gpio_config(&ioConf);
    if (xTaskCreate(
            gpioTask,
            "gpioTask",
            2048,
            B,
            10, //configMAX_PRIORITIES - 5,
            NULL) != pdPASS) {
        printf("Failed to create task!");}
    if(gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT) != ESP_OK){
        printf("ERROR: gpio_install_isr_service failed!\n");
    }
    if(gpio_isr_handler_add(
            GPIO_INPUT_PIN, gpio_isr_handler, (void*) B->pinID) != ESP_OK){
        printf("ERROR: hpio_isr_handler_add failed!\n");
    }
        
    // LED setup
    gpio_reset_pin(LED_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
}

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

//timer callback
/*
static void pressTimerCallback(TimerHandle_t xTimer){
}
*/
static void debounceTimerCallback(TimerHandle_t xTimer){
    //printf("In debounceTimerCallback!\n");
    // turn the interrupt back on
    button_t *B = (button_t *)pvTimerGetTimerID( xTimer );
    /*
    printf("in debounce timer\n");
    printf("B->pinID = %d\n",(int) (B->pinID));
    printf("B->debounce_ms = %d\n", (int)(B->debounce_ms));
    printf("B->long_press_ms = %d\n", (int)(B->long_press_ms));
    printf("B->longPressTicks = %d\n", (int)(B->longPressTicks));
    */
    int level = gpio_get_level(GPIO_INPUT_PIN);
    TickType_t nowTicks;
    nowTicks = xTaskGetTickCountFromISR();
    uint8_t buffer[HID_CC_IN_RPT_LEN] = {0, 0};
    if(level == 1){
        //pressStarted = xTimerStart(B->PressTimer, 5);
        printf("level high, was released for %d ticks\n", (int)(nowTicks - B->lastTicks));
    }else if (level == 0) {
        printf("level low, was pressed for %d ticks", (int)(nowTicks - B->lastTicks));
        if (B->longPressTicks <= nowTicks - B->lastTicks) {
            printf(" LONG PRESS\n");
            gpio_set_level(LED_GPIO, 1);
        }
        else{
            printf(" SHORT PRESS\n");
            gpio_set_level(LED_GPIO, 0);
            esp_hidd_send_consumer_value(HID_CONSUMER_SCAN_NEXT_TRK, true);
            /*
            esp_hidd_dev_input_set(
                    s_ble_hid_param.hid_dev,
                    0, HID_RPT_ID_CC_IN,
                    buffer, HID_CC_IN_RPT_LEN);
                    */
        }
    }else{
        printf("Something is strange!\n");
    }
    B->lastTicks = nowTicks;
    gpio_isr_handler_add(
            GPIO_INPUT_PIN, gpio_isr_handler, (void*) GPIO_INPUT_PIN);
}

//Handler for GPIO interrupts
static void gpioTask(void * args)
{
    button_t *B = (button_t *)args;
    uint32_t io_num;
    int inPin = B->pinID;
    TimerHandle_t dbt = B->DebounceTimer;
    BaseType_t debounceStarted;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            gpio_isr_handler_remove(inPin);
            debounceStarted = xTimerStart(dbt, 5);
        }
    }
}
