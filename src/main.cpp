#include "stm32f446xx.h"

// Yksinkertainen viivefunktio (ohittaa kääntäjän optimoinnin volatile-avainsanalla)
void delay(uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        // asm volatile estää kääntäjää poistamasta tyhjää silmukkaa
        asm volatile ("nop");
    }
}

int main(void) {

    // 3. Pääsilmukka
    while (true) {

        delay(800000);
    }

    return 0;
}