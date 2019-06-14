#include "eeprom_spi.h"
#include "eeprom_spi_params.h"
#include "eeprom_cmd.h"

#define BUS (dev->params.spi)

typedef uint32_t eeprom_addr_t;

typedef struct eeprom_data_t 
{
    uint8_t byte[256]; 
} eeprom_data_t;

void eeprom_spi_init(void)
{
    /* EEPROM pins init */                                                                                                             
    gpio_init(EEPROM_WP_PIN, GPIO_OUT);                                                                                                
    gpio_init(EEPROM_HOLD_PIN, GPIO_OUT);                                                                                              
    gpio_init(EEPROM_SPI_CS_PIN, GPIO_OUT);
    
    SPI_CS_HIGH;
    EEPROM_WP_HIGH;
    EEPROM_HOLD_HIGH;
}

uint8_t test_eeprom(void)
{
    uint8_t spibuf = WREN;                                                                                                                      
    spi_acquire(BUS, SPI_CS_UNDEF, EEPROM_PARAM_SPI_MODE, EEPROM_PARAM_SPI_CLK);                                                        
    SPI_CS_LOW;                                                                                                                         
    spi_transfer_byte(BUS, SPI_CS_UNDEF, true, spibuf);                                                                    
    SPI_CS_HIGH;
    xtimer_usleep(1);                                                                                                                   
    spibuf = RDSR;
    SPI_CS_LOW;
    uint8_t status = spi_transfer_bytes(BUS, SPI_CS_UNDEF, true, spibuf);
    SPI_CS_HIGH;
    spi_release(BUS);                                                                                                                     
    
    if (status == 0b10)                                                                                                              
    {                                                                                                                                       
        return 1;                                                                                                     
    }                                                                                                                                       
    else if (status != 0)  
    {                                                                                                                                       
        return 2;                                                                                   
    }                                                                                                                                      
    else
    {                                                                                                                                           
        return 0;                                                                                        
    }                 
    spi_release(BUS);  
}

void read_eeprom(eeprom_addr_t addr, eeprom_data_t* rx_array)
{
    uint8_t spibuf = READ;
    spi_acquire(BUS, SPI_CS_UNDEF, EEPROM_PARAM_SPI_MODE, EEPROM_PARAM_SPI_CLK);
    SPI_CS_LOW;
    spi_transfer_bytes(BUS, SPI_CS_UNDEF, true, &spibuf, rx_array, 1);
    SPI_CS_HIGH;
    spi_release(BUS);
}

uint8_t write_eeprom(eeprom_addr_t addr, eeprom_data_t* tx_array, size_t lenght)
{
    if (test_eeprom() != 1)
    {
        return 0;
    }

    uint8_t addr_array[3];
    addr_array[0] = (addr >> 16);
    addr_array[1] = (addr >> 8);
    addr_array[2] = addr;
    write_cmd = WRITE;

    spi_acquire(BUS, SPI_CS_UNDEF, EEPROM_PARAM_SPI_MODE, EEPROM_PARAM_SPI_CLK);
    SPI_CS_LOW;
    spi_transfer_bytes(BUS, SPI_CS_UNDEF, true, NULL, write_cmd, 1);
    spi_transfer_bytes(BUS, SPI_CS_UNDEF, true, NULL, addr_array, 3);
    spi_transfer_bytes(BUS, SPI_CS_UNDEF, true, NULL, tx_array, lenght);
    SPI_CS_HIGH;
    spi_release(BUS);
    return 1;
}
