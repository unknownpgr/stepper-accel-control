# Inverted Pendulum

## Notes

- UART / Serial 통신 예제

```c
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#define UART_ID uart0
#define BAUD_RATE 115200

// UART TX/RX 핀 번호
#define UART_TX_PIN 0
#define UART_RX_PIN 1

int main() {
    // 표준 IO 초기화 (USB Serial)
    stdio_init_all();

    // UART 초기화
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // USB Serial과 UART가 초기화되었음을 알리기 위한 메시지 출력
    printf("USB Serial and UART Initialized\n");

    // UART로 보내기 위한 데이터
    const char *uart_message = "Hello from UART0!\n";

    while (true) {
        // USB 시리얼에서 입력 받기 (blocking 방식)
        int c = getchar_timeout_us(0);  // 비차단 입력 (timeout 설정)
        if (c != PICO_ERROR_TIMEOUT) {
            printf("USB received: %c\n", c);
            // UART0로 데이터 전송
            uart_putc(UART_ID, c);
        }

        // UART에서 입력 받기 (비차단 방식)
        if (uart_is_readable(UART_ID)) {
            char uart_char = uart_getc(UART_ID);
            // 수신한 데이터를 USB Serial로 출력
            printf("UART received: %c\n", uart_char);
        }

        // UART로 주기적으로 메시지 보내기
        uart_puts(UART_ID, uart_message);
        sleep_ms(1000);
    }

    return 0;
}
```

Cart-pole 시스템 모델링

- Note: stepper motor를 사용하므로 acceleration을 곧바로 제어 입력으로 사용할 수 있다고 가정한다.

$$
\ddot{x} = \frac{F + m\sin\theta(l\dot{\theta}^2 + g\cos\theta)}{M + m\sin^2\theta}
$$

$$
\ddot{\theta} = \frac{g\sin\theta-\cos\theta\ddot{x}}{l}
$$

이것의 선형화는 다음과 같다.

$$
\ddot{x} = \frac{F}{M}
$$

$$
\ddot{\theta} = \frac{g\sin\theta}{l}
$$

이것을 상태 행렬로 표현하면 다음과 같다.
