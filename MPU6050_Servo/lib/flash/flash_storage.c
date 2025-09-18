#include "flash_storage.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <string.h>

#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

typedef struct {
    uint32_t magic;
    uint32_t rotation_time_ms;
} calib_data_t;

#define MAGIC_KEY 0xABCD1234

bool flash_storage_read(uint32_t *rotation_time_ms) {
    const calib_data_t *data = (const calib_data_t *)(XIP_BASE + FLASH_TARGET_OFFSET);

    if (data->magic == MAGIC_KEY && data->rotation_time_ms >= 400 && data->rotation_time_ms <= 3000) {
        *rotation_time_ms = data->rotation_time_ms;
        return true;
    }
    return false;
}

void flash_storage_write(uint32_t rotation_time_ms) {
    calib_data_t data = {
        .magic = MAGIC_KEY,
        .rotation_time_ms = rotation_time_ms
    };

    uint8_t page_buf[FLASH_PAGE_SIZE] = {0};
    // gravamos apenas os primeiros bytes da página com nossa struct
    memcpy(page_buf, &data, sizeof(data));

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, page_buf, FLASH_PAGE_SIZE);
    restore_interrupts(ints);
}
