# 11_M3_ACCEPTANCE_REPORT.md：M3 验收报告

## 实现摘要

M3 在 M2 已冻结 API 上实现了逐点输入链路的基础时序检查：

- `ppg_ibi_context_t.sample_count` 在 init/reset 后归零，每成功处理一个输入样本递增；
- 正常 20 ms timestamp 步长不触发异常；
- timestamp 倒退、重复、大幅跳变/缺样触发 `PPG_IBI_REJECT_TIMESTAMP_ERROR`；
- timestamp 异常时不输出有效 IBI，状态进入或保持 `PPG_IBI_STATE_REACQUIRE`；
- `allow_measure = false` 时不输出 IBI，状态进入或保持 `PPG_IBI_STATE_REACQUIRE`；
- 使用 `tests/fixtures/sample_ppg.csv` 做全量逐点 smoke test。

本阶段未实现 SQI、主通道选择、滤波、脉搏峰检测、真实 IBI、HR 或 RMSSD。

## 修改文件

- `src/ppg_ibi.c`
- `tests/test_input_timing.c`
- `Makefile`
- `docs/04_IO_CONTRACT.md`
- `docs/06_RESOURCE_BUDGET.md`
- `docs/08_RISK_REVIEW.md`
- `docs/11_M3_ACCEPTANCE_REPORT.md`

## 测试命令

```sh
make test
rg -n "\b(malloc|calloc|realloc)\s*\(" include src tests || true
rg -n "hr_bpm|rmssd|RMSSD" include src tests Makefile || true
```

## 测试结果

- `make test`：通过。
- API 编译 / 链接测试：通过。
- fixture header 检查：通过。
- `tests/fixtures/sample_ppg.csv` 全量逐点输入 smoke test：通过。
- 正常 timestamp 序列 `0, 20, 40, 60`：未触发 timestamp 异常。
- timestamp 倒退序列 `0, 20, 40, 30`：检测为 timestamp 异常。
- timestamp 重复序列 `0, 20, 40, 40`：检测为 timestamp 异常。
- timestamp 跳变序列 `0, 20, 40, 200`：检测为 timestamp 异常。
- `allow_measure = false`：不输出 IBI，进入 `REACQUIRE`。
- 动态内存扫描：未发现 `malloc/calloc/realloc` 调用。
- HR/RMSSD 输出字段扫描：未发现 `hr_bpm`、`rmssd` 或 `RMSSD`。

## 已知限制

- M3 只实现输入时序和门控衔接，不实现 PPG-IBI 检测算法。
- timestamp 异常后采用保守策略，不做复杂重同步。
- fixture smoke test 只能证明工程链路可运行，不能证明 IBI 准确性。

## 约束检查

| 项目 | 结果 |
|---|---|
| 是否修改 API | 否 |
| 是否引入动态内存 | 否 |
| 是否引入第三方依赖 | 否 |
| 是否仍满足 RAM < 20 KB | 是 |
| 是否使用 `tests/fixtures/sample_ppg.csv` | 是 |
| 是否实现 SQI | 否 |
| 是否实现通道选择 | 否 |
| 是否实现滤波 | 否 |
| 是否实现峰值检测 | 否 |
| 是否实现真实 IBI | 否 |
| 是否输出 HR / RMSSD | 否 |
