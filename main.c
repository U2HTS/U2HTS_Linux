#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "u2hts_linux.h"

static volatile sig_atomic_t run = 1;

static void sig_handler(int signo) {
  (void)signo;
  run = 0;
}

static void usage(const char* prog) {
  fprintf(stderr,
          "Usage: %s [options]\n"
          "  -d, --dev <path>         mulittouch input device abs path\n"
          "      --vid <vid>          USB vendor ID (default: 0x1d6b)\n"
          "      --pid <pid>          USB product ID (default: 0x8572)\n"
          "      --serial <str>       USB serial string (default: "
          "read /etc/machine-id)\n"
          "      --configfs <path>    configfs mount point (default: "
          "/sys/kernel/config)\n"
          "  -h, --help               show this help\n",
          prog);
}

int main(int argc, char** argv) {
  const char* serial = NULL;
  const char* configfs_path = "/sys/kernel/config";
  uint16_t vid = 0x1d6b;
  uint16_t pid = U2HTS_USB_PID;
  char custom_config[U2HTS_CUSTOM_CONFIG_STR_MAX_TOTAL_LENGTH] = {0};

  enum { OPT_VID = 0x100, OPT_PID, OPT_SERIAL, OPT_CONFIGFS };

  static const struct option long_opts[] = {
      {"dev", required_argument, NULL, 'd'},
      {"vid", required_argument, NULL, OPT_VID},
      {"pid", required_argument, NULL, OPT_PID},
      {"serial", required_argument, NULL, OPT_SERIAL},
      {"configfs", required_argument, NULL, OPT_CONFIGFS},
      {"help", no_argument, NULL, 'h'},
      {NULL, 0, NULL, 0},
  };

  int opt;
  while ((opt = getopt_long(argc, argv, "d:h", long_opts, NULL)) != -1) {
    switch (opt) {
      case 'd':
        snprintf(custom_config, sizeof(custom_config), "linux-mt.dev=%s",
                 optarg);
        break;
      case OPT_VID: {
        char* end = NULL;
        unsigned long v = strtoul(optarg, &end, 0);
        if (!end || *end || v > 0xFFFF) {
          fprintf(stderr, "invalid vid: %s\n", optarg);
          return EXIT_FAILURE;
        }
        vid = (uint16_t)v;
        break;
      }
      case OPT_PID: {
        char* end = NULL;
        unsigned long v = strtoul(optarg, &end, 0);
        if (!end || *end || v > 0xFFFF) {
          fprintf(stderr, "invalid pid: %s\n", optarg);
          return EXIT_FAILURE;
        }
        pid = (uint16_t)v;
        break;
      }
      case OPT_SERIAL:
        serial = optarg;
        break;
      case OPT_CONFIGFS:
        configfs_path = optarg;
        break;
      case 'h':
        usage(argv[0]);
        return EXIT_SUCCESS;
      default:
        usage(argv[0]);
        return EXIT_FAILURE;
    }
  }

  if (optind < argc) {
    fprintf(stderr, "unexpected argument: %s\n", argv[optind]);
    return EXIT_FAILURE;
  }

  atexit(u2hts_linux_usb_release);
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  const u2hts_linux_usb_config usb_cfg = {
      .vid = vid,
      .pid = pid,
      .serial = serial,
      .configfs_path = configfs_path,
  };

  bool ret = u2hts_linux_usb_init(&usb_cfg);
  if (!ret) exit(EXIT_FAILURE);

  const u2hts_config cfg = {
      .controller = "linux-multitouch",
      .polling_mode = true,
      .custom_controller_config = custom_config[0] ? custom_config : NULL,
  };

  ret = u2hts_init(&cfg);
  if (ret) exit(EXIT_FAILURE);

  while (run) u2hts_task();

  return EXIT_SUCCESS;
}
