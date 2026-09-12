/* Plus-only libretro state extension. Ordinary SNA files remain unchanged. */
#ifndef CAP32_PLUS_SNAPSHOT_H
#define CAP32_PLUS_SNAPSHOT_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define PLUS_SNAPSHOT_SIZE 40960u
bool plus_snapshot_save(uint8_t *data, size_t size);
bool plus_snapshot_valid(const uint8_t *data, size_t size);
bool plus_snapshot_load(const uint8_t *data, size_t size);
#endif
