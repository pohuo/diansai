# 00 主控最小系统测试

目标：验证两块 MSPM0G3507 均可在三台电脑上离线编译、烧录并输出可观察结果。

## 接线

| 信号 | MSPM0 | 连接 |
|---|---|---|
| SWDIO | PA19 | DAPLink/XDS110 SWDIO |
| SWCLK | PA20 | DAPLink/XDS110 SWCLK |
| RST | 板卡标注 | 调试器 RST，若支持 |
| GND | GND | 调试器 GND |
| UART TX/RX | 待冻结 | USB 转 TTL，3.3V 逻辑，TX/RX 交叉 |

PA18 与 BSL 启动有关，测试前不得被外设错误拉高。不得将 5V TTL 接入 MSPM0 GPIO。

## 验收

- 两块板分别完成编译和烧录；
- LED 或串口输出连续运行 30 分钟；
- 三名队员至少各自独立完成一次；
- 保存 CCS、SDK、SysConfig 版本及提交号。
