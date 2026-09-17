#include "stm32f446xx.h"

// Yksinkertainen viivefunktio (ohittaa kääntäjän optimoinnin volatile-avainsanalla)
void delay(uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        // asm volatile estää kääntäjää poistamasta tyhjää silmukkaa
        asm volatile ("nop");
    }
}

int main(void) {
    // 1. Kytketään kello päälle GPIOA-portille (RCC AHB1 peripheral clock enable register)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // 2. Asetetaan PA5-pinni lähtötilaan (General purpose output mode: '01')
    GPIOA->MODER &= ~(3U << (5 * 2)); // Tyhjennetään bitit 10 ja 11
    GPIOA->MODER |=  (1U << (5 * 2)); // Asetetaan bitti 10 arvoon 1

    // 3. Pääsilmukka
    while (true) {
        // Vaihdetaan PA5-pinnin tila (Toggle bit 5)
        GPIOA->ODR ^= GPIO_ODR_OD5;

        // Odotetaan hetki (~16 MHz oletuskellotaajuudella noin puoli sekuntia)
        delay(800000);
    }

    return 0;
}