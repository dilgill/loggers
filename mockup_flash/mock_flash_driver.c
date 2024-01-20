#include "flash_driver.h"

#include "string.h"

#define MOCK_FLASH_BUFF_SIZE 16000000

uint8_t mock_flash_buff[MOCK_FLASH_BUFF_SIZE];

void FLASH_clear_page();

void FLASH_page_program(uint8_t* bytes, uint16_t num_bytes, uint32_t page) {
    int page_start_addr = page * 256;
    // assert(offset_from_page_start == 0);
    memcpy(mock_flash_buff + page_start_addr, bytes, num_bytes);
}

void FLASH_read_page(uint8_t* bytes, uint32_t num_bytes, uint32_t page) {
    memcpy(bytes, mock_flash_buff + (page * 256), num_bytes);
}