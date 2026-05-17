# 04_IO_CONTRACT.md：IO Contract v0.2（M2 可审查冻结版）

> 本文件与 `include/ppg_ibi.h`、`include/ppg_ibi_config.h` 保持一致。后续改变公开 API、结构体字段、采样率、状态机主策略或输出字段，均需按 S0 事项提交 Owner 决策。

## 输入需求

每个样本逐点输入。

已冻结输入信息：

| 字段 | C API 类型 | 状态 | 说明 |
|---|---|---|---|
| `timestamp_ms` | `uint32_t` | M2 冻结 | 外部时间戳，单位 ms |
| `ppg[PPG_IBI_CHANNEL_COUNT]` | `int32_t[4]` | M2 冻结 | 4 路同步绿光 PPG |
| `allow_measure` | `bool` | M2 冻结 | 外部运动/测量门控 |

固定输入规格：

- 采样率：50 Hz，宏为 `PPG_IBI_SAMPLE_RATE_HZ`；
- 时间戳期望步长：20 ms，宏为 `PPG_IBI_SAMPLE_INTERVAL_MS`；
- raw 类型：signed `int32_t`；
- ADC 位宽：24 bit，宏为 `PPG_IBI_ADC_BITS`；
- 多通道严格同步。

## 配置需求

C API 提供 `ppg_ibi_config_t`，用于集中保存固定配置和 timestamp 容忍参数。

| 字段 | 类型 | 默认值 | 说明 |
|---|---|---:|---|
| `sample_rate_hz` | `uint16_t` | 50 | 必须等于 `PPG_IBI_SAMPLE_RATE_HZ` |
| `expected_sample_interval_ms` | `uint16_t` | 20 | 必须等于 `PPG_IBI_SAMPLE_INTERVAL_MS` |
| `timestamp_tolerance_ms` | `uint16_t` | 2 | timestamp 步长占位校验容忍 |
| `channel_count` | `uint8_t` | 4 | 必须等于 `PPG_IBI_CHANNEL_COUNT` |

说明：配置结构体不表示允许改变采样率或通道数。`ppg_ibi_init()` 会拒绝非 50 Hz、非 20 ms、非 4 通道配置。

## 输出需求

每检测到一个有效 IBI，输出一次 event。

已冻结输出字段：

| 字段 | C API 类型 | 状态 | 说明 |
|---|---|---|---|
| `timestamp_ms` | `uint32_t` | M2 冻结 | event 对应时间，建议为后一个有效脉搏时间 |
| `ibi_ms` | `uint32_t` | M2 冻结 | 当前 IBI，单位 ms |
| `confidence` | `float` | M2 冻结 | 范围 0.0-1.0 |
| `signal_quality` | `float` | M2 冻结 | 范围 0.0-1.0 |
| `selected_channel` | `uint8_t` | M2 冻结 | 0-3；无选中通道时为 `PPG_IBI_SELECTED_CHANNEL_NONE` |
| `state` | `ppg_ibi_state_t` | M2 冻结 | 当前算法状态 |
| `reject_reason` | `ppg_ibi_reject_reason_t` | M2 冻结 | 拒绝或状态转换原因 |

API 不输出：

- `hr_bpm`；
- `rmssd`；
- 长期 HRV 指标；
- 医疗诊断结论。

## 状态 enum

`ppg_ibi_state_t` 冻结为：

```text
PPG_IBI_STATE_INIT
PPG_IBI_STATE_ACQUIRE
PPG_IBI_STATE_TRACK
PPG_IBI_STATE_REACQUIRE
PPG_IBI_STATE_INVALID
```

说明：不默认采用 HOLD 输出策略。`allow_measure = false` 时立即停止输出，恢复后进入 `REACQUIRE`。

## reject_reason enum

`ppg_ibi_reject_reason_t` 冻结为：

```text
PPG_IBI_REJECT_NONE
PPG_IBI_REJECT_NOT_READY
PPG_IBI_REJECT_MEASURE_NOT_ALLOWED
PPG_IBI_REJECT_LOW_SIGNAL_QUALITY
PPG_IBI_REJECT_TIMESTAMP_ERROR
PPG_IBI_REJECT_IBI_OUT_OF_RANGE
PPG_IBI_REJECT_CHANNEL_INVALID
PPG_IBI_REJECT_STATE_INVALID
```

## status enum

`ppg_ibi_status_t` 冻结为：

```text
PPG_IBI_STATUS_OK
PPG_IBI_STATUS_ERROR_NULL
PPG_IBI_STATUS_ERROR_CONFIG
```

## API 函数

公开 API：

```c
const char *ppg_ibi_version(void);
void ppg_ibi_config_default(ppg_ibi_config_t *config);
ppg_ibi_status_t ppg_ibi_init(ppg_ibi_context_t *ctx,
                              const ppg_ibi_config_t *config);
ppg_ibi_status_t ppg_ibi_reset(ppg_ibi_context_t *ctx);
ppg_ibi_status_t ppg_ibi_process(ppg_ibi_context_t *ctx,
                                 const ppg_ibi_sample_t *sample,
                                 ppg_ibi_event_t *event,
                                 bool *has_event);
```

处理约定：

1. `ppg_ibi_context_t` 由调用者分配，API 内部不使用动态内存。
2. `config == NULL` 时，`ppg_ibi_init()` 使用默认固定配置。
3. `ppg_ibi_reset()` 只重置运行状态，不改变已保存配置。
4. `ppg_ibi_process()` 逐点输入。
5. `has_event == true` 表示本次产生新的有效 IBI event。
6. 当前 M2 占位实现不做峰值检测，因此始终不产生有效 IBI event。
7. `has_event == false` 时，`event` 可携带当前占位状态和拒绝原因；调用方不得将其当作有效 IBI。
8. API 不输出 `hr_bpm`、`rmssd` 或长期 HRV 指标。

## API 设计约束

M2 C API 已按以下约束建立：

1. `init` / `reset` / `process` 接口清晰；
2. 不使用动态内存；
3. 所有算法状态由 caller 分配的 context 承载；
4. `process` 支持逐点调用；
5. 输出 event 应明确区分“本次有新 IBI”和“本次无输出”；
6. 不输出 HR 或 RMSSD；
7. 不改变 Project Brief 已确认字段。
