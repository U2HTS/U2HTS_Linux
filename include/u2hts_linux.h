#ifndef _U2HTS_LINUX_H_
#define _U2HTS_LINUX_H_
#include <usbg/usbg.h>
#include "u2hts_core.h"
#include <usbg/function/hid.h>
#include <linux/usb/ch9.h>
#include <linux/usb/g_hid.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/sysmacros.h>
#include <time.h>
#include <unistd.h>

typedef struct {
  uint16_t vid;
  uint16_t pid;
  const char* serial;
  const char* configfs_path;
} u2hts_linux_usb_config;

void u2hts_linux_usb_set_config(const u2hts_linux_usb_config* cfg);

void u2hts_linux_usb_release();

#endif