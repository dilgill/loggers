#include "experiment_logger.h"
#include "mockup_flash/flash_using_driver.h"
#include "fits_in_bits.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// Small buffer that represents log storage on the MCU
// #define LOCAL_EXP_LOG_BUFFER_SIZE (2 * 32)

// TODO: turn this into a pointer to one of the two local logging structs
// Use this pointer when needed
// swap what it points to on local log overflow
struct LocalExpLogs local_exp_logs = {
    .num_logs = LOCAL_EXP_LOG_COUNT,
    .tail = 0,
    .logs = {{0, 0, 0, 0, 0, 0, 0, 0}}
};

// static struct LocalExpLogs local_exp_logs_1 = {
//     .buffer_size = LOCAL_EXP_LOG_BUFFER_SIZE,
//     .tail = 0,
//     .buffer = {0}
// };

// static struct LocalExpLogs local_exp_logs_2 = {
//     .buffer_size = LOCAL_EXP_LOG_BUFFER_SIZE,
//     .tail = 0,
//     .buffer = {0}
// };

int is_exp_being_logged();

struct ExperimentLogHeader * current_exp_header;

// TODO: figure out what to set up when starting logging
void start_exp_logging() {
    int current_exp = flash_header.current_exp_num;

    if(current_exp > EXP2) {
        current_exp = EXP1;
    }

    switch(current_exp) {
        case EXP1: {
            current_exp_header = &flash_header.exp1_header;
            break;
        }
        case EXP2: {
            current_exp_header = &flash_header.exp2_header;
            break;
        }
    }
}

// TODO: Save experiment status to flash header
void stop_exp_logging(int experiment_status) {

    current_exp_header->exit_status = experiment_status;
    int current_exp = flash_header.current_exp_num + 1;

    if(current_exp > EXP2) {
        current_exp = 1;
    }
    
    flash_header.current_exp_num = current_exp;
    FLASH_update_header();
}

// TODO: swap out the local buffer used for local logging on overflow
void handle_exp_overflow() {
    printf("moving exp logs\n");
    FLASH_push_exp_logs_to_flash(&local_exp_logs, current_exp_header);

    // Hypothetically, may want to have two sets local logs.
    // One is actively logged to, other won is copied over to flash (by DMA)
    // if(local_exp_logs == &local_exp_logs_1) {
    //     local_exp_logs = local_exp_logs_2;
    // } else {
    //     local_exp_logs = local_exp_logs_1;
    // }
}

/**
 * Builds an experiment log from its arguments and passes them to add_exp_log()
 * Checks that the fields fit in the bitfields
 * Might just be useful for testing
 */
uint8_t build_and_add_exp_log(
    unsigned int rtc_time,     // Date+Hr+Min+Sec
    int16_t  gyro_x,
    int16_t  gyro_y,
    int16_t  gyro_z,
    int16_t  dgyro_x,
    int16_t  dgyro_y,
    int16_t  dgyro_z,
    unsigned int extra
) {

    if (   !fits_in_bits(rtc_time, 12)
        || !fits_in_bits(extra, 11)
    ) {
        // Gave too large a value somewhere :(
        return 1;
    }

    uint64_t current_log_index = local_exp_logs.tail;

	// Current overflow policy - Just overwrite oldest log
	// TODO: Put into separate function - detect_exp_buff_overflow()
	// Simplest is probably just pass the buffer and have it return next index to insert at?

	local_exp_logs.logs[current_log_index].rtc_time = rtc_time;
    local_exp_logs.logs[current_log_index].gyro_x = gyro_x;
    local_exp_logs.logs[current_log_index].gyro_y = gyro_y;
    local_exp_logs.logs[current_log_index].gyro_z = gyro_z;
    local_exp_logs.logs[current_log_index].dgyro_x = dgyro_x;
    local_exp_logs.logs[current_log_index].dgyro_y = dgyro_y;
    local_exp_logs.logs[current_log_index].dgyro_z = dgyro_z;
    local_exp_logs.logs[current_log_index].extra = extra;

    current_log_index++;
    
    if ( current_log_index >= local_exp_logs.num_logs ) {
        local_exp_logs.tail = 0;

        handle_exp_overflow();
    } else {
        local_exp_logs.tail = current_log_index;
    }
    return 0;
}

