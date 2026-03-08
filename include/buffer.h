#pragma once

#include <stdint.h>

constexpr size_t BUFFER_CAPACITY = 10;
constexpr size_t PAYLOAD_SIZE = 128;

const char* buffer_peek(const char* payload);
bool buffer_push();
bool flash_buffer();
void buffer_pop();