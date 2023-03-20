#ifndef CANDEV_SAMD5X_H
#define CANDEV_SAMD5X_H

#ifdef __cplusplus
extern "C" {
#endif

#include "can/candev.h"
#include "periph/gpio.h"

#ifndef CANDEV_SAMD5X_DEFAULT_BITRATE
/** Default bitrate */
#define CANDEV_SAMD5X_DEFAULT_BITRATE 500000U
#endif

#ifndef CANDEV_SAMD5X_DEFAULT_SPT
/** Default sampling-point */
#define CANDEV_SAMD5X_DEFAULT_SPT     875
#endif

#ifndef CANDEV_SAMD5X_DEFAULT_STD_FILTER_NUM
#define CANDEV_SAMD5X_DEFAULT_STD_FILTER_NUM	10
#endif

#ifndef CANDEV_SAMD5X_DEFAULT_EXT_FILTER_NUM
#define CANDEV_SAMD5X_DEFAULT_EXT_FILTER_NUM	3
#endif

#ifndef CANDEV_SAMD5X_DEFAULT_RX_FIFO_0_ELTS_NUM	
#define CANDEV_SAMD5X_DEFAULT_RX_FIFO_0_ELTS_NUM	3
#endif

#ifndef CANDEV_SAMD5X_DEFAULT_RX_FIFO_1_ELTS_NUM	
#define CANDEV_SAMD5X_DEFAULT_RX_FIFO_1_ELTS_NUM	3
#endif

#ifndef CANDEV_SAMD5X_DEFAULT_TX_EVT_FIFO_ELTS_NUM	
#define CANDEV_SAMD5X_DEFAULT_TX_EVT_FIFO_ELTS_NUM	3
#endif

#ifndef CANDEV_SAMD5X_DEFAULT_TX_BUFFER_NUM
#define CANDEV_SAMD5X_DEFAULT_TX_BUFFER_NUM			3
#endif

#ifndef CANDEV_SAMD5X_DEFAULT_TX_BUFFER_FIFO_QUEUE_NUM
#define CANDEV_SAMD5X_DEFAULT_TX_BUFFER_FIFO_QUEUE_NUM		20
#endif

/* unit: elements */
#define CANDEV_SAMD5X_MAX_STD_FILTER		128
#define CANDEV_SAMD5X_MAX_EXT_FILTER		64
#define CANDEV_SAMD5X_MAX_RX_FIFO_0_ELTS	64
#define CANDEV_SAMD5X_MAX_RX_FIFO_1_ELTS	64
#define CANDEV_SAMD5X_MAX_RX_BUFFER			64
#define CANDEV_SAMD5X_MAX_TX_EVT_FIFO_ELTS	32
#define CANDEV_SAMD5X_MAX_TX_BUFFER			32
#define CANDEV_SAMD5X_MSG_RAM_MAX_SIZE		446

typedef struct {
	Can *can;
	gpio_t rx_pin;
	gpio_t tx_pin;
	bool tdc_ctrl;	/** Enable/Disable Transceiver Delay Compensation */
	bool tx_fifo_queue_ctrl;	/** False to use Tx FIFO operation
									True to use Tx Queue operation */

} can_conf_t;
#define HAVE_CAN_CONF_T

typedef struct {
	uint8_t put:4;			/** Message Marker Put index */
	uint8_t get:4;			/** Message Marker Get index */
} can_mm_t;

typedef struct {
	struct {
		uint32_t id:29;		/** CAN Identifier */
		uint32_t rtr:1;		/** Remote Transmission Request */
		uint32_t xtd:1;		/** Extended Filter */
		uint32_t esi:1;		/** Error State Indicator */
	} T0;
	struct {
		uint32_t res:16;	/** reserved */
		uint32_t dlc:4;		/** Data Length Code */
		uint32_t brs:1;		/** Bit Rate Search */
		uint32_t fdf:1;		/** FD Format */
		uint32_t res_1:1;	/** reserved */
		uint32_t efc:1;		/** Event FIFO Control */
		can_mm_t mm; 		/** Message Marker */
	} T1 __attribute__((packed));
	union {
		uint32_t data_32[16];
		uint8_t data_8[64];
	} data;
} can_tx_buffer_t;

typedef struct {
	struct {
		uint32_t id:29;
		uint32_t rtr:1;
		uint32_t xtd:1;
		uint32_t esi:1;
	} E0;
	struct {
		uint32_t txts:16;
		uint32_t dlc:4;
		uint32_t brs:1;
		uint32_t fdf:1;
		uint32_t et:2;
		can_mm_t mm;
	} E1 __attribute__((packed));
} can_tx_event_fifo_t;

typedef struct {
	struct {
		uint32_t id:29;		/** CAN Identifier */
		uint32_t rtr:1;		/** Remote Transmission Request */
		uint32_t xtd:1;		/** Extended Filter */
		uint32_t esi:1;		/** Error State Indicator */
	} R0;
	struct {
		uint32_t rxts:16;	/** Rx timestamp */
		uint32_t dlc:4;		/** Data Length Code */
		uint32_t brs:1;		/** Bit Rate Search */
		uint32_t fdf:1;		/** FD Format */
		uint32_t res:2;		/** Reserved */
		uint32_t fidx:7;	/** Filter Index */
		uint32_t anmf:1;	/** Accepted Non-matching Frame */
	} R1;
	union {
		uint32_t data_32[16];
		uint8_t data_8[64];
	} data;
} can_rx_buffer_t;

typedef struct {
	uint32_t sfid2:11; 		/** Standard Filter ID 2 */
	uint32_t res:5;			/** reserved */
	uint32_t sfid1:11;		/** Standard Filer ID 1 */
	uint32_t sfec:3;		/** Standard Filter Element Configuration */
	uint32_t sft:2;			/** Standard Filter Type */	
} can_std_filter_t;

typedef struct {
	struct {
		uint32_t efid1:29;	/** Extended Filter ID 1 */
		uint32_t efec:3;	/** Extended Filter Element Configuration */
	} F0;
	struct {
		uint32_t efid2:29;	/** Extended Filter ID 2 */
		uint32_t res:1;		/** Reserved */
		uint32_t eft:2;		/** Extended Filter Type */
	} F1;
} can_ext_filter_t;

typedef struct {
	can_std_filter_t std_filter[CANDEV_SAMD5X_DEFAULT_STD_FILTER_NUM];
	can_ext_filter_t ext_filter[CANDEV_SAMD5X_DEFAULT_EXT_FILTER_NUM];
	can_rx_buffer_t rx_fifo_0[CANDEV_SAMD5X_DEFAULT_RX_FIFO_0_ELTS_NUM];
	can_rx_buffer_t rx_fifo_1[CANDEV_SAMD5X_DEFAULT_RX_FIFO_1_ELTS_NUM];
	can_rx_buffer_t rx_buffer[CANDEV_SAMD5X_MAX_RX_BUFFER];
	can_tx_event_fifo_t tx_event_fifo[CANDEV_SAMD5X_DEFAULT_TX_EVT_FIFO_ELTS_NUM];
	can_tx_buffer_t tx_fifo_queue[CANDEV_SAMD5X_DEFAULT_TX_BUFFER_FIFO_QUEUE_NUM];
} msg_ram_conf_t;

typedef struct {
	candev_t candev;
	const can_conf_t *conf;
	msg_ram_conf_t msg_ram_conf;
} can_t;
#define HAVE_CAN_T

void candev_samd5x_set_pins(can_t *dev);

void candev_samd5x_tdc_control(can_t *dev);

void candev_samd5x_enter_test_mode(candev_t *candev);

#endif