# MaixCAM Pro 基础模块测试

## 测试边界

这组脚本测试 MaixCAM Pro 的摄像头、屏幕、颜色目标识别和 UART1。测试目标可使用手机或电脑屏幕上的彩色圆，不需要真实小球，也不需要 YOLO 模型。

MaixCAM Pro 运行 MaixPy v4，脚本通过 MaixVision 逐个发送并运行。不要使用 K210/MaixPy v1 教程。

## 接线

### 单板视觉测试

运行 `01_camera_preview.py` 和 `02_color_target.py` 时无需连接任何杜邦线，只给 MaixCAM Pro 正常供电。

### MaixCAM UART1 回环测试

先断电，用一根杜邦线连接：

```text
MaixCAM A19 / UART1_TX -> MaixCAM A18 / UART1_RX
```

重新上电后运行 `04_uart_loopback.py`。连续出现 `PASS` 即表示 UART1 的发送和接收都正常。完成后断电并拆除回环线。

### MaixCAM 与 MSPM0G3507 联调

两块板分别使用自己的 USB 或电池供电，只连接三根信号线：

```text
MaixCAM A19 / UART1_TX -> MSPM0 A9 / UART1_RX
MaixCAM A18 / UART1_RX <- MSPM0 A8 / UART1_TX
MaixCAM GND             -- MSPM0 GND
```

不要连接两块板之间的 3V3、5V、VBUS 或 VSYS。

串口参数暂定为 `115200, 8N1`。

## 测试目标准备

将 `phone_target.html` 发到手机或电脑，用浏览器打开。页面可以：

- 切换红、绿、蓝色；
- 改变圆形大小；
- 让圆形左右移动；
- 隐藏目标，模拟目标丢失。

首轮使用红色。屏幕亮度调到约 70%，关闭护眼模式，将 MaixCAM 放在屏幕前约 30~80 cm。

## 逐项操作与验收

### TEST01 摄像头与屏幕

1. MaixCAM 单独上电并连接 MaixVision。
2. 运行 `01_camera_preview.py`。
3. 连续观察 10 分钟，期间缓慢移动摄像头。

通过标准：画面正常、方向正确、无持续花屏、无卡死，屏幕左上角显示 `TEST01 CAMERA OK`。

### TEST02 彩色目标识别

1. 打开 `phone_target.html`，选择红色并停止移动。
2. 运行 `02_color_target.py`。
3. 依次把圆放在画面中心、左、右、上、下。
4. 调整圆的大小，模拟远近变化。
5. 点击“隐藏目标”。
6. 可用另一个红色物体制造干扰。

通过标准：目标出现时画出绿色框并输出随位置变化的 `x/y`；目标隐藏后显示 `TARGET LOST`；多个目标时选择面积最大的目标。

若完全识别不到或误识别严重，先用 MaixCAM 自带“找色块”应用测出 LAB 阈值，再修改脚本顶部的 `RED_THRESHOLD`。不要在算法主体中到处改数值。

### TEST03 固定坐标串口发送

1. 尚未连接 MSPM0 时可以先运行脚本查看 MaixVision 日志。
2. 运行 `03_uart_fixed_tx.py`。
3. 日志应每 500 ms 循环打印中心、左、右、上、下和无目标数据。
4. 接入 MSPM0 后，由 MSPM0 接收程序检查收到的内容。

通过标准：序号连续递增；位置按固定顺序循环；无乱码；MSPM0 能区分 `valid=1` 和 `valid=0`。

### TEST04 UART1 本地回环

1. 断电后连接 MaixCAM A19 与 A18。
2. 上电并运行 `04_uart_loopback.py`。
3. 至少观察 100 帧。

通过标准：100 帧全部 `PASS`。出现 `FAIL` 时记录序号、收到的数据和 MaixPy 系统版本。

### TEST05 视觉与串口联合

1. 拆除回环线，按三线表连接 MSPM0。
2. 两块板分别上电。
3. 运行 `05_vision_uart.py`。
4. 在测试页面中移动、缩小和隐藏红色圆。
5. 对照 MaixCAM 屏幕坐标、MaixVision 日志和 MSPM0 接收值。

通过标准：坐标方向一致，目标隐藏时 `valid=0`，重新出现时自动恢复；连续运行 10 分钟无卡死；MSPM0 超过 200 ms 没收到有效帧时应判定视觉断线。

## 测试记录必须包含

- 日期与操作者；
- MaixPy 系统版本；
- 使用的脚本名和 Git 提交号；
- 接线照片；
- 手机/电脑屏幕亮度和距离；
- LAB 阈值；
- 每项成功次数、失败次数；
- 失败日志或视频。

## 临时可读协议

测试脚本使用一行一个数据包：

```text
$V,seq,valid,x,y,checksum*\n
```

- `seq`：0~255 循环序号；
- `valid`：1 表示识别到目标，0 表示无目标；
- `x/y`：320x240 图像中的目标中心；无目标时均为 0；
- `checksum`：字符串 `V,seq,valid,x,y` 所有 ASCII 字节求和后 `% 255`。

该协议用于基础联调，正式比赛协议冻结时可换成带明确长度字段的二进制帧。

## 关于 YOLO

基础模块测试不需要 YOLO。先用颜色阈值验证摄像头、坐标、目标丢失和 UART 整条链路。只有实际题目中目标颜色不固定、背景复杂或必须区分多类物体时，再采集真实场景数据训练 YOLO。
