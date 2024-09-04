#include "FreeRTOSConfig.h"
#include "driver/gpio.h"
#include "freertos/projdefs.h"
#include "hal/gpio_types.h"
#include "portmacro.h"
#include <AndrewsMediaRemote.h>
//#include "portmacro.h"

#define GPIO_INPUT_PIN_SEL (1ULL<<GPIO_INPUT_PIN)


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
