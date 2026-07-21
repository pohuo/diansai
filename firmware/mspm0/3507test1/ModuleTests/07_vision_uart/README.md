# 07 MaixCAM Pro 视觉串口测试

负责人：矫利渝（MSPM0 接收、接口冻结、验收）；占蓝雪（MaixCAM 发送与协议精进）；文超平（接线与实测）。

## 已冻结接口

- MSPM0G3507 LQFP-48：UART1_TX = PA8，UART1_RX = PA9。
- 115200 baud、8N1、无流控，TX/RX 交叉并共地。
- MaixCAM 使用独立、稳定且经验证的 5 V 供电；通信必须确认是 3.3 V UART 逻辑。
- 数据协议见 protocol.md，与远端 PR #2 的基础发送脚本兼容。

## 已交付

- test_vision_uart.c/.h：非阻塞 ASCII 帧解析、字段/校验/序号检查、连续 100 帧验收和 200 ms 超时。
- vision_uart_mspm0_adapter.c/.h：UART1 RX 中断与 128 字节单生产者/单消费者环形缓冲区。
- vision_uart_interface.h：解析器与板级适配的最小接口。
- empty.syscfg：由 SysConfig CLI 验证的 UART1 PA8/PA9 配置。
- result-template.md：线下验收记录。

## 安全约束

UART ISR 只搬运字节，解析与安全处理在主循环执行。接收环形缓冲区溢出或视觉超时均判为失败，并调用底盘电机安全停止；MaixCAM 不直接控制电机。

## 验收

1. 核对 PA8/PA9、电平、交叉连接和共地后再上电。
2. 固定目标下连续接收 100 个正确且序号连续的帧。
3. 分别注入错校验、非法坐标、丢帧和超长帧，统计值必须增加且连续计数清零。
4. 拔掉视觉串口，200 ms 后旧目标必须失效且电机进入安全停止。
5. 运行 30 分钟，环形缓冲区溢出计数必须保持为 0。