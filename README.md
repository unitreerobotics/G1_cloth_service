<div align="center">
  <h1 align="center">
    <a href="https://www.unitree.com/" target="_blank">G1 Cloth Service</a>
  </h1>
  <a href="https://www.unitree.com/" target="_blank">
    <img src="https://www.unitree.com/images/0079f8938336436e955ea3a98c4e1e59.svg" alt="Unitree LOGO" width="15%">
  </a>
  <p align="center">
    <a> English </a> | <a href="README_zh-CN.md">中文</a> </a>
  </p>
</div>

# 0. 📖 Introduction

This is a serial2dds service for the G1 tactile sensing cloth.

The sensing cloth is connected through a CH343 USB-to-serial converter (VID:PID `1a86:55d3`, device node `/dev/ttyACM*`) at 921600 baud. The service reads and merges the serial frames (256 sensor values + gyro quaternion) and publishes them over DDS at 100 Hz.

```mermaid
graph LR
A(sensing cloth) -- serial /dev/ttyACMx @921600 --> B(cloth_service)
B(cloth_service) -- rt/jq_clothstate --> C(user)
```

DDS message layout (topic `rt/jq_clothstate`, type `sensor_msgs::msg::dds_::PointCloud2_`, only the `data` byte array is used):

| Bytes       | Content                                        |
| ----------- | ---------------------------------------------- |
| `data[0..15]`   | gyro quaternion, 4 float32 (w, x, y, z), little-endian |
| `data[16..271]` | 256 sensor values, 8 bits each                 |

# 1. 📦 Installation

```bash
sudo apt update
sudo apt install libserialport-dev
cd ~
git clone https://github.com/unitreerobotics/G1_cloth_service
cd G1_cloth_service
bash build.sh
```

The build script uses the system default compiler (C++17, CMake >= 3.10) and automatically selects the SDK libraries matching the host architecture (`uname -m`). The unitree_sdk2 (DDS) dependency is vendored in `3rd/unitree_sdk2` (prebuilt for `x86_64` and `aarch64`). Executables are generated in `bin/`:

- `bin/cloth_service` — the service
- `bin/JqDdsTest` — DDS subscriber test tool

> 💡 Note: The `3rd/serialport` directory provides offline packages for `sudo apt install libserialport-dev` (Ubuntu 22.04), for both `arm64` (Jetson) and `amd64` (x86_64) machines. If you have trouble installing `libserialport`, you can try installing these deb packages. Install `libserialport0` first, then `libserialport-dev`:
> ```bash
> # arm64 (NVIDIA Jetson Orin NX)
> sudo dpkg -i 3rd/serialport/libserialport0_0.1.1-3_arm64.deb
> sudo dpkg -i 3rd/serialport/libserialport-dev_0.1.1-3_arm64.deb
>
> # amd64 / x86_64 PC
> sudo dpkg -i 3rd/serialport/libserialport0_0.1.1-3_amd64.deb
> sudo dpkg -i 3rd/serialport/libserialport-dev_0.1.1-3_amd64.deb
> ```

# 2. 🚀 Launch

```bash
# start the service (serial port access requires root or the dialout group)
sudo ./bin/cloth_service
```

On startup the service waits up to 30 seconds for the USB device to be enumerated, then opens the port and starts publishing.

Run the test subscriber to verify the data:

```bash
# subscribe via loopback (default) or a specific network interface
./bin/JqDdsTest
./bin/JqDdsTest eth0
```

It prints the receive rate, the gyro quaternion and the 256 sensor values.

# 3. 🚀🚀🚀 Automatic Startup Service

After building and successfully testing `cloth_service`, you can configure it to start automatically on system boot:

```bash
bash setup_autostart.sh
```

The script installs `G1_cloth.service` (systemd), which starts `bin/cloth_service` on boot. Manage the service with:

```bash
sudo systemctl status G1_cloth.service      # check status
sudo journalctl -u G1_cloth.service -f      # view logs in real-time
sudo systemctl restart G1_cloth.service     # restart service
sudo systemctl stop G1_cloth.service        # stop service
sudo systemctl disable G1_cloth.service     # disable auto-start on boot
```

Besides `journalctl`, you can also view historical log files (10 MB × 3 async rotation, configured in `config/default_logconf.json`) under `/unitree/var/log/cloth_service`:

```bash
ls /unitree/var/log/cloth_service
```

# ❓ FAQ

1. The service exits with `Serial device 1a86:55d3 not found`:
   - The sensing cloth is not connected or the cable is loose. Check with `lsusb` (look for `1a86:55d3`) and `ls /dev/ttyACM*`.
2. CMake fails with `local unitree_sdk2 not found`:
   - The `3rd/unitree_sdk2` directory is missing or incomplete. Restore the vendored SDK.
3. `JqDdsTest` receives nothing on another machine:
   - Make sure both machines are on the same subnet and pass the correct network interface name (e.g. `eth0`) as the first argument.
