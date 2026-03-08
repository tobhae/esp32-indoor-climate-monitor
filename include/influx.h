#pragma once

#include <stddef.h>

void build_influxdb_url();
bool build_influx_payload(char* buffer, size_t size, const ClimateData& data);
bool post_influxdb(const char* payload, size_t len);