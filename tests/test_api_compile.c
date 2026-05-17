#include "ppg_ibi.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SAMPLE_FIXTURE_PATH "tests/fixtures/sample_ppg.csv"

static int require_true(bool condition, const char *message)
{
    if (!condition)
    {
        (void)fprintf(stderr, "FAIL: %s\n", message);
        return 1;
    }

    return 0;
}

int main(void)
{
    ppg_ibi_config_t config;
    ppg_ibi_context_t ctx;
    ppg_ibi_sample_t sample = {0u, {0, 0, 0, 0}, true};
    ppg_ibi_event_t event;
    bool has_event = true;
    FILE *fixture;
    char header[96];
    int failures = 0;

    ppg_ibi_config_default(&config);

    failures += require_true(config.sample_rate_hz == PPG_IBI_SAMPLE_RATE_HZ,
                             "default sample rate is 50 Hz");
    failures += require_true(config.expected_sample_interval_ms ==
                                 PPG_IBI_SAMPLE_INTERVAL_MS,
                             "default timestamp step is 20 ms");
    failures += require_true(config.channel_count == PPG_IBI_CHANNEL_COUNT,
                             "default channel count is 4");
    failures += require_true(strcmp(ppg_ibi_version(), "0.2.0") == 0,
                             "version function links and reports M2 version");
    failures += require_true(sizeof(ppg_ibi_context_t) <
                                 PPG_IBI_RAM_BUDGET_BYTES,
                             "context fits RAM budget");
    failures += require_true(ppg_ibi_init(&ctx, &config) == PPG_IBI_STATUS_OK,
                             "init links and succeeds");
    failures += require_true(ppg_ibi_reset(&ctx) == PPG_IBI_STATUS_OK,
                             "reset links and succeeds");

    sample.timestamp_ms = 0u;
    sample.ppg[0] = 3338182;
    sample.ppg[1] = 3708818;
    sample.ppg[2] = 3536433;
    sample.ppg[3] = 3134694;
    sample.allow_measure = true;

    failures += require_true(ppg_ibi_process(&ctx, &sample, &event,
                                             &has_event) == PPG_IBI_STATUS_OK,
                             "process links and succeeds");
    failures += require_true(!has_event, "placeholder process emits no IBI");
    failures += require_true(event.timestamp_ms == sample.timestamp_ms,
                             "event timestamp field exists");
    failures += require_true(event.ibi_ms == 0u, "event ibi_ms field exists");
    failures += require_true(event.confidence == 0.0f,
                             "event confidence field exists");
    failures += require_true(event.signal_quality == 0.0f,
                             "event signal_quality field exists");
    failures += require_true(event.selected_channel ==
                                 PPG_IBI_SELECTED_CHANNEL_NONE,
                             "event selected_channel field exists");
    failures += require_true(event.state == PPG_IBI_STATE_ACQUIRE,
                             "event state field exists");
    failures += require_true(event.reject_reason == PPG_IBI_REJECT_NOT_READY,
                             "event reject_reason field exists");

    sample.timestamp_ms = 20u;
    sample.allow_measure = false;
    has_event = true;
    failures += require_true(ppg_ibi_process(&ctx, &sample, &event,
                                             &has_event) == PPG_IBI_STATUS_OK,
                             "process handles allow_measure false");
    failures += require_true(!has_event,
                             "allow_measure false emits no IBI event");
    failures += require_true(event.state == PPG_IBI_STATE_REACQUIRE,
                             "allow_measure false enters REACQUIRE");
    failures += require_true(event.reject_reason ==
                                 PPG_IBI_REJECT_MEASURE_NOT_ALLOWED,
                             "allow_measure false reject reason is reported");

    sample.timestamp_ms = 40u;
    sample.allow_measure = true;
    has_event = true;
    failures += require_true(ppg_ibi_process(&ctx, &sample, &event,
                                             &has_event) == PPG_IBI_STATUS_OK,
                             "process handles gate recovery");
    failures += require_true(!has_event, "gate recovery emits no IBI event");
    failures += require_true(event.state == PPG_IBI_STATE_REACQUIRE,
                             "gate recovery enters REACQUIRE");

    fixture = fopen(SAMPLE_FIXTURE_PATH, "r");
    failures += require_true(fixture != NULL,
                             "tests/fixtures/sample_ppg.csv is readable");
    if (fixture != NULL)
    {
        failures += require_true(fgets(header, sizeof(header), fixture) != NULL,
                                 "sample fixture header is readable");
        failures += require_true(strstr(header, "timestamp_ms") != NULL,
                                 "sample fixture timestamp field exists");
        failures += require_true(strstr(header, "PPG_G1") != NULL,
                                 "sample fixture PPG_G1 field exists");
        failures += require_true(strstr(header, "PPG_G4") != NULL,
                                 "sample fixture PPG_G4 field exists");
        (void)fclose(fixture);
    }

    return (failures == 0) ? 0 : 1;
}
