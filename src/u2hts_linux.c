#include "u2hts_linux.h"

#include "u2hts_core.h"

#define LOG_TAG "u2hts_linux"

static usbg_gadget* gadget_dev;
static usbg_state* lib_state;
static usbg_function* hid_func;

static int usb_dev_fd = 0;

static char usb_serial[64] = "0123456789";

static const u2hts_linux_usb_config* usb_cfg;

static void u2hts_linux_load_serial() {
  FILE* fp = fopen("/etc/machine-id", "r");
  if (!fp) return;
  if (fgets(usb_serial, sizeof(usb_serial), fp))
    usb_serial[strcspn(usb_serial, "\r\n")] = '\0';
  fclose(fp);
  if (!usb_serial[0]) strcpy(usb_serial, "0123456789");
}

void u2hts_delay_ms(uint32_t ms) {
  struct timespec ts = {
      .tv_nsec = (ms % 1000) * 1000000,
      .tv_sec = ms / 1000,
  };
  nanosleep(&ts, NULL);
}

void u2hts_delay_us(uint32_t us) {
  U2HTS_UNUSED(us);
  u2hts_delay_ms(1);
}

uint16_t u2hts_get_timestamp() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint16_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void u2hts_i2c_init(uint32_t speed_hz) { U2HTS_UNUSED(speed_hz); }

void u2hts_i2c_set_speed(uint32_t speed_hz) { U2HTS_UNUSED(speed_hz); }

bool u2hts_i2c_write(uint8_t slave_addr, void* buf, size_t len, bool stop) {
  U2HTS_UNUSED(slave_addr);
  U2HTS_UNUSED(buf);
  U2HTS_UNUSED(len);
  U2HTS_UNUSED(stop);
}

bool u2hts_i2c_read(uint8_t slave_addr, void* buf, size_t len) {
  U2HTS_UNUSED(slave_addr);
  U2HTS_UNUSED(buf);
  U2HTS_UNUSED(len);
}

bool u2hts_i2c_detect_slave(uint8_t addr) { U2HTS_UNUSED(addr); }

void u2hts_spi_init(u2hts_spi_config* config) { U2HTS_UNUSED(config); }

bool u2hts_spi_transfer(void* buf, size_t len) {
  U2HTS_UNUSED(buf);
  U2HTS_UNUSED(len);
}

void u2hts_tpint_set_mode(bool mode, bool pull) {
  U2HTS_UNUSED(mode);
  U2HTS_UNUSED(pull);
}

void u2hts_tpint_set(bool value) { U2HTS_UNUSED(value); }

bool u2hts_tpint_get() { return false; }

void u2hts_tprst_set(bool value) { U2HTS_UNUSED(value); }

void u2hts_int_init(U2HTS_INT_FLAGS int_flag) { U2HTS_UNUSED(int_flag); }

void u2hts_int_set(bool enable) { U2HTS_UNUSED(enable); }

const static uint8_t u2hts_hid_report_descriptor[] = {
    U2HTS_HID_REPORT_TP_DESC(U2HTS_HID_REPORT_TP_ID),
    U2HTS_HID_REPORT_TP, /* Touch Point 1  */
    U2HTS_HID_REPORT_TP, /* Touch Point 2  */
    U2HTS_HID_REPORT_TP, /* Touch Point 3  */
    U2HTS_HID_REPORT_TP, /* Touch Point 4  */
    U2HTS_HID_REPORT_TP, /* Touch Point 5  */
    U2HTS_HID_REPORT_TP, /* Touch Point 6  */
    U2HTS_HID_REPORT_TP, /* Touch Point 7  */
    U2HTS_HID_REPORT_TP, /* Touch Point 8  */
    U2HTS_HID_REPORT_TP, /* Touch Point 9  */
    U2HTS_HID_REPORT_TP, /* Touch Point 10 */
    U2HTS_HID_REPORT_TP_INFO,
    U2HTS_HID_REPORT_TP_MAX_COUNT_DESC(U2HTS_HID_REPORT_TP_MAX_COUNT_ID),
    U2HTS_HID_REPORT_DESCRIPTOR_END};

void u2hts_linux_usb_set_config(const u2hts_linux_usb_config* cfg) {
  usb_cfg = cfg;
}

