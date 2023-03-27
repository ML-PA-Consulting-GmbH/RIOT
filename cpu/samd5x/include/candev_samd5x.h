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
#define CANDEV_SAMD5X_DEFAULT_STD_FILTER_NUM	3
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
#define CANDEV_SAMD5X_DEFAULT_TX_EVT_FIFO_ELTS_NUM	1
#endif

#ifndef CANDEV_SAMD5X_DEFAULT_TX_BUFFER_NUM
#define CANDEV_SAMD5X_DEFAULT_TX_BUFFER_NUM			3
#endif

#ifndef CANDEV_SAMD5X_DEFAULT_TX_BUFFER_FIFO_QUEUE_NUM
#define CANDEV_SAMD5X_DEFAULT_TX_BUFFER_FIFO_QUEUE_NUM		3
#endif

/* unit: elements */
#define CANDEV_SAMD5X_MAX_STD_FILTER		128
#define CANDEV_SAMD5X_MAX_EXT_FILTER		64
#define CANDEV_SAMD5X_MAX_RX_FIFO_0_ELTS	64
#define CANDEV_SAMD5X_MAX_RX_FIFO_1_ELTS	64
#define CANDEV_SAMD5X_MAX_RX_BUFFER			64
#define CANDEV_SAMD5X_MAX_TX_EVT_FIFO_ELTS	32
#define CANDEV_SAMD5X_MAX_TX_BUFFER			32
#define CANDEV_SAMD5X_MSG_RAM_MAX_SIZE		448

typedef enum {
	CAN_ACCEPT_RX_FIFO_0 = 0x00,
	CAN_ACCEPT_RX_FIFO_1,
	CAN_REJECT
} can_non_matching_filter_t;

typedef struct {
	Can *can;
	gpio_t rx_pin;
	gpio_t tx_pin;
	gpio_mux_t mux;
	bool tdc_ctrl;									/** Enable/Disable Transceiver Delay Compensation */
	bool dar_ctrl;									/** Enable/Disable Automatic Retransmission */
	bool tx_fifo_queue_ctrl;						/** False to use Tx FIFO operation
														True to use Tx Queue operation */
	can_non_matching_filter_t global_filter_cfg;	/** Configure how to treat the messages that do
														not match the CAN filters */
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
		uint32_t data_32[2];
		uint8_t data_8[8];
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
		uint32_t data_32[2];
		uint8_t data_8[8];
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

typedef enum {
	CAN_RF0N_IRQ = 0,		/**< Rx FIFO 0 new message interrupt */
	CAN_RF0W_IRQ,			/**< Rx FIFO 0 watermark reached interrupt */
	CAN_RF0F_IRQ,			/**< Rx FIFO 0 full interrupt */
	CAN_RF0L_IRQ,			/**< Rx FIFO 0 message lost interrupt */
	CAN_RF1N_IRQ,			/**< Rx FIFO 1 new message interrupt */
	CAN_RF1W_IRQ,			/**< Rx FIFO 1 watermark reached interrupt */
	CAN_RF1F_IRQ,			/**< Rx FIFO 1 full interrupt */
	CAN_RF1L_IRQ,			/**< Rx FIFO 1 message lost interrupt */
	CAN_HPM_IRQ,			/**< High priority message received interrupt */
	CAN_TC_IRQ,				/**< Timestamp completed interrupt */
	CAN_TCF_IRQ,			/**< Transmission cancellation finished interrupt */
	CAN_TFE_IRQ,			/**< Tx FIFO empty interrupt */
	CAN_TEFN_IRQ,			/**< Tx event FIFO new entry interrupt */
	CAN_TEFW_IRQ,			/**< Tx event FIFO watermark reached interrupt */
	CAN_TEFF_IRQ,			/**< Tx event FIFO full interrupt */
	CAN_TEFL_IRQ,			/**< Tx event FIFO element lost interrupt */
	CAN_TSW_IRQ,			/**< Timestamp wrap-around interrupt */
	CAN_MRAF_IRQ,			/**< Message RAM access failure interrupt */
	CAN_TOO_IRQ,			/**< Timeout occured interrupt */
	CAN_DRX_IRQ,			/**< Message stored to dedicated Rx buffer interrupt */
	CAN_BEC_IRQ,			/**< Bit error corrected interrupt */
	CAN_BEU_IRQ,			/**< Bit error uncorrected interrupt */
	CAN_ELO_IRQ,			/**< Error logginng overflow interrupt */
	CAN_EP_IRQ,				/**< Error passive interrupt */
	CAN_EW_IRQ,				/**< Error warning interrupt */
	CAN_BO_IRQ,				/**< Bus off interrupt */
	CAN_WDI_IRQ,			/**< Watchdog interrupt */
	CAN_PEA_IRQ,			/**< Protocol error in arbitration phase interrupt */
	CAN_PED_IRQ,			/**< Protocol error in data phase */
	CAN_ARA_IRQ				/**< Access to reserved address */
} can_irq_source_t;

typedef enum {
	CAN_IRQ_LINE_0 = 0,		/**< CAN interrupt line 0 */
	CAN_IRQ_LINE_1			/**< CAN interrupt line 1 */
} can_irq_line_t;

/* Interrupt service routine functions */
#define ISR_CAN0	isr_can0
#define ISR_CAN1	isr_can1

void candev_samd5x_set_pins(can_t *dev);

void candev_samd5x_tdc_control(can_t *dev);

void candev_samd5x_enter_test_mode(candev_t *candev);

int candev_samd5x_irq_init(candev_t *candev, can_irq_source_t irq_source, can_irq_line_t irq_line);

#endif