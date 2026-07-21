# 单模块测试区

线上人员在独立模块分支中准备代码和接线说明，文超平在线下完成接线、通电和实测。模块状态按以下顺序推进：

    draft -> code_ready -> wiring_reviewed -> hardware_testing -> passed / blocked

只有包含实测数据的 passed 模块才能合并到 main。

## 目录约定

每个模块目录包含：

- README.md：型号、供电、接口、接线步骤、预期现象和安全警告；
- *_interface.h：硬件适配接口，只声明 GPIO/PWM/计时/采样回调，不写死未冻结引脚；
- test_*.h/.c：非阻塞测试逻辑和明确错误码；
- result-template.md：线下实测记录。

公共状态定义位于 common/module_test_contract.h。未绑定硬件接口或尚未达到验收条件时，代码必须返回 BLOCKED 或 RUNNING，不得仅通过修改返回值宣称通过。

## 当前代码状态（2026-07-21）

矫利渝已确认文超平完成本轮 01、06、07、08 模块线下验证。由于各 result-template.md 尚未填写测量值、接线照片或日志，仓库先登记为 hardware_verified / evidence_pending；补齐证据后再改为 passed。登记见 hardware-verification-2026-07-21.md。

| 模块 | 代码状态 | 下一项线下交付 |
| --- | --- | --- |
| 00 板级启动 | code_ready / 已接工程 | 下载运行与 30 分钟心跳 |
| 01 电源监测 | hardware_verified / evidence_pending | 补 ADC 引脚、分压值与万用表数据 |
| 02 按键急停 | code_ready | 接线与抖动、长按、急停实测 |
| 03 TB6612 电机 | code_ready | 低占空比正反转与急停实测 |
| 04 编码器 | code_ready | 冻结 A/B 相引脚与每转计数 |
| 05 循迹传感器 | code_ready | 黑白标定与阈值记录 |
| 06 STP-23L | hardware_verified / decoder_pending | 补原始串口数据与厂家帧协议 |
| 07 视觉 UART | hardware_verified / evidence_pending | 补连续帧、错帧和拔线日志 |
| 08 二维云台 | hardware_verified / evidence_pending | 补驱动类型、引脚、零位和限位 |

## GitHub 流程

1. 从 main 建立 module/模块名 分支。
2. 矫利渝提交基础代码和接线草图。
3. 占蓝雪检查超时、错误处理、安全停机和验收标准。
4. 文超平拉取分支，按说明接线并填写 result-template.md。
5. 若失败，在记录中标记 WIRING、HARDWARE、CODE 或 SPEC_MISMATCH。
6. 线上修复、线下复测；通过后再提交 Pull Request 合入 main。