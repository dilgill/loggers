

#include "flash_using_driver.h"
#include "flash_driver.h"

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>

#define MOCK_EVENT_LOG_PAGES 2
#define MOCK_EXP_LOG_PAGES 2

#define EVENT_LOG_START 1

#define EXP_LOG_1_START EVENT_LOG_START + MOCK_EVENT_LOG_PAGES
#define EXP_LOG_1_END EXP_LOG_1_START + MOCK_EXP_LOG_PAGES

#define EXP_LOG_2_START EXP_LOG_1_START + MOCK_EXP_LOG_PAGES
#define EXP_LOG_2_END EXP_LOG_2_START + MOCK_EXP_LOG_PAGES

// How many bytes in a single block when retrieved for ECC
// Currently assumes one block for ECC = one page of FLASH
#define ECC_BLOCK_SIZE 256

// A global variable to be used as our mock flash
// uint8_t mock_flash_buff[sizeof(struct FlashHeader) + (MOCK_EVENT_LOG_PAGES) + (MOCK_EXP_LOG_PAGES * 5)];

// A global struct representing our flash header
const struct FlashHeader default_flash_header = {
    .events_header = {
        .start_page_num = EVENT_LOG_START,
        .end_page_num = EVENT_LOG_START + MOCK_EVENT_LOG_PAGES,
        .tail = EVENT_LOG_START,
        .oldest_page_num = EVENT_LOG_START
    },
    .exp1_header = {
        .start_page_num = EXP_LOG_1_START,
        .end_page_num = EXP_LOG_1_END,
        .tail = EXP_LOG_1_START,
        .oldest_page_num = EXP_LOG_1_START
    },
    .exp2_header = {
        .start_page_num = EXP_LOG_2_START,
        .end_page_num = EXP_LOG_2_END,
        .tail = EXP_LOG_2_START,
        .oldest_page_num = EXP_LOG_2_START
    },
    .current_exp_num = 1
};

struct FlashHeader flash_header;

void FLASH_init_header() {
    FLASH_page_program((uint8_t*) &default_flash_header, 256, 0);
    memcpy(&flash_header, &default_flash_header, sizeof(default_flash_header));
}

void FLASH_fetch_header() {
    // memcpy(&flash_header, mock_flash_buff, sizeof(struct FlashHeader));
    FLASH_read_page((uint8_t*) &flash_header, 256, 0);
}

void FLASH_update_header() {
    // memcpy(mock_flash_buff, &flash_header, sizeof(struct FlashHeader));
    FLASH_page_program((uint8_t*) &flash_header, 256, 0);
}

static uint32_t advance_addr(
    const uint32_t start,
    const uint32_t end,
    const uint32_t addr_to_advance,
    const uint32_t increment
) {
    uint32_t offset_from_start = addr_to_advance - start;
    return start + (offset_from_start + increment) % (end - start);
}

static void FLASH_advance_oldest_event_block() {
    flash_header.events_header.oldest_page_num = advance_addr(
        flash_header.events_header.start_page_num,
        flash_header.events_header.end_page_num,
        flash_header.events_header.oldest_page_num,
        1
    );
}

static void FLASH_advance_oldest_exp_block(struct ExperimentLogHeader * exp_log_header) {
    exp_log_header->oldest_page_num = advance_addr(
        exp_log_header->start_page_num,
        exp_log_header->end_page_num,
        exp_log_header->oldest_page_num,
        1
    );
}

enum LogType FLASH_get_oldest_page(uint8_t page_buff[ECC_BLOCK_SIZE]) {
    static enum LogType oldest_block_type = EVENT;
    uint32_t oldest_page_num;
    switch(oldest_block_type) {
        case EVENT: {
            oldest_page_num = flash_header.events_header.oldest_page_num;
            FLASH_advance_oldest_event_block();
        } break;

        case EXP1: {
            oldest_page_num = flash_header.exp1_header.oldest_page_num;
            FLASH_advance_oldest_exp_block(&flash_header.exp1_header);

        } break;

        case EXP2: {
            oldest_page_num = flash_header.exp2_header.oldest_page_num;
            FLASH_advance_oldest_exp_block(&flash_header.exp2_header);
        } break;
    }

    FLASH_update_header();

