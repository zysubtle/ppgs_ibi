# 06_RESOURCE_BUDGET.md：资源预算 v0.2（M2 可审查冻结版）

## 已确认资源约束

| 项目 | 约束 |
|---|---|
| MCU | Ambiq Apollo3.5 / Cortex-M4F 级 |
| RAM | 算法总 RAM < 20 KB |
| ROM | 暂不硬性要求 |
| 单次 process 时间 | 暂不硬性要求 |
| float | 允许 |
| malloc/calloc/realloc | 禁止 |
| MISRA 风格 | 需要 |

## M2 API 资源约束

| 项目 | M2 约定 |
|---|---|
| context 分配 | 由调用者分配 `ppg_ibi_context_t` |
| 动态内存 | API 和测试扫描禁止 `malloc` / `calloc` / `realloc` |
| 大数组上栈 | M2 API 未引入大数组 |
| 主要状态 | `ppg_ibi_context_t` 保存配置、状态、reject reason、sample counter、最近 timestamp、门控状态 |
| buffer | M2 未引入算法 buffer |
| RAM budget 宏 | `PPG_IBI_RAM_BUDGET_BYTES` = 20 KB |
| 编译测试 | `make test` 检查 `sizeof(ppg_ibi_context_t) < PPG_IBI_RAM_BUDGET_BYTES` |

## RAM 管理原则

1. 不使用动态内存。
2. 避免大数组上栈。
3. 算法状态放入 context 结构体。
4. 所有 buffer 长度通过配置宏集中定义。
5. 每个模块在实现后记录自身 RAM 预算。
6. 后续需要估算 context 总大小。

## RAM 台账

| 模块 | 主要 buffer / 状态 | 估算字节 | 状态 |
|---|---|---:|---|
| M2 API context | config、state、reject reason、counter、timestamp、门控状态 | host C 编译器下小于 64 B | M2 |
| input/counter | timestamp 校验状态，后续细化 | TBD | M3 |
| preprocess | TBD | TBD | M4 |
| sqi | TBD | TBD | M4 |
| detector | TBD | TBD | M5 |
| state/output | TBD | TBD | M6 |
| debug | TBD | TBD | M8/M9 |
| total | M2 context + 后续模块 | < 20 KB | 待审查 |

## 栈使用约束

- 不在函数内部定义大数组；
- 不使用递归；
- 临时变量数量受控；
- 后续测试或静态审查应关注 stack usage。

## ROM / 运行时间

当前不硬性约束，但不得实现明显不适合 MCU 的高复杂度算法。

后续如需增加硬性限制，必须作为 S0 决策记录。
