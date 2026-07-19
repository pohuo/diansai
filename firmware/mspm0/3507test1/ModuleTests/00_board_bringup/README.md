# 00 主控最小系统测试

目标：验证两块 MSPM0G3507 均可在三台电脑上离线编译、烧录并输出可观察结果。

## 当前代码设计

- CPUCLK 使用工程默认 32 MHz；
- SysTick 每 1 ms 进入一次中断；
- PA0 每 500 ms 翻转一次，形成 1 Hz 心跳；
- 主循环调用一次非阻塞状态检查后执行 WFI，后续模块可直接挂入主循环；
- 连续运行 30 分钟后 BoardBringupTest_RunOnce() 返回代码层 PASSED，但模块最终验收仍必须填写线下实测记录。

## 接线

| 信号 | MSPM0 | 连接 |
|---|---|---|
| SWDIO | PA19 | DAPLink/XDS110 SWDIO |
| SWCLK | PA20 | DAPLink/XDS110 SWCLK |
| RST | 板卡标注 | 调试器 RST，若支持 |
| GND | GND | 调试器 GND |
| HEARTBEAT | PA0 | 优先观察板载 LED；若板载 LED 未连接 PA0，则接 PA0 → 330 Ω 电阻 → LED → GND |
| UART TX/RX | 预留 PA10/PA11 | 本模块暂不启用；后续按默认 UART 接口冻结 |

PA18 与 BSL 启动有关，测试前不得被外设错误拉高。不得将 5V TTL 接入 MSPM0 GPIO。

PA0 是否连接板载 LED 取决于具体开发板版本。首次上电前，文超平必须核对开发板原理图或商品引脚图；若不一致，只在 SysConfig 中改 GPIO_STATUS/HEARTBEAT 分配，不在 C 文件中写死新引脚。

## 验收

- 两块板分别完成编译和烧录；
- PA0 心跳每 0.5 秒翻转一次，连续运行 30 分钟；
- 三名队员至少各自独立完成一次；
- 保存 CCS、SDK、SysConfig 版本及提交号。

## 状态与交付

- 矫利渝：维护状态机、主循环接入和接线说明；
- 占蓝雪：复核中断共享变量、溢出、安全停机和错误码；
- 文超平：核对 PA0 实际去向，完成两块板烧录及 30 分钟实测；
- 当前状态：code_ready，硬件确认前不得标记为 passed。
