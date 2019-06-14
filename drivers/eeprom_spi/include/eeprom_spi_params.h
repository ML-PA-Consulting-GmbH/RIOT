#include "board.h"

#define SPI_CS_HIGH       gpio_set(EEPROM_SPI_CS_PIN)                                                                                  
#define SPI_CS_LOW        gpio_clear(EEPROM_SPI_CS_PIN) 
#define EEPROM_WP_HIGH    gpio_set(EEPROM_WP_PIN)                                                                                      
#define EEPROM_WP_LOW     gpio_clear(EEPROM_WP_PIN)                                                                                    
#define EEPROM_HOLD_HIGH  gpio_set(EEPROM_HOLD_PIN)                                                                                   
#define EEPROM_HOLD_LOW   gpio_clear(EEPROM_HOLD_PIN)   



//functions declaration
