#ifndef PPG_IBI_CONFIG_H
#define PPG_IBI_CONFIG_H

#include <stdint.h>

#define PPG_IBI_VERSION_MAJOR (0u)
#define PPG_IBI_VERSION_MINOR (2u)
#define PPG_IBI_VERSION_PATCH (0u)

#define PPG_IBI_CHANNEL_COUNT (4u)
#define PPG_IBI_SAMPLE_RATE_HZ (50u)
#define PPG_IBI_SAMPLE_INTERVAL_MS (20u)
#define PPG_IBI_TIMESTAMP_TOLERANCE_MS (2u)
#define PPG_IBI_ADC_BITS (24u)

#define PPG_IBI_RAM_BUDGET_BYTES (20u * 1024u)

#define PPG_IBI_SELECTED_CHANNEL_NONE (255u)

typedef struct
{
    uint16_t sample_rate_hz;
    uint16_t expected_sample_interval_ms;
    uint16_t timestamp_tolerance_ms;
    uint8_t channel_count;
} ppg_ibi_config_t;

#endif
