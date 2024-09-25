#include <stdio.h>
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"

#define abs(x) ((x) < 0 ? -(x) : (x))

// UART configuration
#define UART_ID uart0
#define BAUD_RATE 9600
#define UART_TX_PIN 0
#define UART_RX_PIN 1

volatile uint32_t position = 0; // Steps
volatile float velocity = 0;    // Revolutions per second

/**
 * Core 0 is responsible for acceleration control
 */
void core0_main()
{
    // USB serial initialization
    stdio_init_all();
    while (!stdio_usb_connected())
    {
        sleep_ms(10);
    }
    printf("USB serial initialized\n");

    // UART initialization (for encoder sensor reading)
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    printf("UART initialized\n");

    // Acceleration control variables
    float acceleration = 0.0f;
    uint32_t previous_time = time_us_32();
    int32_t sign = 1;
    int32_t acceleration_buffer = 0;

    // Log variables
    uint32_t last_log_time = time_us_32();
    uint32_t log_interval = 1000000; // 1 second

    // Main loop
    while (true)
    {
        // Update velocity
        uint32_t current_time = time_us_32();
        uint32_t delta_time = current_time - previous_time;
        previous_time = current_time;
        velocity += acceleration * delta_time / 1000000.0f;

        // Read acceleration from USB serial
        int c = getchar_timeout_us(0);
        if (c == '-')
        {
            sign = -1;
        }
        else if (c >= '0' && c <= '9')
        {
            acceleration_buffer = acceleration_buffer * 10 + (c - '0');
        }
        else if (c == '\r' || c == '\n')
        {
            acceleration = sign * acceleration_buffer / 100.0f;
            sign = 1;
            acceleration_buffer = 0;
            printf("Acceleration set: %f\n", acceleration);
        }

        // Log acceleration every second
        if (current_time - last_log_time >= log_interval)
        {
            last_log_time += log_interval;
            printf("Position: %lu\n", position);
            printf("Velocity: %f\n", velocity);
        }
    }
}

/**
 * Core 1 is responsible for driving the stepper motor
 */
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

    uint64_t max_step_time_us = 100000; // 100ms
    uint64_t time_us = 0;
    uint64_t time_last_us = 0;

    while (true)
    {
        time_us = time_us_64();

        if (abs(velocity) > 0.01)
        {
            float interval = (1000000.0f / 400.f) / (abs(velocity));

            // Step the motor if the time since the last step is greater than the interval
            if (time_us - time_last_us > interval)
            {
                time_last_us = time_us;
                for (int i = 0; i < 4; i++)
                {
                    gpio_put(PIN_A + i, pin_map[position & 0x07][i]);
                }
                position += velocity > 0 ? 1 : -1;
            }
        }

        // Limit the step holding time to max_step_time_us
        if (time_us - time_last_us > max_step_time_us)
        {
            for (int i = 0; i < 4; i++)
            {
                gpio_put(PIN_A + i, 0);
            }
        }
    }
}

/**
 * Entrypoint
 */
int main()
{
    multicore_launch_core1(core1_main);
    core0_main();
    return 0;
}
