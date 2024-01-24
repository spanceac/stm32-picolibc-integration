#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include "stm32f10x.h"
#include "stm32-ints.h"

#define bit_set(var,bitno) ((var) |= 1 << (bitno))
#define bit_clr(var,bitno) ((var) &= ~(1 << (bitno)))
#define testbit(var,bitno) (((var)>>(bitno)) & 0x01)

#define SYS_CLK_HZ 8000000
#define TMR_FREQ_HZ 1
#define TMR_PRESC 1000
#define TMR_LOAD_VAL (((SYS_CLK_HZ) / (TMR_PRESC)) / (TMR_FREQ_HZ))

#define TMR2_IRQ_NO 28

char test[2];

void IRQ_enable(unsigned char nr)
{
    unsigned char base = nr / 32;

    if (nr > 80) {
        /* unsupported IRQ NR */
        return;
    }

    bit_set(NVIC->ISER[base], nr % 32);
}

void TIM2_IRQHandler(void)
{
    /* clear UIF flag(IRQ) */
    bit_clr(TIM2->SR, 0);

    memcpy(test, "on", sizeof(test));
}

void timer2_init(void)
{
    bit_set(RCC->APB1ENR, 0); /* enable TMR2 clock */
    TIM2->ARR = TMR_LOAD_VAL;
    TIM2->PSC = TMR_PRESC - 1;
    TIM2->CR1 = 0;
    TIM2->CR2 = 0;
    bit_set(TIM2->CR1, 7); /* auto reload */
    bit_set(TIM2->CR1, 0); /* counter enable */
    bit_set(TIM2->DIER, 0); /* TMR2 interrupt enable */
}

void set_gpio_out(GPIO_TypeDef *gpio_bank, unsigned char pin)
{
    vu32 *dest;
    unsigned char pin_offset;

    if (pin > 15)
        return;

    dest = pin > 7 ? &(gpio_bank->CRH) : &(gpio_bank->CRL);
    pin_offset = pin > 7 ? pin - 8 : pin;

    /* output 10MHz */
    bit_set(*dest, pin_offset * 4);
    bit_clr(*dest, pin_offset * 4 + 1);
    bit_clr(*dest, pin_offset * 4 + 2);
    bit_clr(*dest, pin_offset * 4 + 3);
}

void set_gpio_in(GPIO_TypeDef *gpio_bank, unsigned char pin)
{
    vu32 *dest;
    dest = pin > 7 ? &(gpio_bank->CRH) : &(gpio_bank->CRL);

    /* input with pull-up / pull-down */
    bit_clr(*dest, pin * 4);
    bit_clr(*dest, pin * 4 + 1);
    bit_clr(*dest, pin * 4 + 2);
    bit_set(*dest, pin * 4 + 3);
}

int main(void)
{
    bool gpio_on = false;

    bit_set(RCC->APB2ENR, 4); /* Enable the GPIOC (bit 4) */

    set_gpio_out(GPIOC, 9);

    timer2_init();
    IRQ_enable(TMR2_IRQ_NO);

    while(1) {
        if (!memcmp(test, "on", sizeof(test))) {
            gpio_on ? bit_clr(GPIOC->ODR, 9) : bit_set(GPIOC->ODR, 9);
            gpio_on = !gpio_on;
            memset(test, 0, sizeof(test));
        }
    }
}

void nmi_handler(void)
{
    return;
}

void hardfault_handler(void)
{
    return;
}
