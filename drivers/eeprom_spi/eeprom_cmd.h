#ifdef __cplusplus
extern "C" {
#endif

#define WREN                  0b00000110 //Write Enable
#define WRDI                  0b00000100 //Write Disable
#define RDSR                  0b00000101 //Read Status Register
#define WRSR                  0b00000001 //Write Status Register
#define READ                  0b00000011 //Read from Memory Array
#define WRITE                 0b00000010 //Write to Memory Array
//For M95M01-D only:
#define RDID                  0b10000011 //Read Identification Page
#define WRID                  0b10000010 //Write Identification Page
#define RDLS                  0b10000011 //Reads the Identification Page lock status
#define LID                   0b10000010 //Locks the Identification Page in read-only mode

#ifdef __cplusplus                                                                                                                     
}                                                                                                                                  
#endif