static bool u2hts_linux_usb_create() {
  usbg_config* c;
  int usbg_ret;

  if (usb_cfg->serial && usb_cfg->serial[0]) {
    strncpy(usb_serial, usb_cfg->serial, sizeof(usb_serial) - 1);
    usb_serial[sizeof(usb_serial) - 1] = '\0';
  } else {
    u2hts_linux_load_serial();
  }

  struct usbg_gadget_attrs g_attrs = {
      .bcdUSB = 0x0200,
      .bDeviceClass = USB_CLASS_PER_INTERFACE,
      .bDeviceSubClass = 0x00,
      .bDeviceProtocol = 0x00,
      .bMaxPacketSize0 = 64, /* Max allowed ep0 packet size */
      .idVendor = usb_cfg->vid,
      .idProduct = usb_cfg->pid,
      .bcdDevice = 0x0001,
  };

  struct usbg_gadget_strs g_strs = {.serial = usb_serial,
                                    .manufacturer = U2HTS_MANU_STR,
                                    .product = U2HTS_PROD_STR};

  struct usbg_config_strs c_strs = {.configuration = "hid"};

  struct usbg_f_hid_attrs f_attrs = {
      .protocol = 1,
      .report_desc =
          {
              .desc = (char*)u2hts_hid_report_descriptor,
              .len = sizeof(u2hts_hid_report_descriptor),
          },
      .report_length = sizeof(u2hts_hid_report),
      .subclass = 0,
  };

  usbg_ret = usbg_init(usb_cfg->configfs_path, &lib_state);
  if (usbg_ret != USBG_SUCCESS) {
    ULOGE(LOG_TAG, "Error on usbg init: %s : %s", usbg_error_name(usbg_ret),
          usbg_strerror(usbg_ret));
    return false;
  }

  usbg_ret =
      usbg_create_gadget(lib_state, "u2hts", &g_attrs, &g_strs, &gadget_dev);
  if (usbg_ret != USBG_SUCCESS) {
    ULOGE(LOG_TAG, "Error creating gadget: %s : %s", usbg_error_name(usbg_ret),
          usbg_strerror(usbg_ret));
    return false;
  }
  usbg_ret =
      usbg_create_function(gadget_dev, USBG_F_HID, "hid", &f_attrs, &hid_func);
  if (usbg_ret != USBG_SUCCESS) {
    ULOGE(LOG_TAG, "Error creating function: %s : %s",
          usbg_error_name(usbg_ret), usbg_strerror(usbg_ret));
    return false;
  }

  usbg_ret = usbg_create_config(gadget_dev, 1, "cfg", NULL, &c_strs, &c);
  if (usbg_ret != USBG_SUCCESS) {
    ULOGE(LOG_TAG, "Error creating config: %s : %s", usbg_error_name(usbg_ret),
          usbg_strerror(usbg_ret));
    return false;
  }

  usbg_ret = usbg_add_config_function(c, "hid", hid_func);
  if (usbg_ret != USBG_SUCCESS) {
    ULOGE(LOG_TAG, "Error adding function: %s : %s", usbg_error_name(usbg_ret),
          usbg_strerror(usbg_ret));
    return false;
  }

  return true;
}

void u2hts_linux_usb_release() {
  int ret = 0;
  if (gadget_dev) {
    usbg_disable_gadget(gadget_dev);
    ret = usbg_rm_gadget(gadget_dev, USBG_RM_RECURSE);
    if (ret != USBG_SUCCESS)
      ULOGE(LOG_TAG, "rm gadget: %s", usbg_strerror(ret));
  }
  if (lib_state) usbg_cleanup(lib_state);
  if (usb_dev_fd >= 0) close(usb_dev_fd);
  usb_dev_fd = -1;
  lib_state = NULL;
  gadget_dev = NULL;
  hid_func = NULL;
}

void u2hts_usb_init() {
  if (!u2hts_linux_usb_create()) exit(EXIT_FAILURE);

  int usbg_ret = usbg_enable_gadget(gadget_dev, DEFAULT_UDC);
  if (usbg_ret != USBG_SUCCESS) {
    ULOGE(LOG_TAG, "Error enabling gadget: %s : %s",
          usbg_error_name(usbg_ret), usbg_strerror(usbg_ret));
    exit(EXIT_FAILURE);
  }
  // save gadget fd.
  usbg_f_hid* hf = usbg_to_hid_function(hid_func);
  if (!hf) {
    ULOGW(LOG_TAG, "Failed to get hid func: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  dev_t dev;
  int ret = usbg_f_hid_get_dev(hf, &dev);
  if (ret != USBG_SUCCESS) {
    ULOGE(LOG_TAG, "get_dev: %s", usbg_strerror(ret));
    exit(EXIT_FAILURE);
  }

  char path[64];
  snprintf(path, sizeof(path), "/dev/hidg%u", minor(dev));
  usb_dev_fd = open(path, O_WRONLY | O_NONBLOCK);
  if (usb_dev_fd < 0) {
    ULOGE(LOG_TAG, "open %s: %s", path, strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Answer the Contact Count Maximum feature report (Report ID 2). hidg asks
  // userspace for GET_REPORT responses through this ioctl and returns an
  // empty report after a 2.5 s timeout otherwise.
  struct usb_hidg_report max_count_report = {
      .report_id = U2HTS_HID_REPORT_TP_MAX_COUNT_ID,
      .userspace_req = 0,
      .length = 2,
      .data = {U2HTS_HID_REPORT_TP_MAX_COUNT_ID, u2hts_get_max_tps()},
  };
  if (ioctl(usb_dev_fd, GADGET_HID_WRITE_GET_REPORT, &max_count_report) < 0)
    ULOGE(LOG_TAG, "set max count feature report: %s", strerror(errno));
}

void u2hts_usb_hid_report(const u2hts_hid_report* report) {
  // blocking write
  write(usb_dev_fd, report, sizeof(u2hts_hid_report));
}

bool u2hts_get_usb_status() { return true; }