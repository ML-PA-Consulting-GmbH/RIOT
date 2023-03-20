/**
 * @ingroup     cpu_samd5x
 * @{
 *
 * @file
 * @brief       Implementation of the CAN controller driver
 *
 * @author      Firas Hamdi <firas.hamdi@ml-pa.com>
 * @}
 */

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <cpu_conf.h>

#include "candev_samd5x.h"
#include "periph/can.h"
#include "can/device.h"
#include "periph_cpu.h"

#define ENABLE_DEBUG 1
#include "debug.h"

typedef enum {
    MODE_INIT,
    MODE_TEST,
    MODE_MONITORING
} can_mode_t;

static int _init(candev_t *candev);
static int _send(candev_t *candev, const struct can_frame *frame);

static int _set_mode(Can *can, can_mode_t mode);

static const candev_driver_t candev_samd5x_driver = {
    .init = _init,
    .send = _send,
};

static const struct can_bittiming_const bittiming_const = {
    .tseg1_min = 1,
    .tseg1_max = 256,
    .tseg2_min = 1,
    .tseg2_max = 128,
    .sjw_max = 1,
    .brp_min = 1,
    .brp_max = 512,
    .brp_inc = 1,
};

static int _power_on(can_t *dev)
{
    if (dev->conf->can == CAN0) {
        DEBUG_PUTS("CAN0 controller is used");
        MCLK->AHBMASK.reg |= MCLK_AHBMASK_CAN0;
    }
    else if (dev->conf->can == CAN1) {
        DEBUG_PUTS("CAN1 controller is used");
        MCLK->AHBMASK.reg |= MCLK_AHBMASK_CAN1;
    }
    else {
        DEBUG_PUTS("Unsupported CAN channel");
        return -1;
    }

    return 0;
}

static int _set_mode(Can *can, can_mode_t can_mode)
{
    switch (can_mode) {
        case MODE_INIT:
            DEBUG_PUTS("Initialization mode");
            can->CCCR.reg |= CAN_CCCR_INIT | CAN_CCCR_CCE;
            break;
        case MODE_TEST:
            DEBUG_PUTS("Test mode");
            can->CCCR.reg |= CAN_CCCR_INIT | CAN_CCCR_CCE;
            can->CCCR.reg |= CAN_CCCR_TEST;
            can->TEST.reg |= CAN_TEST_LBCK;
            break;
        case MODE_MONITORING:
            DEBUG_PUTS("Monitoring mode");
            can->CCCR.reg |= CAN_CCCR_INIT | CAN_CCCR_CCE;
            can->CCCR.reg |= CAN_CCCR_MON;
            break;
        default:
            DEBUG_PUTS("Unsupported mode");
            return -1;
    }

    return 0;
}

static void _setup_clock(can_t *dev)
{
    if (dev->conf->can == CAN0) {
        GCLK->PCHCTRL[CAN0_GCLK_ID].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN(SAM0_GCLK_MAIN);
    }
    else if (dev->conf->can == CAN1) {
        GCLK->PCHCTRL[CAN1_GCLK_ID].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN(SAM0_GCLK_MAIN);
    }
    else {
        DEBUG_PUTS("CAN channel not supported");
    }
}

static void _set_bit_timing(can_t *dev)
{
    assert(dev->candev.bittiming.sjw >= 1);
    assert(dev->candev.bittiming.phase_seg2 >= 1);
    assert(dev->candev.bittiming.phase_seg1 + dev->candev.bittiming.prop_seg >= 1);
    assert(dev->candev.bittiming.brp >= 1);

    DEBUG("bitrate=%" PRIu32 ", sample_point=%" PRIu32 ", brp=%" PRIu32 ", prop_seg=%" PRIu32
          ", phase_seg1=%" PRIu32 ", phase_seg2=%" PRIu32 ", sjw=%" PRIu32 "\n", dev->candev.bittiming.bitrate, dev->candev.bittiming.sample_point,
          dev->candev.bittiming.brp, dev->candev.bittiming.prop_seg, dev->candev.bittiming.phase_seg1, dev->candev.bittiming.phase_seg2, dev->candev.bittiming.sjw);

    /* Set bit timing */
    dev->conf->can->NBTP.reg = (uint32_t)((CAN_NBTP_NTSEG2(dev->candev.bittiming.phase_seg2 - 1))
                            | (CAN_NBTP_NTSEG1(dev->candev.bittiming.phase_seg1 + dev->candev.bittiming.prop_seg - 1))
                            | (CAN_NBTP_NBRP(dev->candev.bittiming.brp - 1))
                            | (CAN_NBTP_NSJW(dev->candev.bittiming.sjw - 1)));
}

void candev_samd5x_set_pins(can_t *dev)
{
    assert(dev->conf->tx_pin != GPIO_UNDEF);
    assert(dev->conf->rx_pin != GPIO_UNDEF);

    gpio_init(dev->conf->tx_pin, GPIO_OUT);
    gpio_init(dev->conf->rx_pin, GPIO_IN_PU);
}

