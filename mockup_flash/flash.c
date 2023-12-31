

#include "flash.h"

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>

#define MOCK_EVENT_LOG_BUFF_SIZE (MOCK_EVENT_LOG_COUNT * 8)
#define MOCK_EXP_LOG_BUFF_SIZE (MOCK_EXP_LOG_COUNT * 8)

#define EVENT_LOG_START sizeof(struct FlashHeader)

#define EXP_LOG_1_START EVENT_LOG_START + MOCK_EVENT_LOG_BUFF_SIZE
#define EXP_LOG_1_END EXP_LOG_1_START + MOCK_EXP_LOG_BUFF_SIZE

#define EXP_LOG_2_START EXP_LOG_1_START + MOCK_EXP_LOG_BUFF_SIZE
#define EXP_LOG_2_END EXP_LOG_2_START + MOCK_EXP_LOG_BUFF_SIZE

static_assert(EVENT_LOG_START % 256 == 0, "");


// How many bytes in a single block when retrieved for ECC
// Currently assumes one block for ECC = one page of FLASH
#define ECC_BLOCK_SIZE 256

// A global variable to be used as our mock flash
uint8_t mock_flash_buff[sizeof(struct FlashHeader) + (MOCK_EVENT_LOG_BUFF_SIZE) + (MOCK_EXP_LOG_BUFF_SIZE * 5)];

// A global struct representing our flash header
const struct FlashHeader default_flash_header = {
    .events_header = {
        .start_addr = EVENT_LOG_START,
        .end_addr = EVENT_LOG_START + MOCK_EVENT_LOG_BUFF_SIZE,
        .tail = EVENT_LOG_START,
        .oldest_block_addr = EVENT_LOG_START
    },
    .exp1_header = {
        .start_addr = EXP_LOG_1_START,
        .end_addr = EXP_LOG_1_END,
        .tail = EXP_LOG_1_START,
        .oldest_block_addr = EXP_LOG_1_START
    },
    .exp2_header = {
        .start_addr = EXP_LOG_2_START,
        .end_addr = EXP_LOG_2_END,
        .tail = EXP_LOG_2_START,
        .oldest_block_addr = EXP_LOG_2_START
    },
    .current_exp_num = 1
};

struct FlashHeader flash_header;

void init_flash_header() {
    memcpy(mock_flash_buff, &default_flash_header, sizeof(default_flash_header));
    memcpy(&flash_header, &default_flash_header, sizeof(default_flash_header));
}

// TODO: could write these to match flash interface by just writing two:
//  - arbitrary byte read - takes a single address
//  - 256 byte "page program" - takes buffer up to 256 bytes, and handles overflow on page and stuff
//      - Could have a bulk write function that queues larger transfers. Might not be worth if no real gains
// void write_to_flash(void * buff, uint8_t size, uint32_t addr) {
//     uint32_t offset_from_256 = (addr % 256);
//     uint32_t cutoff;
//     if(size < )
//     memcpy(mock_flash_buff + addr, buff, size)

// }

// void read_from_flash(void * buff, uint32_t size);

void fetch_flash_header() {
    memcpy(&flash_header, mock_flash_buff, sizeof(struct FlashHeader));
}

