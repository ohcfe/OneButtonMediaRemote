#include "driver/gpio.h"
#include "freertos/projdefs.h"
#include "hal/gpio_types.h"
#include <AndrewsMediaRemote.h>

#define GPIO_INPUT_PIN_SEL (1ULL<<GPIO_INPUT_PIN)

void setupGPIO(void * args)
{
    button_t *B = (button_t *)args;
    printf("in setupGPIO: GPIO_INPUT_PIN=%d, B->pinID=%d, debounce_ms = %d\n",
            (int)GPIO_INPUT_PIN,
            (int)B->pinID,
            (int)B->debounce_ms);
    printf("args is pointing to %p\n", args);
    //B.pinID = GPIO_INPUT_PIN;
    /*
    B.debounceTimerPos = xTimerCreate(
            "debounce",
            pdMS_TO_TICKS(B.debounce_ms),
            pdFALSE, &B, debounceTimerPosCallback);
    B.debounceTimerNeg = xTimerCreate(
            "debounce",
            pdMS_TO_TICKS(B.debounce_ms),
            pdFALSE, &B, debounceTimerNegCallback);
            */
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    gpio_config_t ioConf = {};
    ioConf.intr_type = GPIO_INTR_NEGEDGE;
    ioConf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    ioConf.mode = GPIO_MODE_INPUT;
    ioConf.pull_up_en = 0;
    ioConf.pull_down_en = 1;
    gpio_config(&ioConf);
    xTaskCreate(gpioTask, "gpioTask", 2048, args, 10, NULL);
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(
            GPIO_INPUT_PIN, gpio_isr_handler, (void*) B->pinID);
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

//Handler for GPIO interrupts
static void gpioTask(void * args)
{
    button_t *B = (button_t *)args;
    printf("in gpioTask, outside of queue\n");
    printf("    B->pinID = %d\n", (int)B->pinID);
    printf("    B = %p\n", B);
    uint32_t io_num;
    TickType_t ticks;
    int debounceTicks = pdMS_TO_TICKS(B->debounce_ms);
    int lastPressedTick = 0;
    int lastReleasedTick = 0;
    int level;
    int inPin = B->pinID;
    int pressCount = 0;
    int pushTicks = 0;

    char *string_ptr = (char *)&B;
    int kk = sizeof(button_t);
    while(kk--){
        printf(" %hhx ", *string_ptr++);
    }
    printf("\n");
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            level = gpio_get_level(inPin);
            ticks = xTaskGetTickCountFromISR();
            //printf("in gpioTask, inside queue\n");
            /*
            printf("    B->pinID = %d\n", (int)B->pinID);
            printf("    B->debounceTicks = %d\n", (int)B->debounceTicks);
            printf("    B->lastPressTime = %d\n", (int)B->LastPressTime);
            printf("    B->lastReleaseTime = %d\n", (int)B->LastReleaseTime);
            printf("    B = %p\n", B);
            string_ptr = (char *)&B;
            kk = sizeof(button_t);
            while(kk--){
                printf(" %hhx ", *string_ptr++);
            }
            printf("\n");
            */
            if(level == 1){
                //gpio_set_level(LED_GPIO, 1);
                /*
                if (lastPressedTick == 0){
                    //printf("First press at %d\n", (int)ticks);
                    lastPressedTick = ticks;
                }
                else */
                if (ticks < lastPressedTick ){
                    //printf("Time went through max\n");
                    lastPressedTick = ticks;
                }
                else if (ticks -lastPressedTick  < debounceTicks || ticks - lastReleasedTick  < debounceTicks){
                    //printf("Bounce 1!\n");
                }
                else{
                    //pressCount++;
                    //printf("Button pressed at %d, pressCount=%d\n", (int)ticks, pressCount);
                    //printf("Button was released for %d ticks\n", (int)(ticks - lastReleasedTick));
                    lastPressedTick = ticks;
                }
            }
            else if(level == 0){
                //gpio_set_level(LED_GPIO, 0);
                /*
                if (lastReleasedTick == 0){
                    //printf("First released at %d\n", (int)ticks);
                    lastReleasedTick = ticks;
                }
                else*/
                if (ticks < lastReleasedTick){
                    //printf("Time went through max\n");
                    lastReleasedTick = ticks;
                }
                else if (ticks -lastPressedTick  < debounceTicks || ticks - lastReleasedTick  < debounceTicks){
                    //printf("Bounce 0!\n");
                }
                else{
                    //printf("Button released at %d\n", (int)ticks);
                    //printf("Button was pressed for %d ticks\n", (int)(ticks - lastPressedTick));
                   // printf("ticks - lastPressedTick = %d ", (int)(ticks - lastPressedTick));
                    if (ticks - lastPressedTick > 100){
                        //printf("-\n");
                        gpio_set_level(LED_GPIO, 0);
                    }else{
                        gpio_set_level(LED_GPIO, 1);
                        //printf(".\n");
                    }
                    pushTicks = ticks - lastPressedTick;
                    lastReleasedTick = ticks;
                }
            }
            else{
                printf("Shouldn't be here, level was %d\n", level);
            }
        }
    }
}
//Handler for rising edge interrupt
/*
static void gpioTaskPos(void *args)
{
    button_t *B = (button_t *)args;
    BaseType_t high_task_wakeup = pdFALSE;
    printf("Inside gpioTaskPos!\n");
    uint32_t io_num;
    for (;;) {
    if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
        printf("GPIOpos[%d] intr, val: %d\n", (int)io_num, gpio_get_level(io_num));
        vTaskDelete(B->gpioTask);
        xTaskCreate(gpioTaskNeg, "gpioTaskNeg", 2048, &B, 10, &(B->gpioTask));
        gpio_set_intr_type (B->pinID, GPIO_INTR_NEGEDGE);
        //xTimerStartFromISR(B->debounceTimerPos, &high_task_wakeup);
    }
    }
}

//handler for falling edge interrupt
static void gpioTaskNeg(void *args)
{
    button_t *B = (button_t *)args;
    BaseType_t high_task_wakeup = pdFALSE;
    printf("Inside GPIO gpioTaskNeg!\n");
    uint32_t io_num;
    for (;;) {
    if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
        printf("GPIOneg[%d] intr, val: %d\n", (int)io_num, gpio_get_level(io_num));
        vTaskDelete(B->gpioTask);
        xTaskCreate(gpioTaskPos, "gpioTaskPos", 2048, &B, 10, &(B->gpioTask));
        gpio_set_intr_type (B->pinID, GPIO_INTR_POSEDGE);
        //xTimerStartFromISR(B->debounceTimerNeg, &high_task_wakeup);
    }
    }
}
static void debounceTimerPosCallback(TimerHandle_t xTimer)
{
    printf("Debounce Timer Rising Edge Callback!\n");
}

static void debounceTimerNegCallback(TimerHandle_t xTimer)
{
    printf("Debounce Timer Falling Edge Callback!\n");
}
*/