void candev_samd5x_tdc_control(can_t *dev)
{
    if(dev->conf->tdc_ctrl) {
        DEBUG_PUTS("Enable Transceiver Delay Compensation");
        dev->conf->can->DBTP.reg |= CAN_DBTP_TDC;
    }
    else {
        DEBUG_PUTS("Disable Transceiver Delay Compensation");
        dev->conf->can->DBTP.reg &= ~(CAN_DBTP_TDC);
    }
}

void candev_samd5x_enter_test_mode(candev_t *candev)
{
    can_t *dev = container_of(candev, can_t, candev);

    DEBUG_PUTS("entering test mode");
    _set_mode(dev->conf->can, MODE_TEST);
}

void can_init(can_t *dev, const can_conf_t *conf)
{
    DEBUG("CCCR register = 0x%08lx\n", dev->conf->can->CCCR.reg);
    dev->candev.driver = &candev_samd5x_driver;

    struct can_bittiming timing = { .bitrate = CANDEV_SAMD5X_DEFAULT_BITRATE,
                                    .sample_point = CANDEV_SAMD5X_DEFAULT_SPT };

    uint32_t clk_freq = sam0_gclk_freq(SAM0_GCLK_MAIN);
    DEBUG("clock frequency = %lu\n", clk_freq);
    can_device_calc_bittiming(clk_freq, &bittiming_const, &timing);

    memcpy(&dev->candev.bittiming, &timing, sizeof(timing));
    dev->conf = conf;
}

static int _init(candev_t *candev)
{
    can_t *dev = container_of(candev, can_t, candev);
    int res = 0;

    sam0_gclk_enable(SAM0_GCLK_MAIN);

    _setup_clock(dev);
    _power_on(dev);

    candev_samd5x_set_pins(dev);
    res = _set_mode(dev->conf->can, MODE_INIT);

    candev_samd5x_tdc_control(dev);

    /*Configure the start addresses of the RAM message sections */
    dev->conf->can->SIDFC.reg = CAN_SIDFC_FLSSA((uint32_t)(dev->msg_ram_conf.std_filter)) | CAN_SIDFC_LSS((uint32_t)(ARRAY_SIZE(dev->msg_ram_conf.std_filter)));
    DEBUG("start address = %p\n", dev->msg_ram_conf.std_filter);
    DEBUG("SIDFC = 0x%08lx\n", dev->conf->can->SIDFC.reg);
    dev->conf->can->XIDFC.reg = CAN_XIDFC_FLESA((uint32_t)(dev->msg_ram_conf.ext_filter)) | CAN_XIDFC_LSE((uint32_t)(ARRAY_SIZE(dev->msg_ram_conf.ext_filter)));
    DEBUG("start address = %p\n", dev->msg_ram_conf.ext_filter);
    DEBUG("XIDFC = 0x%08lx\n", dev->conf->can->XIDFC.reg);
    dev->conf->can->RXF0C.reg = CAN_RXF0C_F0SA((uint32_t)(dev->msg_ram_conf.rx_fifo_0)) | CAN_RXF0C_F0S((uint32_t)(ARRAY_SIZE(dev->msg_ram_conf.rx_fifo_0)));
    DEBUG("start address = %p\n", dev->msg_ram_conf.rx_fifo_0);
    DEBUG("RXF0C = 0x%08lx\n", dev->conf->can->RXF0C.reg);
    dev->conf->can->RXF1C.reg = CAN_RXF0C_F0SA((uint32_t)(dev->msg_ram_conf.rx_fifo_1)) | CAN_RXF0C_F0S((uint32_t)(ARRAY_SIZE(dev->msg_ram_conf.rx_fifo_1)));
    DEBUG("start address = %p\n", dev->msg_ram_conf.rx_fifo_1);
    DEBUG("RXF1C = 0x%08lx\n", dev->conf->can->RXF1C.reg);
    dev->conf->can->RXBC.reg = CAN_RXBC_RBSA((uint32_t)(dev->msg_ram_conf.rx_buffer));
    DEBUG("start address = %p\n", dev->msg_ram_conf.rx_buffer);
    DEBUG("RXBC = 0x%08lx\n", dev->conf->can->RXBC.reg);
    dev->conf->can->TXEFC.reg = CAN_TXEFC_EFSA((uint32_t)(dev->msg_ram_conf.tx_event_fifo)) | CAN_TXEFC_EFS((uint32_t)(ARRAY_SIZE(dev->msg_ram_conf.tx_event_fifo)));
    DEBUG("start address = %p\n", dev->msg_ram_conf.tx_event_fifo);
    DEBUG("TXEFC = 0x%08lx\n", dev->conf->can->TXEFC.reg);
    dev->conf->can->TXBC.reg = CAN_TXBC_TBSA((uint32_t)(dev->msg_ram_conf.tx_fifo_queue)) /*| CAN_TXBC_NDTB((uint32_t)(ARRAY_SIZE(dev->msg_ram_conf.tx_buffer))) */
                                    | CAN_TXBC_TFQS((uint32_t)(ARRAY_SIZE(dev->msg_ram_conf.tx_fifo_queue)));
    DEBUG("start address = %p\n", dev->msg_ram_conf.tx_fifo_queue);
    DEBUG("TXBC = 0x%08lx\n", dev->conf->can->TXBC.reg);

    /* Control FIFO/queue op */

    /* Enable the interrupts */

    _set_bit_timing(dev);

    return res;
}

