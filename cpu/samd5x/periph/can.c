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
} can_mode_t;

static can_t *_can;

static int _init(candev_t *candev);
static int _send(candev_t *candev, const struct can_frame *frame);
static int _set_filter(candev_t *candev, const struct can_filter *filter);
static int _remove_filter(candev_t *candev, const struct can_filter *filter);
static void _isr(candev_t *candev);

static int _set_mode(Can *can, can_mode_t mode);

static const candev_driver_t candev_samd5x_driver = {
    .init = _init,
    .send = _send,
    .set_filter = _set_filter,
    .remove_filter = _remove_filter,
    .isr = _isr,
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

static void _enter_init_mode(Can *can)
{
    can->CCCR.reg |= CAN_CCCR_INIT;
    while(!(can->CCCR.reg & CAN_CCCR_INIT));
    DEBUG_PUTS("Device in init mode");
}

static void _exit_init_mode(Can *can)
{
    if (can->CCCR.reg & CAN_CCCR_INIT) {
        can->CCCR.reg &= ~CAN_CCCR_INIT;
    }

    while(can->CCCR.reg & CAN_CCCR_INIT);
    DEBUG_PUTS("Device out of init mode");
}

static void _enter_sleep_mode(Can *can)
{
    can->CCCR.reg |= CAN_CCCR_CSR;
    while(!(can->CCCR.reg & CAN_CCCR_CSA));
    DEBUG("CCCR = 0x%08lx\n", can->CCCR.reg);
    DEBUG_PUTS("Device in sleep mode");
}

static void _exit_sleep_mode(Can *can)
{
    can->CCCR.reg &= ~CAN_CCCR_CSR;
    while(can->CCCR.reg & CAN_CCCR_CSA);
    DEBUG_PUTS("Device out of sleep mode");
}

static int _set_mode(Can *can, can_mode_t can_mode)
{
    switch (can_mode) {
        case MODE_INIT:
            _enter_init_mode(can);
            can->CCCR.reg |= CAN_CCCR_CCE;
            break;
        case MODE_TEST:
            DEBUG_PUTS("test mode");
            _enter_init_mode(can);
            can->CCCR.reg |= CAN_CCCR_CCE;
            can->CCCR.reg |= CAN_CCCR_TEST;
            can->TEST.reg |= CAN_TEST_LBCK;
            _exit_init_mode(can);
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
        GCLK->PCHCTRL[CAN0_GCLK_ID].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN(SAM0_GCLK_PERIPH);
    }
    else if (dev->conf->can == CAN1) {
        GCLK->PCHCTRL[CAN1_GCLK_ID].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN(SAM0_GCLK_PERIPH);
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
    gpio_init(dev->conf->rx_pin, GPIO_IN);

    gpio_init_mux(dev->conf->tx_pin, dev->conf->mux);
    gpio_init_mux(dev->conf->rx_pin, dev->conf->mux);
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

    _set_mode(dev->conf->can, MODE_TEST);
}

void can_init(can_t *dev, const can_conf_t *conf)
{
    dev->candev.driver = &candev_samd5x_driver;

    struct can_bittiming timing = { .bitrate = CANDEV_SAMD5X_DEFAULT_BITRATE,
                                    .sample_point = CANDEV_SAMD5X_DEFAULT_SPT };

    uint32_t clk_freq = sam0_gclk_freq(SAM0_GCLK_PERIPH);
    can_device_calc_bittiming(clk_freq, &bittiming_const, &timing);

    memcpy(&dev->candev.bittiming, &timing, sizeof(timing));
    dev->conf = conf;
}

static int _init(candev_t *candev)
{
    can_t *dev = container_of(candev, can_t, candev);
    int res = 0;

    sam0_gclk_enable(SAM0_GCLK_PERIPH);

    _setup_clock(dev);
    _power_on(dev);

    _enter_sleep_mode(dev->conf->can);
    candev_samd5x_set_pins(dev);
    _exit_sleep_mode(dev->conf->can);

    res = _set_mode(dev->conf->can, MODE_INIT);

    _set_bit_timing(dev);

    candev_samd5x_tdc_control(dev);

    /*Configure the start addresses of the RAM message sections */
    dev->conf->can->SIDFC.reg = CAN_SIDFC_FLSSA((uint32_t)(dev->msg_ram_conf.std_filter)) | CAN_SIDFC_LSS((uint32_t)(ARRAY_SIZE(dev->msg_ram_conf.std_filter)));
    dev->conf->can->XIDFC.reg = CAN_XIDFC_FLESA((uint32_t)(dev->msg_ram_conf.ext_filter)) | CAN_XIDFC_LSE((uint32_t)(ARRAY_SIZE(dev->msg_ram_conf.ext_filter)));
    dev->conf->can->RXF0C.reg = CAN_RXF0C_F0SA((uint32_t)(dev->msg_ram_conf.rx_fifo_0)) | CAN_RXF0C_F0S((uint32_t)(ARRAY_SIZE(dev->msg_ram_conf.rx_fifo_0)));
    dev->conf->can->RXF1C.reg = CAN_RXF1C_F1SA((uint32_t)(dev->msg_ram_conf.rx_fifo_1)) | CAN_RXF1C_F1SA((uint32_t)(ARRAY_SIZE(dev->msg_ram_conf.rx_fifo_1)));
    dev->conf->can->RXBC.reg = CAN_RXBC_RBSA((uint32_t)(dev->msg_ram_conf.rx_buffer));

    dev->conf->can->TXEFC.reg = CAN_TXEFC_EFSA((uint32_t)(dev->msg_ram_conf.tx_event_fifo)) | CAN_TXEFC_EFS((uint32_t)(ARRAY_SIZE(dev->msg_ram_conf.tx_event_fifo)));

    dev->conf->can->TXBC.reg = CAN_TXBC_TBSA((uint32_t)(dev->msg_ram_conf.tx_fifo_queue)) | CAN_TXBC_TFQS((uint32_t)(ARRAY_SIZE(dev->msg_ram_conf.tx_fifo_queue)));

    dev->conf->can->TXESC.reg = CAN_TXESC_TBDS_DATA8;

    dev->conf->can->CCCR.reg |= CAN_CCCR_DAR;

    /* Control FIFO/queue op */

    /* Enable the peripheral's interrupt */
    if (dev->conf->can == CAN0) {
        NVIC_EnableIRQ(CAN0_IRQn);
    }
    else {
        NVIC_EnableIRQ(CAN1_IRQn);
    }

    dev->conf->can->IE.reg |= CAN_IE_RF0NE;
    /* Enable the interrupt lines */
    dev->conf->can->ILE.reg = CAN_ILE_EINT0 | CAN_ILE_EINT1;
    /* Enable the interrupt on every Tx buffer transmission */
    // dev->conf->can->TXBTIE.reg = CAN_TXBTIE_MASK;
    dev->conf->can->IE.reg |= CAN_IE_TEFNE;

    _can = dev;

    /* Exit initialization mode */
    _exit_init_mode(dev->conf->can);

    return res;
}

/* Maybe do not provide this API to the user and init the interrupts according to your use case */
int candev_samd5x_irq_init(candev_t *candev, can_irq_source_t irq_source, can_irq_line_t irq_line)
{
    can_t *dev = container_of(candev, can_t, candev);

    assert((irq_line == CAN_IRQ_LINE_0) | (irq_line == CAN_IRQ_LINE_1));

    dev->conf->can->IE.reg |= (1 << irq_source);
    DEBUG("IE = 0x%08lx\n", dev->conf->can->IE.reg);

    if (irq_line == CAN_IRQ_LINE_0) {
        dev->conf->can->ILS.reg &= ~(1 << irq_source);
    }
    else {
        dev->conf->can->ILS.reg |= (1 << irq_source);
    }
    DEBUG("ILS = 0x%08lx\n", dev->conf->can->ILS.reg);

    return 0;
}

static int _send(candev_t *candev, const struct can_frame *frame)
{
    can_t *dev = container_of(candev, can_t, candev);

    if (frame->can_dlc > CAN_MAX_DLEN) {
        DEBUG_PUTS("CAN frame payload not supported");
        return -1;
    }

    /* Check if the Tx FIFO is full */
    if (dev->conf->can->TXFQS.reg & CAN_TXFQS_TFQF) {
        DEBUG_PUTS("Tx FIFO is full");
        return -1;
    }

    can_mm_t can_mm = {0};
    uint8_t fifo_queue_put_idx = (dev->conf->can->TXFQS.reg & 0x001F0000) >> 16;
    DEBUG("Tx FIFO put index = %u\n", fifo_queue_put_idx);
    uint8_t fifo_queue_get_idx = (dev->conf->can->TXFQS.reg & 0x00001F00) >> 8;
    DEBUG("Tx FIFO get index = %u\n", fifo_queue_get_idx);
    can_mm.put = fifo_queue_put_idx;
    can_mm.get = fifo_queue_get_idx;

    /* Add frame to the Tx FIFO */
    if (frame->can_id & CAN_EFF_FLAG) {
        DEBUG_PUTS("Extended ID");
        dev->msg_ram_conf.tx_fifo_queue[can_mm.put].T0.id = frame->can_id & CAN_EFF_MASK;
        dev->msg_ram_conf.tx_fifo_queue[can_mm.put].T0.xtd = 1;
    }
    else {
        DEBUG_PUTS("Standard identifier");
        dev->msg_ram_conf.tx_fifo_queue[can_mm.put].T0.id = (frame->can_id & CAN_SFF_MASK) << 18;
        dev->msg_ram_conf.tx_fifo_queue[can_mm.put].T0.xtd = 0;
    }
    dev->msg_ram_conf.tx_fifo_queue[can_mm.put].T0.rtr = (frame->can_id & CAN_RTR_FLAG) >> 30;
    dev->msg_ram_conf.tx_fifo_queue[can_mm.put].T0.esi = 1;
    dev->msg_ram_conf.tx_fifo_queue[can_mm.put].T1.dlc = frame->can_dlc;
    dev->msg_ram_conf.tx_fifo_queue[can_mm.put].T1.efc = 1;
    memcpy(&(dev->msg_ram_conf.tx_fifo_queue[can_mm.put].T1.mm), &can_mm, sizeof(can_mm_t));
    memcpy(dev->msg_ram_conf.tx_fifo_queue[can_mm.put].data.data_8, frame->data, frame->can_dlc);

    /* Request transmission */
    dev->conf->can->TXBAR.reg |= (1 << can_mm.put);

    /* Wait for successful transmission */
    while(!(dev->conf->can->TXBTO.reg & (1 << can_mm.put)));

    // if(dev->candev.event_callback){
    //     dev->candev.event_callback(&(dev->candev), CANDEV_EVENT_TX_CONFIRMATION, (void *)frame);
    // }
    return 0;
}

static bool _find_filter(can_t *can, const struct can_filter *filter, bool is_std_filter, int16_t *idx)
{
    if (is_std_filter) {
        for (uint8_t i = 0; i < ARRAY_SIZE(can->msg_ram_conf.std_filter); i++) {
            if (((filter->can_id & CAN_SFF_MASK) == can->msg_ram_conf.std_filter[i].sfid1)) {
                *idx = i;
                return true;
            }
        }
    }
    else {
        for (uint8_t i = 0; i < ARRAY_SIZE(can->msg_ram_conf.ext_filter); i++) {
            if (((filter->can_id & CAN_EFF_MASK) == can->msg_ram_conf.ext_filter[i].F0.efid1)) {
                *idx = i;
                return true;
            }
        }
    }

    return false;
}

static int _set_filter(candev_t *candev, const struct can_filter *filter)
{
    can_t *dev = container_of(candev, can_t, candev);

    int16_t idx = 0;
    bool _filter_exists = false;
    if (filter->can_id & CAN_EFF_FLAG) {
        DEBUG_PUTS("Extended filter to add in the extended filter section of the message RAM");
        /* Check if the filter already exists */
        _filter_exists = _find_filter(dev, filter, false, &idx);
        if (_filter_exists) {
            DEBUG_PUTS("Extended filter already exists --> Update it");
        }
        else {
            /* Find a free slot where to save the filter */
            /* Use static function instead */
            for (;(uint16_t)idx < ARRAY_SIZE(dev->msg_ram_conf.ext_filter); idx++) {
                if (dev->msg_ram_conf.ext_filter[idx].F0.efec == CAN_FILTER_DISABLE) {
                    DEBUG_PUTS("empty slot");
                    break;
                }
            }
        }

        if (idx == ARRAY_SIZE(dev->msg_ram_conf.ext_filter)) {
            DEBUG_PUTS("Reached maximum capacity of extended filters --> Could not add filter");
            return -1;
        }

        DEBUG("Filter to add at idx = %d\n", idx);
        dev->msg_ram_conf.ext_filter[idx].F0.efid1 = filter->can_id;
        dev->msg_ram_conf.ext_filter[idx].F0.efec = filter->can_filter_conf;
        dev->msg_ram_conf.ext_filter[idx].F1.efid2 = filter->can_mask & CAN_EFF_MASK;
        dev->msg_ram_conf.ext_filter[idx].F1.eft = filter->can_filter_type & CAN_EFF_MASK;

        for (uint8_t i = 0; i < ARRAY_SIZE(dev->msg_ram_conf.ext_filter); i++) {
            DEBUG("can->msg_ram_conf.std_filter[%u] = 0x%08lx, filter conf = %u\n", i,
                    (uint32_t)(dev->msg_ram_conf.ext_filter[i].F0.efid1), dev->msg_ram_conf.ext_filter[i].F0.efec);
        }
    }
    else {
        DEBUG_PUTS("Standard filter to add in the standard filter section of the message RAM");
        /* Check if the filter already exists */
        _filter_exists = _find_filter(dev, filter, true, &idx);
        if (_filter_exists) {
            DEBUG_PUTS("Standard filter already exists --> Update it");
        }
        else {
            /* Find a free slot where to save the filter */
            /* Use static function instead */
            for (; (uint16_t)idx < ARRAY_SIZE(dev->msg_ram_conf.std_filter); idx++) {
                /* Find a free slot where to save the filter */
                if (dev->msg_ram_conf.std_filter[idx].sfec == CAN_FILTER_DISABLE) {
                    DEBUG_PUTS("empty slot");
                    break;
                }
            }
        }

        if (idx == ARRAY_SIZE(dev->msg_ram_conf.std_filter)) {
            DEBUG_PUTS("Reached maximum capacity of standard filters --> Could not add filter");
            return -1;
        }

        DEBUG("Filter to add at idx = %d\n", idx);
        dev->msg_ram_conf.std_filter[idx].sfec = filter->can_filter_conf;
        dev->msg_ram_conf.std_filter[idx].sft = filter->can_filter_type;
        dev->msg_ram_conf.std_filter[idx].sfid2 = filter->can_mask & CAN_SFF_MASK;
        dev->msg_ram_conf.std_filter[idx].sfid1 = filter->can_id & CAN_SFF_MASK;

        for (uint8_t i = 0; i < ARRAY_SIZE(dev->msg_ram_conf.std_filter); i++) {
            DEBUG("can->msg_ram_conf.std_filter[%u] = 0x%08lx, filter conf = %u\n", i,
                    (uint32_t)(dev->msg_ram_conf.std_filter[i].sfid1), dev->msg_ram_conf.std_filter[i].sfec);
        }
    }

    return idx;
}

static int _remove_filter(candev_t *candev, const struct can_filter *filter)
{
    can_t *dev = container_of(candev, can_t, candev);

    int16_t idx = 0;
    bool _filter_exists = false;
    if (filter->can_id & CAN_EFF_FLAG) {
        _filter_exists = _find_filter(dev, filter, false, &idx);
        if (_filter_exists) {
            DEBUG("Extended filter to disable at idx = %d\n", idx);
            dev->msg_ram_conf.ext_filter[idx].F0.efec = CAN_FILTER_DISABLE;

            for (uint8_t i = 0; i < ARRAY_SIZE(dev->msg_ram_conf.ext_filter); i++) {
                DEBUG("can->msg_ram_conf.std_filter[%u] = 0x%08lx, filter conf = %u\n", i,
                        (uint32_t)(dev->msg_ram_conf.ext_filter[i].F0.efid1), dev->msg_ram_conf.ext_filter[i].F0.efec);
            }
        }
        else {
            DEBUG_PUTS("Filter not found");
            return -1;
        }
    }
    else {
        _filter_exists = _find_filter(dev, filter, true, &idx);
        if(_filter_exists) {
            DEBUG("Standard filter to disable at idx = %d\n", idx);
            dev->msg_ram_conf.std_filter[idx].sfec = CAN_FILTER_DISABLE;

            for (uint8_t i = 0; i < ARRAY_SIZE(dev->msg_ram_conf.std_filter); i++) {
                DEBUG("can->msg_ram_conf.std_filter[%u] = 0x%08lx, filter conf = %u\n", i,
                        (uint32_t)(dev->msg_ram_conf.std_filter[i].sfid1), dev->msg_ram_conf.std_filter[i].sfec);
            }
        }
        else {
            DEBUG_PUTS("Filter not found");
            return -1;
        }
    }

    return idx;
}

static void _isr(candev_t *candev)
{
    can_t *dev = container_of(candev, can_t, candev);

    uint32_t irq_reg = dev->conf->can->IR.reg;
    DEBUG("IR = 0x%08lx\n", irq_reg);
    if (irq_reg & CAN_IR_TSW) {
        DEBUG_PUTS("Timestamp wraparound interrupt");
        /* Clear the interrupt source flag */
        dev->conf->can->IR.reg |= CAN_IR_TSW;
    }

    if (irq_reg & CAN_IR_RF0N) {
        DEBUG_PUTS("New message in Rx FIFO 0");
        /* Clear the interrupt source flag */
        dev->conf->can->IR.reg |= CAN_IR_RF0N;

        uint16_t rx_get_idx = 0;
        uint16_t rx_put_idx = 0;
        rx_get_idx = (dev->conf->can->RXF0S.reg & 0x00003F00) >> 8;
        DEBUG("rx get index = %u\n", rx_get_idx);
        rx_put_idx = (dev->conf->can->RXF0S.reg & 0x003F0000) >> 16;
        DEBUG("rx put index = %u\n", rx_put_idx);

        struct can_frame frame_received = {0};
        if (!dev->msg_ram_conf.rx_fifo_0[rx_get_idx].R0.xtd) {
            DEBUG_PUTS("Received standard CAN frame");
            frame_received.can_id = dev->msg_ram_conf.rx_fifo_0[rx_get_idx].R0.id >> 18;
        }
        else {
            DEBUG_PUTS("Received extended CAN frame");
            frame_received.can_id = dev->msg_ram_conf.rx_fifo_0[rx_get_idx].R0.id;
        }
        frame_received.can_dlc = dev->msg_ram_conf.rx_fifo_0[rx_get_idx].R1.dlc;
        memcpy(frame_received.data, dev->msg_ram_conf.rx_fifo_0[rx_get_idx].data.data_8, frame_received.can_dlc);

        dev->conf->can->RXF0A.reg = CAN_RXF0A_F0AI(rx_get_idx);
        if (dev->candev.event_callback) {
            dev->candev.event_callback(&(dev->candev), CANDEV_EVENT_RX_INDICATION, &frame_received);
        }
    }

    if (irq_reg & CAN_IR_RF1N) {
        DEBUG_PUTS("New message in Rx FIFO 1");
        /* Clear the interrupt source flag */
        dev->conf->can->IR.reg |= CAN_IR_RF1N;

        uint16_t rx_get_idx = 0;
        uint16_t rx_put_idx = 0;
        rx_get_idx = (dev->conf->can->RXF1S.reg & 0x00003F00) >> 8;
        DEBUG("rx get index = %u\n", rx_get_idx);
        rx_put_idx = (dev->conf->can->RXF1S.reg & 0x003F0000) >> 16;
        DEBUG("rx put index = %u\n", rx_put_idx);

        struct can_frame frame_received = {0};
        if (!dev->msg_ram_conf.rx_fifo_1[rx_get_idx].R0.xtd) {
            DEBUG_PUTS("Received standard CAN frame");
            frame_received.can_id = dev->msg_ram_conf.rx_fifo_1[rx_get_idx].R0.id >> 18;
        }
        else {
            DEBUG_PUTS("Received extended CAN frame");
            frame_received.can_id = dev->msg_ram_conf.rx_fifo_1[rx_get_idx].R0.id;
        }
        frame_received.can_dlc = dev->msg_ram_conf.rx_fifo_1[rx_get_idx].R1.dlc;
        memcpy(frame_received.data, dev->msg_ram_conf.rx_fifo_1[rx_get_idx].data.data_8, frame_received.can_dlc);

        dev->conf->can->RXF1A.reg = CAN_RXF1A_F1AI(rx_get_idx);
        if (dev->candev.event_callback) {
            dev->candev.event_callback(&(dev->candev), CANDEV_EVENT_RX_INDICATION, &frame_received);
        }
    }
    if (irq_reg & CAN_IR_TEFN) {
        DEBUG_PUTS("New Tx event FIFO entry");
        dev->conf->can->IR.reg |= CAN_IR_TEFN;
        DEBUG("IR after clear = 0x%08lx\n", dev->conf->can->IR.reg);
        if (dev->candev.event_callback) {
            dev->candev.event_callback(&(dev->candev), CANDEV_EVENT_TX_CONFIRMATION, NULL);
        }
    }
}

void ISR_CAN1(void)
{
    DEBUG_PUTS("ISR CAN1");

    if (_can->candev.event_callback) {
        _can->candev.event_callback(&(_can->candev), CANDEV_EVENT_ISR, NULL);
    }

}

void ISR_CAN0(void)
{
    DEBUG_PUTS("ISR CAN0");

    if (_can->candev.event_callback) {
        _can->candev.event_callback(&(_can->candev), CANDEV_EVENT_ISR, NULL);
    }
}