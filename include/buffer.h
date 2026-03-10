#pragma once

#include <cstddef>

#include "sensor.h"

constexpr size_t BUFFER_CAPACITY = 64;
constexpr size_t PAYLOAD_SIZE = 96;

const char* buffer_peek();
void buffer_push(const ClimateSample &sample);
bool flush_buffer();
void buffer_pop();