void update_flash_header() {
    memcpy(mock_flash_buff, &flash_header, sizeof(struct FlashHeader));
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

static void advance_oldest_event_block() {
    flash_header.events_header.oldest_block_addr = advance_addr(
        flash_header.events_header.start_addr,
        flash_header.events_header.end_addr,
        flash_header.events_header.oldest_block_addr,
        ECC_BLOCK_SIZE
    );
}

static void advance_oldest_exp_block(struct ExperimentLogHeader * exp_log_header) {
    exp_log_header->oldest_block_addr = advance_addr(
        exp_log_header->start_addr,
        exp_log_header->end_addr,
        exp_log_header->oldest_block_addr,
        ECC_BLOCK_SIZE
    );
}

enum LogType get_oldest_page(uint8_t page_buff[ECC_BLOCK_SIZE], uint32_t * block_addr) {
    static enum LogType oldest_block_type = EVENT;
    uint32_t oldest_block_addr;
    switch(oldest_block_type) {
        case EVENT: {
            oldest_block_addr = flash_header.events_header.oldest_block_addr;
            advance_oldest_event_block();
        } break;

        case EXP1: {
            oldest_block_addr = flash_header.exp1_header.oldest_block_addr;
            advance_oldest_exp_block(&flash_header.exp1_header);

        } break;

        case EXP2: {
            oldest_block_addr = flash_header.exp2_header.oldest_block_addr;
            advance_oldest_exp_block(&flash_header.exp2_header);
        } break;
    }

    update_flash_header();

    memcpy(page_buff, mock_flash_buff + oldest_block_addr, ECC_BLOCK_SIZE);
    // memcpy(page_buff, mock_flash_buff + oldest_block_addr, ECC_BLOCK_SIZE);

    enum LogType block_type = oldest_block_type;

    oldest_block_type++;
    if(oldest_block_type > EXP2) oldest_block_type = EVENT;

    *block_addr = oldest_block_addr;
    return block_type;
}

/**
If the local buffer is longer than remaining space in mock flash buffer, this will write too much.

Potential solutions:
    1. Make the size of block a multiple of local buffer size, and only transfer when local buffer is full
    2. Check if size of block being transfered > remaining space in buffer and wrap around to start.
*/
uint8_t push_event_logs_to_flash(struct LocalEventLogs * local_event_logs) {
    memcpy(mock_flash_buff + flash_header.events_header.tail, local_event_logs->logs, sizeof(local_event_logs->logs));
    flash_header.events_header.tail = advance_addr(
        flash_header.events_header.start_addr,
        flash_header.events_header.end_addr,
        flash_header.events_header.tail,
        sizeof(local_event_logs->logs)
    );

    return 0;
}

uint8_t push_exp_logs_to_flash(struct LocalExpLogs * local_exp_logs, struct ExperimentLogHeader * current_exp_header) {
    memcpy(mock_flash_buff + current_exp_header->tail, local_exp_logs->logs, sizeof(local_exp_logs->logs));
    current_exp_header->tail = advance_addr(
        current_exp_header->start_addr,
        current_exp_header->end_addr,
        current_exp_header->tail,
        sizeof(local_exp_logs->logs)
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

uint16_t get_TLE_backup() {
    return flash_header.backup_tle_addr;
}


int main() {
    // Check that all regions are on start of 256 byte boundaries
    assert(default_flash_header.events_header.start_addr % 256 == 0);
    assert(default_flash_header.exp1_header.start_addr % 256 == 0);
    assert(default_flash_header.exp2_header.start_addr % 256 == 0);

    init_flash_header();
    fetch_flash_header();

    // Checking that flash header was properly initialized to default
    assert(memcmp(&flash_header, &default_flash_header, sizeof(default_flash_header)) == 0);

    uint8_t page_buff[256];
    uint32_t block_addr;

    // Should return the events in the correct sequence
    assert(get_oldest_page(page_buff, &block_addr) == EVENT);
    assert(get_oldest_page(page_buff, &block_addr) == EXP1);
    assert(get_oldest_page(page_buff, &block_addr) == EXP2);
    // Should start over again from 
    assert(get_oldest_page(page_buff, &block_addr) == EVENT);
    assert(get_oldest_page(page_buff, &block_addr) == EVENT);

    // The mock buffer should match the flash header:
    assert(memcmp(&flash_header, mock_flash_buff, sizeof(flash_header)) == 0);
    
    // After calling get_oldest_page, the flash header should be different from the default
    assert(memcmp(&flash_header, &default_flash_header, sizeof(default_flash_header)) != 0);

    // The only change should be the oldest_block_addrs for each set of logs
    struct FlashHeader new_header = default_flash_header;
    // events header should have rolled back to start
    // new_header.events_header.oldest_block_addr;
    new_header.exp1_header.oldest_block_addr += ECC_BLOCK_SIZE;
    new_header.exp2_header.oldest_block_addr += ECC_BLOCK_SIZE;

    // Check that the new header matches the 
    assert(memcmp(&flash_header, &new_header, sizeof(flash_header)) == 0);
    assert(memcmp(&new_header, mock_flash_buff, sizeof(new_header)) == 0);
}
