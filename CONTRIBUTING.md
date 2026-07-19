# 三人协作规则

## 分支命名

- 矫利渝：`feat/control-*`、`feat/integration-*`、`docs/report-*`
- 文超平：`feat/hardware-*`、`test/module-*`
- 占蓝雪：`fix/*`、`test/regression-*`

禁止直接在 `main` 上试验高风险改动。

## 每次开始工作

```powershell
git switch main
git pull --ff-only
git switch -c feat/具体任务
```

已有任务分支则使用 `git switch 分支名`，不要重复创建。

## 提交与推送

```powershell
git status
git add 明确的文件路径
git commit -m "完成单电机PWM限幅"
git push -u origin 当前分支名
```

不要使用未经检查的 `git add -A`。不得提交 CCS/SDK 安装目录、`Debug/`、临时日志或与任务无关的大文件。

## Pull Request 验收

PR 描述必须写明：

- 改了什么；
- 使用的硬件、供电和固件版本；
- 测试条件与数据；
- 已知限制；
- 回退到哪个稳定标签。

复核人必须能按照说明独立复现。P0 功能没有数据或版本号时不得合入 `main`。

## 发布标签

```powershell
git switch main
git pull --ff-only
git tag -a motor_ok -m "双电机闭环已验收"
git push origin motor_ok
```

同名标签不重复创建。需要更新节点时使用带日期或版本的标签，例如 `motor_ok_v2`。
