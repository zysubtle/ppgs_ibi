#include "ppg_ibi.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SAMPLE_FIXTURE_PATH "tests/fixtures/sample_ppg.csv"
#define EXPECTED_FIXTURE_ROWS (25001u)

static int require_true(bool condition, const char *message)
{
    if (!condition)
    {
        (void)fprintf(stderr, "FAIL: %s\n", message);
        return 1;
    }

    return 0;
}

static int init_default(ppg_ibi_context_t *ctx)
{
    ppg_ibi_config_t config;

    ppg_ibi_config_default(&config);
    return require_true(ppg_ibi_init(ctx, &config) == PPG_IBI_STATUS_OK,
                        "default init succeeds");
}

static ppg_ibi_status_t process_timestamp(ppg_ibi_context_t *ctx,
                                          uint32_t timestamp_ms,
                                          bool allow_measure,
                                          ppg_ibi_event_t *event,
                                          bool *has_event)
{
    ppg_ibi_sample_t sample = {0u, {3338182, 3708818, 3536433, 3134694},
                               true};

    sample.timestamp_ms = timestamp_ms;
    sample.allow_measure = allow_measure;

    return ppg_ibi_process(ctx, &sample, event, has_event);
}

static int require_valid_timestamp_step(ppg_ibi_context_t *ctx,
                                        uint32_t timestamp_ms)
{
    ppg_ibi_event_t event;
    bool has_event = true;
    int failures = 0;

    failures += require_true(process_timestamp(ctx, timestamp_ms, true, &event,
                                               &has_event) ==
                                 PPG_IBI_STATUS_OK,
                             "normal timestamp process succeeds");
    failures += require_true(!has_event, "normal timestamp emits no IBI");
    failures += require_true(event.reject_reason !=
                                 PPG_IBI_REJECT_TIMESTAMP_ERROR,
                             "normal timestamp has no timestamp reject");

    return failures;
}

static int test_normal_sequence(void)
{
    ppg_ibi_context_t ctx;
    int failures = 0;

    failures += init_default(&ctx);
    failures += require_valid_timestamp_step(&ctx, 0u);
    failures += require_true(ctx.sample_count == 1u,
                             "counter increments after first sample");
    failures += require_valid_timestamp_step(&ctx, 20u);
    failures += require_valid_timestamp_step(&ctx, 40u);
    failures += require_valid_timestamp_step(&ctx, 60u);
    failures += require_true(ctx.sample_count == 4u,
                             "counter tracks processed samples");
    failures += require_true(ppg_ibi_reset(&ctx) == PPG_IBI_STATUS_OK,
                             "reset succeeds");
    failures += require_true(ctx.sample_count == 0u,
                             "reset clears sample counter");

    return failures;
}

static int test_timestamp_anomaly(uint32_t t0,
                                  uint32_t t1,
                                  uint32_t t2,
                                  uint32_t t3,
                                  const char *message)
{
    ppg_ibi_context_t ctx;
    ppg_ibi_event_t event;
    bool has_event = true;
    int failures = 0;

    failures += init_default(&ctx);
    failures += require_valid_timestamp_step(&ctx, t0);
    failures += require_valid_timestamp_step(&ctx, t1);
    failures += require_valid_timestamp_step(&ctx, t2);
    failures += require_true(process_timestamp(&ctx, t3, true, &event,
                                               &has_event) ==
                                 PPG_IBI_STATUS_OK,
                             message);
    failures += require_true(!has_event, "timestamp anomaly emits no IBI");
    failures += require_true(event.state == PPG_IBI_STATE_REACQUIRE,
                             "timestamp anomaly enters REACQUIRE");
    failures += require_true(event.reject_reason ==
                                 PPG_IBI_REJECT_TIMESTAMP_ERROR,
                             "timestamp anomaly reject reason is timestamp");

    return failures;
}

static int test_allow_measure_false(void)
{
    ppg_ibi_context_t ctx;
    ppg_ibi_event_t event;
    bool has_event = true;
    int failures = 0;

    failures += init_default(&ctx);
    failures += require_valid_timestamp_step(&ctx, 0u);
    failures += require_true(process_timestamp(&ctx, 20u, false, &event,
                                               &has_event) ==
                                 PPG_IBI_STATUS_OK,
                             "allow_measure false process succeeds");
    failures += require_true(!has_event,
                             "allow_measure false emits no IBI");
    failures += require_true(event.state == PPG_IBI_STATE_REACQUIRE,
                             "allow_measure false enters REACQUIRE");
    failures += require_true(event.reject_reason ==
                                 PPG_IBI_REJECT_MEASURE_NOT_ALLOWED,
                             "allow_measure false reject reason is reported");

    return failures;
}

static int test_fixture_smoke(void)
{
    ppg_ibi_context_t ctx;
    ppg_ibi_sample_t sample = {0u, {0, 0, 0, 0}, true};
    ppg_ibi_event_t event;
    bool has_event;
    FILE *fixture;
    char header[96];
    char line[160];
    uint32_t row_count = 0u;
    int parsed;
    int failures = 0;

    failures += init_default(&ctx);

    fixture = fopen(SAMPLE_FIXTURE_PATH, "r");
    failures += require_true(fixture != NULL,
                             "tests/fixtures/sample_ppg.csv is readable");
    if (fixture == NULL)
    {
        return failures;
    }

    failures += require_true(fgets(header, sizeof(header), fixture) != NULL,
                             "fixture header is readable");
    failures += require_true(strstr(header, "timestamp_ms") != NULL,
                             "fixture header has timestamp_ms");
    failures += require_true(strstr(header, "PPG_G1") != NULL,
                             "fixture header has PPG_G1");
    failures += require_true(strstr(header, "PPG_G4") != NULL,
                             "fixture header has PPG_G4");

    while (fgets(line, sizeof(line), fixture) != NULL)
    {
        parsed = sscanf(line,
                        "%" SCNu32 ",%" SCNd32 ",%" SCNd32 ",%" SCNd32
                        ",%" SCNd32,
                        &sample.timestamp_ms, &sample.ppg[0], &sample.ppg[1],
                        &sample.ppg[2], &sample.ppg[3]);
        if (parsed != 5)
        {
            failures += require_true(false, "fixture row parses");
            break;
        }

        sample.allow_measure = true;
        has_event = true;
        failures += require_true(ppg_ibi_process(&ctx, &sample, &event,
                                                 &has_event) ==
                                     PPG_IBI_STATUS_OK,
                                 "fixture sample process succeeds");
        failures += require_true(!has_event,
                                 "fixture placeholder emits no IBI");
        failures += require_true(event.reject_reason !=
                                     PPG_IBI_REJECT_TIMESTAMP_ERROR,
                                 "fixture timestamp stays valid");
        row_count++;
    }

    failures += require_true(row_count == EXPECTED_FIXTURE_ROWS,
                             "fixture row count matches expected sample count");
    failures += require_true(ctx.sample_count == row_count,
                             "counter matches processed fixture rows");

    (void)fclose(fixture);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_normal_sequence();
    failures += test_timestamp_anomaly(0u, 20u, 40u, 30u,
                                       "timestamp rollback is detected");
    failures += test_timestamp_anomaly(0u, 20u, 40u, 40u,
                                       "timestamp repeat is detected");
    failures += test_timestamp_anomaly(0u, 20u, 40u, 200u,
                                       "timestamp jump is detected");
    failures += test_allow_measure_false();
    failures += test_fixture_smoke();

    return (failures == 0) ? 0 : 1;
}
