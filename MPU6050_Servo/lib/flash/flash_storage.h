#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Lê da flash o tempo de rotação calibrado (ms). Retorna true se ok.
 */
bool flash_storage_read(uint32_t *rotation_time_ms);

/**
 * @brief Grava na flash o tempo de rotação calibrado (ms).
 */
void flash_storage_write(uint32_t rotation_time_ms);

#endif
