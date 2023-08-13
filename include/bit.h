/* Byte formatting utilities.
 */

#ifndef BIT_UTILS_H
#define BIT_UTILS_H

#include <stdint.h>
#include <stdbool.h>

// Convert little-endian sequence of bytes to specified types

char byte_to_char(uint8_t byte);

int16_t bytes_to_int16_LE(uint8_t *buffer);
int32_t bytes_to_int32_LE(uint8_t *buffer);
int64_t bytes_to_int64_LE(uint8_t *buffer);

uint16_t bytes_to_uint16_LE(uint8_t *buffer);
uint32_t bytes_to_uint32_LE(uint8_t *buffer);
uint64_t bytes_to_uint64_LE(uint8_t *buffer);

float bytes_to_float32_LE(uint8_t *buffer);
double bytes_to_double64_LE(uint8_t *buffer);

bool bytes_to_bool32_LE(uint8_t *buffer);

#endif  // BIT_UTILS_H
