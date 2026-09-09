# PVZHelper 基于Inline Hook的Windows游戏热补丁

---
##  项目简介

本项目以《植物大战僵尸》为实验目标，实现了一套完整的基于 **DLL注入 + Inline Hook** 热补丁方案。通过 `CreateRemoteThread` + `LoadLibrary` 将 DLL 注入目标进程，在 DLL 内部实现内存读写和 Hook ，支持可交互控制台实时执行指令，动态修改游戏行为。

---

##  核心功能

###  内存读写 模块
封装了 `VirtualQuery` / `VirtualProtect` / `memcpy` 流程，实现内存属性的安全修改与恢复，所有内存操作统一调用该模块。

###  Inline Hook 模块
- 覆盖目标函数入口 **6字节**（`55 8B EC 83 E4 F8`）
- 通过 `JMP` 指令劫持执行流至跳板函数
- 修改函数参数（如强制僵尸生成在中路）
- 跳回原始函数继续执行（返回地址 = 原始地址 + 6）

### 运行时控制
- 控制台程序与 DLL 通过scanf输入交互
- 支持：阳光修改 / 金币修改 / 僵尸生成 / Hook 安装与卸载
- 状态标志防止重复安装，重复卸载导致崩溃
  
---
## 版本兼容性声明
该项目的植物大战僵尸版本是`Plants_Vs_Zombies_V1.0.0.1051_EN`，其他版本可能会因为金币阳光的偏移量变化导致失效或者崩溃，hook的函数使用的是特征码查询，所以在不同版本能够正常使用

## 快速开始

### 1. 准备工作
1.使用 Visual Studio 打开解决方案，编译 `PVZHook` 项目（生成 `PVZHook.dll`）和 `Injector` 项目（生成 `Injector.exe`）。
2.将生成的`PVZHook.dll` 和 `Injector.exe`放到 `popcapgame1.exe`（植物大战僵尸）当前的目录下

**注：在游戏目录下已经保持了编译完成后的`PVZHook.dll`、`Injector.exe`。**

### 2. 启动与注入
1. 启动 `popcapgame1.exe`（植物大战僵尸）
2. 运行 `Injector.exe`，即可完成注入（注入器会通过检索进程名的方式自动获取PID，无需手动输入PID）
3.运行`popcapgame1.exe`后退出，把现有的《植物大战僵尸》原版存档,位于C盘ProgramData文件夹下的PopCap Games\PlantsVsZombie\userdata文件夹 中的内容替换为该项目的`userdata`
### 3. 操作
在控制台窗口输入对应命令：

| 命令 | 功能 |
| :--- | :--- |
| `1` | 自定义修改阳光数量 |
| `2` | 自定义修改金币数量 |
| `3` | 生成一只僵尸 |
| `4` | 安装 Inline Hook（僵尸强制出现于中路、并且更改了出现的僵尸种类） |
| `5` | 卸载 Hook |

---

## 效果演示
1.控制台操作界面
![控制台操作界面](https://github.com/Smile-axis/PVZ_Helper/blob/main/%E6%BC%94%E7%A4%BA%E5%9B%BE%E7%89%87/%E6%B3%A8%E5%85%A5%E5%90%8E%E4%B8%BB%E7%95%8C%E9%9D%A2.png))

2.自定义阳光数值
![自定义阳光](https://github.com/Smile-axis/PVZ_Helper/blob/main/%E6%BC%94%E7%A4%BA%E5%9B%BE%E7%89%87/%E8%87%AA%E5%AE%9A%E4%B9%89%E9%98%B3%E5%85%89%E6%95%B0%E6%8D%AE.png)

3.自定义金币数值
![自定义金币](https://github.com/Smile-axis/PVZ_Helper/blob/main/%E6%BC%94%E7%A4%BA%E5%9B%BE%E7%89%87/%E8%87%AA%E5%AE%9A%E4%B9%89%E9%98%B3%E5%85%89%E6%95%B0%E6%8D%AE.png)

4.调用僵尸生成函数，在一路生成僵尸
![调用僵尸生成函数](https://github.com/Smile-axis/PVZ_Helper/blob/main/%E6%BC%94%E7%A4%BA%E5%9B%BE%E7%89%87/%E8%B0%83%E7%94%A8%E5%83%B5%E5%B0%B8%E7%94%9F%E6%88%90%E5%87%BD%E6%95%B0%EF%BC%8C%E5%9C%A81%E8%B7%AF%E7%94%9F%E6%88%90.png)

5.装载钩子，修改僵尸种类并只出现在中路
![装载钩子，修改僵尸种类并只出现在中路](https://github.com/Smile-axis/PVZ_Helper/blob/main/%E6%BC%94%E7%A4%BA%E5%9B%BE%E7%89%87/%E8%A3%85%E8%BD%BD%E9%92%A9%E5%AD%90.png)

6.卸载钩子，僵尸生成恢复正常
![卸载钩子](https://github.com/Smile-axis/PVZ_Helper/blob/main/%E6%BC%94%E7%A4%BA%E5%9B%BE%E7%89%87/%E5%8D%B8%E8%BD%BD%E9%92%A9%E5%AD%90.png))

---


## 核心难点

### 特征码查询函数
通过IDA分析获取的指定函数特征码，在模块中查询函数的位置，计算偏移得到函数的地址

### 指令边界对齐
目标函数开头为 6 字节（`55 8B EC 83 E4 F8`），因此返回地址必须为 `原始地址 + 6`，而非常规的 `+5`，否则会跳回指令中间导致崩溃。

### 跳板函数硬编码
使用 裸函数`__declspec(naked)` + `_emit` 插入机器码，确保跳板函数与原函数开头的机器码（`
0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8`）完全一致，使用裸函数避免编译器插入额外指令破坏堆栈。

---

## 技术栈

| 类别 | 技术 |
| :--- | :--- |
| 语言 | C / C++ / x86 汇编 |
| 底层 | Win32 API / PE结构 / Inline Hook / DLL注入 |
| 工具 | Visual Studio 2022 / IDA Pro / x64dbg / Cheat Engine |

---


## 运行环境
运行环境
Windows 10 / 11 (x64)


