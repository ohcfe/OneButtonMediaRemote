#include <AndrewsMediaRemote.h>

void app_main(void)
{
    button_t button;
    button.pinID = GPIO_INPUT_PIN;
    button.debounce_ms = 20;
    int level;
    printf("The pin is %d\n", GPIO_INPUT_PIN);
    printf("The address of button is: %p\n", &button);
    setupGPIO(&button);
    level = gpio_get_level(GPIO_INPUT_PIN);
    printf("The level of pin %d is %d\n", GPIO_INPUT_PIN, level);
}
