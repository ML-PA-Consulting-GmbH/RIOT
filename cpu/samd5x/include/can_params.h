#ifndef CAN_PARAMS_H
#define CAN_PARAMS_H

#include "can/device.h"
#include "candev_samd5x.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Default SAMD5x CAN devices config */
static const can_conf_t candev_conf[] = {
    {
        .can = CAN0,
        .rx_pin = GPIO_PIN(PA, 23),
        .tx_pin = GPIO_PIN(PA, 22),
        .mux = GPIO_MUX_I,
        .tdc_ctrl = false,
        .dar_ctrl = false,
        .tx_fifo_queue_ctrl = false,
    },
    {
        /* SAME54-xpro pins */
        .can = CAN1,
        .rx_pin = GPIO_PIN(PB, 13),
        .tx_pin = GPIO_PIN(PB, 12),
        .mux = GPIO_MUX_H,
        .tdc_ctrl = false,
        .tx_fifo_queue_ctrl = true,
    }
};

/** Default SAMD5x CAN devices parameters */
static const candev_params_t candev_params[] = {
    {
        .name = "can_samd5x_0",
    },
    {
        .name = "can_samd5x_1",
    }
};

#ifdef __cplusplus
}
#endif

#endif /* CAN_PARAMS_H */
