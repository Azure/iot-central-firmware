// See LICENSE.md

#include "mbed.h"

DigitalOut led(LED1);

int main()
{
    while(1) {
        led = 1; // LED is ON
        wait(1); // 1 s
        led = 0; // LED is OFF
        wait(1.5); // 1500 ms
    }
}
