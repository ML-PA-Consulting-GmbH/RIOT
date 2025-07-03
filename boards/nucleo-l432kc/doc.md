@defgroup    boards_nucleo-l432kc STM32 Nucleo-L432KC
@ingroup     boards_common_nucleo32
@brief       Support for the STM32 Nucleo-L432KC

## Overview

The Nucleo-L432KC is a board from ST's Nucleo family supporting ARM Cortex-M4
STM32L432KCU6 microcontroller with 64KiB of RAM and 256KiB of Flash.

## Pinout

@image html pinouts/nucleo-l432kc-and-more.svg "Pinout for the Nucleo-L432KC (from STM user manual, UM1956, https://www.st.com/resource/en/user_manual/um1956-stm32-nucleo32-boards-mb1180-stmicroelectronics.pdf, page 32)" width=25%

### MCU

| MCU        |    STM32L432KC      |
|:---------- |:------------------- |
| Family     | ARM Cortex-M4F      |
| Vendor     | ST Microelectronics |
| RAM        | 64KiB               |
| Flash      | 256KiB              |
| Frequency  | up to 80MHz         |
| FPU        | yes                 |
| Timers     | 11 (7x 16-bit, 1x 32, 1x Systick, 2x Watchdog) |
| ADC        | 1x 12-bit (10 channels) |
| UARTs      | 3 (two USARTs and one Low-Power UART) |
| SPIs       | 2                   |
| CANs       | 1                   |
| RTC        | 1                   |
| I2Cs       | 2                   |
| Vcc        | 1.71V - 3.6V        |
| Datasheet  | [Datasheet](https://www.st.com/resource/en/datasheet/stm32l432kc.pdf) |
| Reference Manual | [Reference Manual](https://www.st.com/resource/en/reference_manual/rm0394-stm32l41xxx42xxx43xxx44xxx45xxx46xxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)|
| Programming Manual | [Programming Manual](http://www.st.com/content/ccc/resource/technical/document/programming_manual/6c/3a/cb/e7/e4/ea/44/9b/DM00046982.pdf/files/DM00046982.pdf/jcr:content/translations/en.DM00046982.pdf) |
| Board Manual | [Board Manual](https://www.st.com/resource/en/user_manual/um1956-stm32-nucleo32-boards-mb1180-stmicroelectronics.pdf) |

## Flashing the Board Using ST-LINK Removable Media

On-board ST-LINK programmer provides via composite USB device removable media.
Copying the HEX file causes reprogramming of the board. This task
could be performed manually; however, the cpy2remed (copy to removable
media) PROGRAMMER script does this automatically. To program board in
this manner, use the command:
```
make BOARD=nucleo-l432kc PROGRAMMER=cpy2remed flash
```
@note This PROGRAMMER was tested using ST-LINK firmware 2.37.26. Firmware updates
      can be found on [this STM webpage](https://www.st.com/en/development-tools/stsw-link007.html).
