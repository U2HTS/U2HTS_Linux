# U2HTS_Linux
[U2HTS](https://github.com/U2HTS/U2HTS) 的 Linux gadget 移植版。
在树莓派 4 上测试通过。

# 构建

```bash
sudo apt install pkg-config libusbgx-dev libconfig-dev
git clone https://github.com/U2HTS/U2HTS_Linux.git --recursive --depth 1
cd U2HTS_Linux
cmake --preset Release
cmake --build build
```

# 使用
注意：  
- 需要选择带 USB 从机（gadget/UDC）功能的机器。
- 内核必须支持上报 HID feature report。

先加载`libcomposite`模块：
```bash
sudo modprobe libcomposite
```
如果使用树莓派，则还需要添加dwc2设备树插件：
```bash
sudo dtoverlay dwc2
```
运行：
```bash
sudo ./build/U2HTS_Linux
```

# 注意事项

当前没有实现任何总线传输，所以只能使用 `linux-multitouch` 驱动。  
该驱动会自动扫描系统中的多点触摸设备，并将其作为触摸数据源。  
命令行参数如下：
```bash
用法: U2HTS_Linux [选项]
  -d, --dev <path>         多点触摸设备的绝对路径
      --vid <vid>          USB 制造商 ID (默认: 0x1d6b)
      --pid <pid>          USB 产品 ID (默认: 0x8572)
      --serial <str>       USB 序列号 (默认: 读取 /etc/machine-id)
      --configfs <path>    configfs 挂载点 ( 默认: /sys/kernel/config)
      --invert-x           翻转 X 轴
      --invert-y           翻转 Y 轴
      --swap-x-y           交换 XY 轴
      --max-tps <n>        限制触摸点为 n
  -h, --help               显示此帮助
```