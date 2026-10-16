#ifndef __LED_H
#define __LED_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef struct Led Led;

typedef struct {
    void    (*on)(Led *self);
    void    (*off)(Led *self);
    void    (*toggle)(Led *self);
    uint8_t (*is_on)(Led *self);
} LedOps;

struct Led {
    GPIO_TypeDef *port;
    uint16_t      pin;
    uint8_t       active_low;
    uint8_t       state;
    const LedOps *ops;
};

void    Led_Init(Led *self, GPIO_TypeDef *port, uint16_t pin, uint8_t active_low);
void    Led_SetState(Led *self, uint8_t on);
void    Led_UpdatePin(Led *self);

#endif