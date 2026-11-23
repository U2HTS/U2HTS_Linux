# U2HTS_Linux
Linux gadget port of [U2HTS](https://github.com/U2HTS/U2HTS).  
Tested on Raspberry Pi 4.  
[zh_CN(简体中文)](./README_zh.md)

# Build

```bash
sudo apt install pkg-config libusbgx-dev libconfig-dev
git clone https://github.com/U2HTS/U2HTS_Linux.git --recursive --depth 1
cd U2HTS_Linux
cmake --preset Release
cmake --build build
```

# Usage

Note:

- A machine with USB device (gadget/UDC) capability is required.
- The kernel must support reporting HID feature reports.

Load the `libcomposite` module:

```bash
sudo modprobe libcomposite
```

If you are using a Raspberry Pi, you will also need to add the `dwc2` device tree overlay:

```bash
sudo dtoverlay dwc2
```

Run:

```bash
sudo ./build/U2HTS_Linux
```

# Notes

No bus transports are currently implemented, so the `linux-multitouch` is the only driver supported.

This driver automatically scans for multi-touch devices in the system and use them as touch data source.