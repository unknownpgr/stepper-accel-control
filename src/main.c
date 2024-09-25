#include <stdio.h>
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"

#define abs(x) ((x) < 0 ? -(x) : (x))

volatile uint32_t position = 0; // Steps
volatile float velocity = 0;    // Revolutions per second

void core0_main()
{
    printf("Hello, Raspberry Pi Pico USB Serial!\n");

    while (true)
    {
        char input[20];
        if (scanf("%19s", input) == 1)
        {
            float new_velocity;
            if (sscanf(input, "%f", &new_velocity) == 1)
            {
                velocity = new_velocity;
                printf("Velocity set to: %.2f\n", velocity);
            }
            else
            {
                printf("Invalid input. Please enter a number.\n");
            }
        }
    }
}

void core1_main()
{
    // Initialize GPIO pins for stepper motor
    const uint PIN_A = 10;
    const uint PIN_B = 11;
    const uint PIN_C = 12;
    const uint PIN_D = 13;

    gpio_init(PIN_A);
    gpio_init(PIN_B);
    gpio_init(PIN_C);
    gpio_init(PIN_D);

    gpio_set_dir(PIN_A, GPIO_OUT);
    gpio_set_dir(PIN_B, GPIO_OUT);
    gpio_set_dir(PIN_C, GPIO_OUT);
    gpio_set_dir(PIN_D, GPIO_OUT);

    // Pin output map for each step
    const uint8_t pin_map[8][4] = {
        {1, 0, 0, 0},
        {1, 1, 0, 0},
        {0, 1, 0, 0},
        {0, 1, 1, 0},
        {0, 0, 1, 0},
        {0, 0, 1, 1},
        {0, 0, 0, 1},
        {1, 0, 0, 1},
    };

    uint64_t time_us = 0;
    uint64_t time_last_us = 0;

    while (true)
    {
        if (abs(velocity) < 0.01)
            continue;
        float step = (1000000.0f / 400.f) / (abs(velocity));
        time_us = time_us_64();
        if (time_us - time_last_us > step)
        {
            time_last_us = time_us;
            for (int i = 0; i < 4; i++)
            {
                gpio_put(PIN_A + i, pin_map[position & 0x07][i]);
            }
            position += velocity > 0 ? 1 : -1;
        }
    }
}

int main()
{
    stdio_init_all();
    while (!stdio_usb_connected())
    {
        sleep_ms(10);
    }
    multicore_launch_core1(core1_main);
    core0_main();
    return 0;
}
