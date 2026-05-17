#ifndef PPG_IBI_H
#define PPG_IBI_H

#include <stdbool.h>
#include <stdint.h>

#include "ppg_ibi_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    PPG_IBI_STATUS_OK = 0,
    PPG_IBI_STATUS_ERROR_NULL = 1,
    PPG_IBI_STATUS_ERROR_CONFIG = 2
} ppg_ibi_status_t;

typedef enum
{
    PPG_IBI_STATE_INIT = 0,
    PPG_IBI_STATE_ACQUIRE = 1,
    PPG_IBI_STATE_TRACK = 2,
    PPG_IBI_STATE_REACQUIRE = 3,
    PPG_IBI_STATE_INVALID = 4
} ppg_ibi_state_t;

typedef enum
{
    PPG_IBI_REJECT_NONE = 0,
    PPG_IBI_REJECT_NOT_READY = 1,
    PPG_IBI_REJECT_MEASURE_NOT_ALLOWED = 2,
    PPG_IBI_REJECT_LOW_SIGNAL_QUALITY = 3,
    PPG_IBI_REJECT_TIMESTAMP_ERROR = 4,
    PPG_IBI_REJECT_IBI_OUT_OF_RANGE = 5,
    PPG_IBI_REJECT_CHANNEL_INVALID = 6,
    PPG_IBI_REJECT_STATE_INVALID = 7
} ppg_ibi_reject_reason_t;

typedef struct
{
    uint32_t timestamp_ms;
    int32_t ppg[PPG_IBI_CHANNEL_COUNT];
    bool allow_measure;
} ppg_ibi_sample_t;

typedef struct
{
    uint32_t timestamp_ms;
    uint32_t ibi_ms;
    float confidence;
    float signal_quality;
    uint8_t selected_channel;
    ppg_ibi_state_t state;
    ppg_ibi_reject_reason_t reject_reason;
} ppg_ibi_event_t;

typedef struct
{
    ppg_ibi_config_t config;
    ppg_ibi_state_t state;
    ppg_ibi_reject_reason_t reject_reason;
    uint32_t sample_count;
    uint32_t last_timestamp_ms;
    bool has_last_timestamp;
    bool measurement_was_allowed;
} ppg_ibi_context_t;

const char *ppg_ibi_version(void);

void ppg_ibi_config_default(ppg_ibi_config_t *config);

ppg_ibi_status_t ppg_ibi_init(ppg_ibi_context_t *ctx,
                              const ppg_ibi_config_t *config);

ppg_ibi_status_t ppg_ibi_reset(ppg_ibi_context_t *ctx);

ppg_ibi_status_t ppg_ibi_process(ppg_ibi_context_t *ctx,
                                 const ppg_ibi_sample_t *sample,
                                 ppg_ibi_event_t *event,
                                 bool *has_event);

#ifdef __cplusplus
}
#endif

#endif