// Function for adding an experiment log using bitwise operations. This may be the way we have to move forward
/*
uint8_t build_exp_2(
    uint16_t rtc_time,     // Date+Hr+Min+Sec
    int16_t  gyro_x,
    int16_t  gyro_y,
    int16_t  gyro_z,
    int16_t  dgyro_x,
    int16_t  dgyro_y,
    int16_t  dgyro_z,
    uint32_t extra,
    struct LocalExpLogs * local_exp_logs
) {
    if (   !fits_in_bits(rtc_time, 12)
        || !fits_in_bits(extra, 20)
    ) {
        // Gave too large a value somewhere :(
        return 1;
    }

    uint64_t log[2] = {0, 0};

    log[0] |= ((uint64_t)rtc_time << 52);
    log[0] |= ((uint64_t)gyro_x << 36);
    log[0] |= ((uint64_t)gyro_y << 20);
    log[0] |= ((uint64_t)gyro_z << 4);
    log[0] |= ((uint64_t)dgyro_x >> 12);

    log[1] |= ((uint64_t)dgyro_y << 52);
    log[1] |= ((uint64_t)dgyro_z << 36);
    log[1] |= extra;


    add_exp_log(log, local_exp_logs);
    return 0;
}
*/

/*
// Turns a pair of uint64_t into a single experiment log with bitwise operations.
// Currently unused - using bitfields, unions, and memcpy.
struct UnpackedExpLog decode(uint64_t log_0, uint64_t log_1) {
    struct UnpackedExpLog unpacked = {
        .rtc_time = (log_0 >> 52) & MASK_12_BIT,
        .gyro_x   = (log_0 >> 36) & MASK_16_BIT,
        .gyro_y   = (log_0 >> 20) & MASK_16_BIT,
        .gyro_z   = (log_0 >> 4)  & MASK_16_BIT,
        .dgyro_x  = (log_0 & MASK_4_BIT) + ((log_1 >> 52) & MASK_12_BIT),
        .dgyro_y  = (log_1 >> 36) & MASK_16_BIT,
        .dgyro_z  = (log_1 >> 20) & MASK_16_BIT,
        .extra    = (log_1 & MASK_20_BIT)
    };

    return unpacked;
}
*/

uint8_t get_exp_log(uint64_t addr, struct ExperimentLog * retrieved_log) {
    if(addr > local_exp_logs.num_logs) {
        // printf("idx %lu fail\n", idx);
        return -1;
    } else {
        *retrieved_log = local_exp_logs.logs[addr];
        return 0;
    }
}

int detect_exp_buff_overflow();

void handle_exp_buff_overflow();

// Some testing of overflows and such
int main() {

     // Start Logging Experiment
     //  - get metadata:
     //      - exp log size
     //      - exp log current insert idx
     //      - exp log start addr?
     //  - store metadata in struct
     //  - store logs locally
     //  - upon buffer fill:
     //      - move local logs to flash (could offload to DMA???)
     //      - After transfer is complete, update flash's metadata ( mainly log head )
     //      - possibly swap buffer used for local logging in the meantime

	// get_flash_header(&flash_header);

    bool first_time_running = true;

    if(first_time_running) {
        FLASH_init_header();
        first_time_running = true;
    } else {
        FLASH_fetch_header();
    }
    
    start_exp_logging();

    // unsigned int curr_idx = 0;
    for (int i = 0; i < 112; ++i) {
        build_and_add_exp_log(i, i, i, i, i, i, i, i);
    }

    printf("Local Logs:\n");
    for(uint64_t i = 0; i < local_exp_logs.num_logs; ++i) {
        printf("L Exp, rtc_time: %u\n", local_exp_logs.logs[i].rtc_time);
    }

    uint8_t page_buff[256];
    enum LogType log_type = FLASH_get_oldest_page(page_buff);
    log_type = FLASH_get_oldest_page(page_buff);

    if(log_type == EXP1) {
        printf("oldest page is experiment 1\n");
        struct ExperimentLog* exp_logs = (struct ExperimentLog* ) page_buff;

        for(int i = 0; i < 16; ++i) {
            printf("exp log rtc: %d\n", exp_logs[i].rtc_time);
            assert(memcmp(&exp_logs[i], &local_exp_logs.logs[i], 16) == 0);
        }
    }

    return 0;
}