static void _exit_init_mode(Can *can)
{
    if (can->CCCR.reg &= 0x1) {
        DEBUG_PUTS("exiting Initialization");
        can->CCCR.reg &= ~CAN_CCCR_INIT;
    }
    else {
        DEBUG_PUTS("Already out of init mode");
    }
}

static int _send(candev_t *candev, const struct can_frame *frame)
{
    can_t *dev = container_of(candev, can_t, candev);

    if (frame->can_dlc > CAN_MAX_DLEN) {
        DEBUG_PUTS("CAN frame payload not supported");
        return -1;
    }

    _exit_init_mode(dev->conf->can);

    can_mm_t can_mm = {0};
    DEBUG("TXFQS = 0x%08lx\n", dev->conf->can->TXFQS.reg);
    uint8_t fifo_queue_put_idx = (dev->conf->can->TXFQS.reg & 0x001F0000) >> 16;
    uint8_t fifo_queue_get_idx = (dev->conf->can->TXFQS.reg & 0x00001F00) >> 8;
    can_mm.put = fifo_queue_put_idx;
    can_mm.get = fifo_queue_get_idx;

    dev->msg_ram_conf.tx_fifo_queue[can_mm.put].T0.id = frame->can_id & CAN_EFF_MASK;
    dev->msg_ram_conf.tx_fifo_queue[can_mm.put].T0.xtd = (frame->can_id & CAN_EFF_FLAG) >> 31;
    dev->msg_ram_conf.tx_fifo_queue[can_mm.put].T0.rtr = (frame->can_id & CAN_RTR_FLAG) >> 30;
    dev->msg_ram_conf.tx_fifo_queue[can_mm.put].T1.dlc = frame->can_dlc;
    dev->msg_ram_conf.tx_fifo_queue[can_mm.put].T1.efc = 1;
    memcpy(dev->msg_ram_conf.tx_fifo_queue[can_mm.put].data.data_8, frame->data, frame->can_dlc);
    memcpy(&(dev->msg_ram_conf.tx_fifo_queue[can_mm.put].T1.mm), &can_mm, sizeof(can_mm_t));

    DEBUG("T1 mm get = %u\n", dev->msg_ram_conf.tx_fifo_queue[can_mm.put].T1.mm.get);
    DEBUG("T1 mm put = %u\n", dev->msg_ram_conf.tx_fifo_queue[can_mm.put].T1.mm.put);

    DEBUG("TXEFS = 0x%08lx\n", dev->conf->can->TXEFS.reg);
    uint8_t fifo_event_put_index = (dev->conf->can->TXEFS.reg & 0x001F0000) >> 16;
    // uint8_t fifo_event_get_index = (dev->conf->can->TXEFS.reg & 0x00001F00) >> 8;
    dev->msg_ram_conf.tx_event_fifo[fifo_event_put_index].E0.id = frame->can_id & CAN_EFF_MASK;
    dev->msg_ram_conf.tx_event_fifo[fifo_event_put_index].E0.rtr = (frame->can_id & CAN_EFF_FLAG) >> 31;
    dev->msg_ram_conf.tx_event_fifo[fifo_event_put_index].E0.xtd = (frame->can_id & CAN_RTR_FLAG) >> 30;
    memcpy(&(dev->msg_ram_conf.tx_event_fifo[fifo_event_put_index].E1.mm), &(dev->msg_ram_conf.tx_fifo_queue[can_mm.put].T1.mm), sizeof(can_mm_t));
    dev->msg_ram_conf.tx_event_fifo[fifo_event_put_index].E1.et = 0x1;
    dev->msg_ram_conf.tx_event_fifo[fifo_event_put_index].E1.dlc = frame->can_dlc;

    dev->conf->can->TXBAR.reg |= (1 << can_mm.put);
    DEBUG("TXFQS = 0x%08lx\n", dev->conf->can->TXFQS.reg);
    DEBUG("TXBRP = 0x%08lx\n", dev->conf->can->TXBRP.reg);
    DEBUG("TXBTO = 0x%08lx\n", dev->conf->can->TXBTO.reg);

    return 0;
}