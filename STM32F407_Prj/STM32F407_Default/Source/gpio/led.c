#include "led.h"

static void    led_on(Led *self)    { self->state = 1; Led_UpdatePin(self); }
static void    led_off(Led *self)   { self->state = 0; Led_UpdatePin(self); }
static void    led_toggle(Led *self){ self->state = !self->state; Led_UpdatePin(self); }
static uint8_t led_is_on(Led *self) { return self->state; }

static const LedOps s_led_ops = {
    .on     = led_on,
    .off    = led_off,
    .toggle = led_toggle,
    .is_on  = led_is_on,
};

void Led_Init(Led *self, GPIO_TypeDef *port, uint16_t pin, uint8_t active_low)
{
    self->port       = port;
    self->pin        = pin;
    self->active_low = active_low;
    self->state      = 0;
    self->ops        = &s_led_ops;
    Led_UpdatePin(self);
}

void Led_SetState(Led *self, uint8_t on)
{
    self->state = on ? 1 : 0;
    Led_UpdatePin(self);
}

void Led_UpdatePin(Led *self)
{
    GPIO_PinState level;
    if (self->active_low) {
        level = self->state ? GPIO_PIN_RESET : GPIO_PIN_SET;
    } else {
        level = self->state ? GPIO_PIN_SET   : GPIO_PIN_RESET;
    }
    HAL_GPIO_WritePin(self->port, self->pin, level);
}