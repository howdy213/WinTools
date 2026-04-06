# TaskManager

## 项目介绍

任务调度工具，用于根据 INI 配置文件中设定的时间自动执行指定命令。  
支持**时间点**（每天固定时刻执行一次）和**时间段**（每天在某个时间段内执行一次）两种触发模式。

## 使用说明

- 运行方式：直接运行程序或通过命令行指定 INI 配置文件路径  
  - 示例：`TaskManager.exe C:\tasks.ini`  
  - 若不指定路径，默认使用程序目录下的 `tasks.ini` 文件

- INI 配置文件格式：

### 时间点模式

```ini
[任务1名称]
Time=HH:MM:SS
Command=notepad "D:\example.txt"
ShowCmd=1
RunAsAdmin=0
```

### 时间段模式

```ini
[任务2名称]
StartTime=HH:MM:SS
EndTime=HH:MM:SS
Command=backup.bat
ShowCmd=0
RunAsAdmin=1
```

**说明：**
- `Time` 与 `StartTime+EndTime` 二选一，不可同时使用
- 时间段模式中，`StartTime` 必须早于 `EndTime`，不支持跨天（例如 23:00:00 至 01:00:00 无效）
- 时间段内每天只会触发一次（进入时间段后的第一次检测时执行）
- 实际执行的是 `cmd /c Command`，因此 `Command` 可以是任意可执行的命令或脚本
- `ShowCmd` 值对应 Windows `SW_*` 常量，用于控制窗口显示方式（通常填 `0` 可隐藏 cmd 窗口）
- `RunAsAdmin` 为 `1` 时，将以管理员权限运行（可能会触发 UAC 弹窗）

### 完整配置示例

```ini
[MorningTask]
Time=09:00:00
Command=echo "Good morning" >> C:\log.txt
ShowCmd=0
RunAsAdmin=0

[WorkHours]
StartTime=09:00:00
EndTime=17:00:00
Command=C:\Tools\report.exe -auto
ShowCmd=7
RunAsAdmin=0

[NightCleanup]
StartTime=22:00:00
EndTime=23:00:00
Command=cleanup.cmd /silent
ShowCmd=0
RunAsAdmin=1
```

## 功能特点

- 支持精确时间点触发（误差 ±3 秒）  
- 支持时间段触发（时间段内只执行一次）  
- 每个任务每天最多执行一次，跨日自动重置  
- 程序持续运行，实时监控任务时间  
- 支持配置文件热加载：修改 `tasks.ini` 后自动重新读取，无需重启  
- 单实例控制：程序已有实例运行时，会询问是否关闭所有实例  

## 环境依赖

- Visual Studio 2022  
- Windows SDK（10.0+）  
- 项目内依赖 `WinUtils` 库

## 许可证

本项目采用 GPLv3 许可证，详情参见 LICENSE 文件。