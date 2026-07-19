# 海南省 2026 大学生电子设计竞赛备赛仓库

本仓库用于三人团队协作开发地面控制类 MSPM0G3507 平台。`main` 只保存经复测、可编译、可烧录或明确标注为文档状态的版本。

## 当前目标

在 2026 年 7 月 28 日 20:00 前交付一台以 MSPM0 为唯一控制核心、可离线烧录并稳定复现基本动作的基线车：

- 一键冷启动、急停和安全停机可用；
- 双电机可控正反转，完成 1 m 直行和 90° 转向；
- 循迹或测距至少完成一个稳定闭环；
- 完整流程测试 10 次，至少成功 9 次；
- 保存测试数据、接线图、源代码以及可烧录备份。

## 仓库结构

```text
firmware/mspm0/   CCS 工程与源码
hardware/         接线图、引脚表和硬件验收资料
test-data/        电压、电流、PID、误差和成功率记录
docs/             报告、备赛指南和进度看板
```

## 团队分工

- 矫利渝：统筹、软件架构、控制算法、系统集成和报告发布包。
- 文超平：设备验收、电源、底盘、传感器、线束和实测数据。
- 占蓝雪：代码审查、Debug、回归测试、缺陷闭环和发布验收。

详细责任与交付物见 [`docs/team-responsibilities.md`](docs/team-responsibilities.md)。

## CCS 工程

当前最小工程位于 `firmware/mspm0/3507test1/`。导入 CCS 后先确认本机 MSPM0 SDK 与 SysConfig 版本，再编译和烧录。仓库不包含 CCS 安装文件、MSPM0 SDK 或 `Debug/` 编译目录。

## 版本规则

1. 开工前执行 `git pull --ff-only`。
2. 从 `main` 创建个人任务分支，例如 `feat/motor-pwm`、`test/power-rails` 或 `fix/encoder-noise`。
3. 一次提交只解决一个问题，提交说明使用动词和具体对象。
4. 推送分支并发起 Pull Request，由另一名队员复测后合并。
5. 稳定节点使用标签：`motor_ok`、`chassis_ok`、`basic_ok`、`final_candidate`。

协作命令和验收要求见 [`CONTRIBUTING.md`](CONTRIBUTING.md)。
