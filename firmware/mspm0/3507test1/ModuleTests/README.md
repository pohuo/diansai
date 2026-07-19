# 单模块测试区

线上人员在独立模块分支中准备代码和接线说明，文超平在线下完成接线、通电和实测。模块状态按以下顺序推进：

```text
draft -> code_ready -> wiring_reviewed -> hardware_testing -> passed / blocked
```

只有包含实测数据的 `passed` 模块才能合并到 `main`。

## 目录约定

每个模块目录包含：

- `README.md`：型号、供电、接口、接线步骤、预期现象和安全警告；
- `*_interface.h`：硬件适配接口，只声明 GPIO/PWM/计时/采样回调，不写死未冻结引脚；
- `test_*.h/.c`：单模块测试接口；
- `result-template.md`：线下实测记录。

公共状态定义位于 `common/module_test_contract.h`。初始 C 文件均返回 `MODULE_TEST_BLOCKED`，表示引脚和硬件尚未验收；实现代码后不得仅通过修改返回值宣称通过。

## GitHub 流程

1. 从 `main` 建立 `module/<模块名>` 分支。
2. 矫利渝提交基础代码和接线草图。
3. 占蓝雪检查超时、错误处理、安全停机和验收标准。
4. 文超平拉取分支，按说明接线并填写 `result-template.md`。
5. 若失败，在记录中标记 `WIRING`、`HARDWARE`、`CODE` 或 `SPEC_MISMATCH`。
6. 线上修复、线下复测；通过后再提交 Pull Request 合入 `main`。

## 当前优先级

1. `00_board_bringup`
2. `01_power_monitor`
3. `02_button_estop`
4. `03_motor_tb6612`
5. `04_encoder`
6. `05_line_sensor`
7. `06_tof_stp23l`
8. `07_vision_uart`
9. `08_gimbal`
