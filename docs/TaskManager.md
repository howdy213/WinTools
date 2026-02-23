# TaskManager

## 项目介绍

任务调度工具，用于根据 INI 配置文件中设定的时间自动执行指定命令

## 使用说明

- 运行方式：直接运行程序或通过命令行指定 INI 配置文件路径

  - 示例：`TaskManager.exe C:\tasks.ini`
  - 若不指定路径，默认使用程序目录下的`tasks.ini`文件

- INI 配置文件格式：

  ```ini
  [任务1名称]
  Time=HH:MM:SS
  Command=notepad "D:\example.txt"
  ShowCmd=1
  RunAsAdmin=0
  
  [任务2名称]
  ...
  ```
  注：

  - Time项必须补零
  - 实际执行的是cmd /c Command，可以不局限于文件名+参数的格式
  - ShowCmd值同SW_*相关常量，仅用于cmd窗口（一般填0，不出现cmd窗口）
  - RunAsAdmin同样用于cmd窗口，等同于向命令行启动的程序提供管理员权限

- 功能特点：

  - 任务每天执行一次，时间误差在 ±3 秒内
  - 程序运行时会持续监控任务时间，直到手动关闭
  - 程序已有实例运行时会提醒是否关闭所有实例

## 环境依赖

Visual Studio 2022

## 许可证

本项目采用 GPLv3 许可证，详情参见 LICENSE 文件