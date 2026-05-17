#include "ppg_ibi_internal.h"

#include <string.h>

static void ppg_ibi_clear_event(ppg_ibi_event_t *event)
{
    if (event != NULL)
    {
        (void)memset(event, 0, sizeof(*event));
        event->selected_channel = PPG_IBI_SELECTED_CHANNEL_NONE;
        event->state = PPG_IBI_STATE_INIT;
        event->reject_reason = PPG_IBI_REJECT_NOT_READY;
    }
}

static void ppg_ibi_reset_runtime(ppg_ibi_context_t *ctx)
{
    ctx->state = PPG_IBI_STATE_INIT;
    ctx->reject_reason = PPG_IBI_REJECT_NOT_READY;
    ctx->sample_count = 0u;
    ctx->last_timestamp_ms = 0u;
    ctx->has_last_timestamp = false;
    ctx->measurement_was_allowed = false;
}

const char *ppg_ibi_version(void)
{
    return "0.2.0";
}

void ppg_ibi_config_default(ppg_ibi_config_t *config)
{
    if (config != NULL)
    {
        config->sample_rate_hz = (uint16_t)PPG_IBI_SAMPLE_RATE_HZ;
        config->expected_sample_interval_ms =
            (uint16_t)PPG_IBI_SAMPLE_INTERVAL_MS;
        config->timestamp_tolerance_ms =
            (uint16_t)PPG_IBI_TIMESTAMP_TOLERANCE_MS;
        config->channel_count = (uint8_t)PPG_IBI_CHANNEL_COUNT;
    }
}

bool ppg_ibi_config_is_valid(const ppg_ibi_config_t *config)
{
    bool valid = false;

    if (config != NULL)
    {
        valid =
            (config->sample_rate_hz == (uint16_t)PPG_IBI_SAMPLE_RATE_HZ) &&
            (config->expected_sample_interval_ms ==
             (uint16_t)PPG_IBI_SAMPLE_INTERVAL_MS) &&
            (config->timestamp_tolerance_ms <=
             config->expected_sample_interval_ms) &&
            (config->channel_count == (uint8_t)PPG_IBI_CHANNEL_COUNT);
    }

    return valid;
}

ppg_ibi_status_t ppg_ibi_init(ppg_ibi_context_t *ctx,
                              const ppg_ibi_config_t *config)
{
    ppg_ibi_config_t local_config;

    if (ctx == NULL)
    {
        return PPG_IBI_STATUS_ERROR_NULL;
    }

    if (config == NULL)
    {
        ppg_ibi_config_default(&local_config);
        config = &local_config;
    }

    if (!ppg_ibi_config_is_valid(config))
    {
        ppg_ibi_reset_runtime(ctx);
        ctx->state = PPG_IBI_STATE_INVALID;
        ctx->reject_reason = PPG_IBI_REJECT_STATE_INVALID;
        return PPG_IBI_STATUS_ERROR_CONFIG;
    }

    ctx->config = *config;
    ppg_ibi_reset_runtime(ctx);

    return PPG_IBI_STATUS_OK;
}

ppg_ibi_status_t ppg_ibi_reset(ppg_ibi_context_t *ctx)
{
    if (ctx == NULL)
    {
        return PPG_IBI_STATUS_ERROR_NULL;
    }

    ppg_ibi_reset_runtime(ctx);
    return PPG_IBI_STATUS_OK;
}

ppg_ibi_status_t ppg_ibi_process(ppg_ibi_context_t *ctx,
                                 const ppg_ibi_sample_t *sample,
                                 ppg_ibi_event_t *event,
                                 bool *has_event)
{
    uint32_t delta_ms;
    bool timestamp_error;

    if ((ctx == NULL) || (sample == NULL) || (event == NULL) ||
        (has_event == NULL))
    {
        return PPG_IBI_STATUS_ERROR_NULL;
    }

    ppg_ibi_clear_event(event);
    *has_event = false;

    if (!ppg_ibi_config_is_valid(&ctx->config))
    {
        ctx->state = PPG_IBI_STATE_INVALID;
        ctx->reject_reason = PPG_IBI_REJECT_STATE_INVALID;
        event->state = ctx->state;
        event->reject_reason = ctx->reject_reason;
        return PPG_IBI_STATUS_ERROR_CONFIG;
    }

    ctx->sample_count++;

    if (!sample->allow_measure)
    {
        ctx->state = PPG_IBI_STATE_REACQUIRE;
        ctx->reject_reason = PPG_IBI_REJECT_MEASURE_NOT_ALLOWED;
        ctx->measurement_was_allowed = false;
        ctx->last_timestamp_ms = sample->timestamp_ms;
        ctx->has_last_timestamp = true;
        event->timestamp_ms = sample->timestamp_ms;
        event->state = ctx->state;
        event->reject_reason = ctx->reject_reason;
        return PPG_IBI_STATUS_OK;
    }

    timestamp_error = false;
    if (ctx->has_last_timestamp)
    {
        delta_ms = sample->timestamp_ms - ctx->last_timestamp_ms;
        if ((delta_ms <
             ((uint32_t)ctx->config.expected_sample_interval_ms -
              (uint32_t)ctx->config.timestamp_tolerance_ms)) ||
            (delta_ms >
             ((uint32_t)ctx->config.expected_sample_interval_ms +
              (uint32_t)ctx->config.timestamp_tolerance_ms)))
        {
            timestamp_error = true;
        }
    }

    ctx->last_timestamp_ms = sample->timestamp_ms;
    ctx->has_last_timestamp = true;

    if (timestamp_error)
    {
        ctx->state = PPG_IBI_STATE_REACQUIRE;
        ctx->reject_reason = PPG_IBI_REJECT_TIMESTAMP_ERROR;
        ctx->measurement_was_allowed = true;
        event->timestamp_ms = sample->timestamp_ms;
        event->state = ctx->state;
        event->reject_reason = ctx->reject_reason;
        return PPG_IBI_STATUS_OK;
    }

    if (ctx->state == PPG_IBI_STATE_INIT)
    {
        ctx->state = PPG_IBI_STATE_ACQUIRE;
    }
    else if (!ctx->measurement_was_allowed)
    {
        ctx->state = PPG_IBI_STATE_REACQUIRE;
    }
    else
    {
        ctx->state = PPG_IBI_STATE_ACQUIRE;
    }

    ctx->reject_reason = PPG_IBI_REJECT_NOT_READY;
    ctx->measurement_was_allowed = true;
    event->timestamp_ms = sample->timestamp_ms;
    event->state = ctx->state;
    event->reject_reason = ctx->reject_reason;

    return PPG_IBI_STATUS_OK;
}
