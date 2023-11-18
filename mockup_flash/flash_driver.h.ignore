#ifndef FLASH_DRIVER_H_
#define FLASH_DRIVER_H_
#include <stdint.h>

void FLASH_clear_page();

/**
Lets you write from 1-256 bytes of data
@bytes: The data to write. A buffer with size from 1-256 bytes
@num_bytes: The size of the `bytes` array
@addr: The 24-bit address of the page to program. Ideally should be aligned to 256 byte boundary???
*/
void FLASH_page_program(uint8_t* bytes, uint16_t num_bytes, uint32_t addr);

/**
Starting at `addr`, read `num_bytes` from the flash and copy into `bytes`.
@bytes: Buffer of bytes to copy to
@num_bytes: The number of bytes to read in
@addr: Address in flash to start reading from
*/
void FLASH_read_data(uint8_t* bytes, uint32_t num_bytes, uint32_t addr);


#endif