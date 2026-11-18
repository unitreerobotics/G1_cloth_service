<div align="center">
  <h1 align="center">
    <a href="https://www.unitree.com/cn" target="_blank">G1 Cloth Service</a>
  </h1>
  <a href="https://www.unitree.com/" target="_blank">
    <img src="https://www.unitree.com/images/0079f8938336436e955ea3a98c4e1e59.svg" alt="Unitree LOGO" width="15%">
  </a>
  <p align="center">
    <a href="README.md"> English </a> | <a>中文</a> </a>
  </p>
</div>

# 0. 📖 介绍

这是一个用于 G1 触觉传感服的 serial2dds 服务。

传感服通过 CH343 USB 转串口芯片（VID:PID `1a86:55d3`，设备节点 `/dev/ttyACM*`）连接，波特率 921600。服务读取并融合串口数据帧（256 路传感器数据 + 陀螺仪四元数），并以 100 Hz 通过 DDS 发布。

```mermaid
graph LR
A(传感服) -- 串口 /dev/ttyACMx @921600 --> B(cloth_service)
B(cloth_service) -- rt/jq_clothstate --> C(用户)
```

DDS 消息布局（话题 `rt/jq_clothstate`，类型 `sensor_msgs::msg::dds_::PointCloud2_`，仅使用 `data` 字节数组）：

| 字节          | 内容                                             |
| ------------- | ------------------------------------------------ |
| `data[0..15]`   | 陀螺仪四元数，4 个 float32（w, x, y, z），小端 |
| `data[16..271]` | 256 路传感器数据，每路 8 bit                     |

# 1. 📦 安装

```bash
sudo apt update
sudo apt install libserialport-dev
cd ~
git clone https://github.com/unitreerobotics/G1_cloth_service
cd G1_cloth_service
bash build.sh
```

构建脚本使用系统默认编译器（支持 C++17，CMake >= 3.10），并根据当前主机架构自动选择对应的 SDK 库。unitree_sdk2（DDS）已随仓库自带（`3rd/unitree_sdk2`，预编译了 `x86_64` 和 `aarch64` 两种架构）。可执行文件生成在 `bin/` 目录下：

- `bin/cloth_service` —— 服务本体
- `bin/JqDdsTest` —— DDS 订阅测试工具

> 💡 注意：在 `3rd/serialport` 目录下提供了上述安装命令中 sudo apt install libserialport-dev 的离线 deb 文件（Ubuntu 22.04），同时提供 `arm64`（Jetson）和 `amd64`（x86_64）两种架构，以供用户方便离线安装。如果你在安装 libserialport 时遇到问题，可以尝试安装这些 deb 文件。要注意安装顺序，以下是安装命令示例：
> ```bash
> # arm64（NVIDIA Jetson Orin NX）
> sudo dpkg -i 3rd/serialport/libserialport0_0.1.1-3_arm64.deb
> sudo dpkg -i 3rd/serialport/libserialport-dev_0.1.1-3_arm64.deb
>
> # amd64 / x86_64 PC
> sudo dpkg -i 3rd/serialport/libserialport0_0.1.1-3_amd64.deb
> sudo dpkg -i 3rd/serialport/libserialport-dev_0.1.1-3_amd64.deb
> ```

# 2. 🚀 启动

```bash
# 启动服务（访问串口需要 root 或 dialout 用户组权限）
sudo ./bin/cloth_service
```

服务启动后最多等待 30 秒让 USB 设备完成枚举，随后打开串口并开始发布数据。

运行测试订阅程序验证数据：

```bash
# 通过本机回环（默认）或指定网卡订阅
./bin/JqDdsTest
./bin/JqDdsTest eth0
```

程序会打印接收频率、陀螺仪四元数和 256 路传感器数据。

# 3. 🚀🚀🚀 开机自启服务

在编译并成功测试 `cloth_service` 后，可运行以下脚本将其配置为开机自动运行：

```bash
bash setup_autostart.sh
```

脚本会安装 `G1_cloth.service`（systemd），开机自动启动 `bin/cloth_service`（`Restart=always`，用户 `unitree`，用户组 `dialout`，并已将 `LD_LIBRARY_PATH` 指向仓库自带的 SDK 库）。管理服务：

```bash
sudo systemctl status G1_cloth.service      # 查看状态
sudo journalctl -u G1_cloth.service -f      # 实时查看日志
sudo systemctl restart G1_cloth.service     # 重启服务
sudo systemctl stop G1_cloth.service        # 停止服务
sudo systemctl disable G1_cloth.service     # 关闭开机自启
```

除了 `journalctl`，还可以在 `/unitree/var/log/cloth_service` 目录下查看历史日志文件（10MB × 3 异步轮转，由 `config/default_logconf.json` 配置）：

```bash
ls /unitree/var/log/cloth_service
```

# ❓ 常见问题

1. 服务报错退出 `Serial device 1a86:55d3 not found`：
   - 传感服未连接或线路接触不良。可用 `lsusb`（应能看到 `1a86:55d3`）和 `ls /dev/ttyACM*` 检查。
2. CMake 报错 `local unitree_sdk2 not found`：
   - `3rd/unitree_sdk2` 目录缺失或不完整，请恢复仓库自带的 SDK。
3. 在另一台机器上 `JqDdsTest` 收不到数据：
   - 确认两台机器处于同一网段，并将正确的网卡名（如 `eth0`）作为第一个参数传入。
