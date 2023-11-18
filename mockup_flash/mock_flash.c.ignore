#include "flash_driver.h"

#include "string.h"

#define MOCK_FLASH_BUFF_SIZE 16000000

uint8_t mock_flash_buff[MOCK_FLASH_BUFF_SIZE];

void FLASH_clear_page();

void FLASH_page_program(uint8_t* bytes, uint16_t num_bytes, uint32_t addr) {
    int page_start_addr = addr / 256;
    int offset_from_page_start = addr % 256;
    // assert(offset_from_page_start == 0);
    if(num_bytes < 256) {
        memcpy(mock_flash_buff + addr, bytes, num_bytes - offset_from_page_start);
        memcpy(mock_flash_buff + page_start_addr, bytes, offset_from_page_start);
    }
    memcpy(mock_flash_buff + addr, bytes, num_bytes - offset_from_page_start);
    memcpy(mock_flash_buff + page_start_addr, bytes, offset_from_page_start);
}

void FLASH_read_data(uint8_t* bytes, uint32_t num_bytes, uint32_t addr) {
    memcpy(bytes, mock_flash_buff + addr, num_bytes);
}