    // memcpy(page_buff, mock_flash_buff + oldest_page_num, ECC_BLOCK_SIZE);
    FLASH_read_page(page_buff, 256, oldest_page_num);

    enum LogType block_type = oldest_block_type;

    oldest_block_type++;
    if(oldest_block_type > EXP2) oldest_block_type = EVENT;

    return block_type;
}

/**
If the local buffer is longer than remaining space in mock flash buffer, this will write too much.

Potential solutions:
    1. Make the size of block a multiple of local buffer size, and only transfer when local buffer is full
    2. Check if size of block being transfered > remaining space in buffer and wrap around to start.
*/
uint8_t FLASH_push_event_logs_to_flash(/* struct LocalEventLogs * local_event_logs*/) {
    // memcpy(mock_flash_buff + flash_header.events_header.tail, local_event_logs->logs, sizeof(local_event_logs->logs));
    flash_header.events_header.tail = advance_addr(
        flash_header.events_header.start_page_num,
        flash_header.events_header.end_page_num,
        flash_header.events_header.tail,
        1
    );

    return 0;
}

uint8_t FLASH_push_exp_logs_to_flash(struct LocalExpLogs * local_exp_logs, struct ExperimentLogHeader * current_exp_header) {
    // memcpy(mock_flash_buff + current_exp_header->tail, local_exp_logs->logs, sizeof(local_exp_logs->logs));
    FLASH_page_program((uint8_t*) local_exp_logs->logs, 256, current_exp_header->tail);
    printf("Pushed exp logs to page: %d", current_exp_header->tail);
    current_exp_header->tail = advance_addr(
        current_exp_header->start_page_num,
        current_exp_header->end_page_num,
        current_exp_header->tail,
        1
    ); 

    return 0;
}

// void update_TLE_backup(uint16_t TLE_backup) {

// }

void print_exp(uint8_t exp_log[sizeof(struct ExperimentLog)]) {
    struct ExperimentLog exp;
    memcpy(&exp, exp_log, sizeof(struct ExperimentLog));

    printf("exp_log:\n\trtc_time: %u\n\t gx: %u,  gy: %u,  gz: %u\n\tdgx: %u, dgy: %u, dgz: %u\n\textra: %u",
        exp.rtc_time,
        exp.gyro_x,
        exp.gyro_y,
        exp.gyro_z,
        exp.dgyro_x,
        exp.dgyro_y,
        exp.dgyro_z,
        exp.extra
    );
}


// int main() {
//     // // Check that all regions are on start of 256 byte boundaries
//     // assert(default_flash_header.events_header.start_page_num % 256 == 0);
//     // assert(default_flash_header.exp1_header.start_page_num % 256 == 0);
//     // assert(default_flash_header.exp2_header.start_page_num % 256 == 0);
//
//     FLASH_init_header();
//     FLASH_fetch_header();
//
//     // Checking that flash header was properly initialized to default
//     assert(memcmp(&flash_header, &default_flash_header, sizeof(default_flash_header)) == 0);
//
//     uint8_t page_buff[256];
//
//     log_exp_overflow();
//
//     // Should return the events in the correct sequence
//     assert(FLASH_get_oldest_page(page_buff) == EVENT);
//     assert(FLASH_get_oldest_page(page_buff) == EXP1);
//     assert(FLASH_get_oldest_page(page_buff) == EXP2);
//     // Should start over again from
//     assert(FLASH_get_oldest_page(page_buff) == EVENT);
//     assert(FLASH_get_oldest_page(page_buff) == EXP1);
//
//     // The mock buffer should match the flash header:
//     // assert(memcmp(&flash_header, mock_flash_buff, sizeof(flash_header)) == 0);
//
//     // After calling get_oldest_page, the flash header should be different from the default
//     assert(memcmp(&flash_header, &default_flash_header, sizeof(default_flash_header)) != 0);
//
//     // The only change should be the oldest_block_addrs for each set of logs
//     struct FlashHeader new_header = default_flash_header;
//     // events header should have rolled back to start
//     // new_header.events_header.oldest_block_addr;
//     new_header.exp2_header.oldest_page_num += 1;
//
//     // Check that the new header matches the
//     assert(memcmp(&flash_header, &new_header, sizeof(flash_header)) == 0);
//     // assert(memcmp(&new_header, mock_flash_buff, sizeof(new_header)) == 0);
// }