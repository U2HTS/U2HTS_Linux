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
          "  -d, --dev <path>         absolute path of input device (default: "
          "auto)\n"
          "      --vid <vid>          USB vendor ID (default: 0x1d6b)\n"
          "      --pid <pid>          USB product ID (default: 0x8572)\n"
          "      --serial <str>       USB serial string (default: "
          "read /etc/machine-id)\n"
          "      --configfs <path>    configfs mount point (default: "
          "/sys/kernel/config)\n"
          "      --invert-x           Invert X axis\n"
          "      --invert-y           Invert Y axis\n"
          "      --swap-x-y           Swap X and Y axis\n"
          "      --max-tps <n>        Limit touch points by n\n"
          "  -h, --help               show this help\n",
          prog);
}

int main(int argc, char** argv) {
  ULOGI("u2hts_linux", "U2HTS_Linux " U2HTS_LINUX_GIT_SHA " built @ %s %s",
        __DATE__, __TIME__);
  const char* serial = NULL;
  const char* configfs_path = "/sys/kernel/config";
  uint16_t vid = 0x1d6b;
  uint16_t pid = U2HTS_USB_PID;
  char custom_config[U2HTS_CUSTOM_CONFIG_STR_MAX_TOTAL_LENGTH] = {0};
  bool x_invert = false, y_invert = false, x_y_swap = false;
  uint8_t max_tps = 0;

  enum {
    OPT_VID = 0x100,
    OPT_PID,
    OPT_SERIAL,
    OPT_CONFIGFS,
    OPT_INVERT_X,
    OPT_INVERT_Y,
    OPT_SWAP_X_Y,
    OPT_MAX_TPS
  };

  static const struct option long_opts[] = {
      {"dev", required_argument, NULL, 'd'},
      {"vid", required_argument, NULL, OPT_VID},
      {"pid", required_argument, NULL, OPT_PID},
      {"serial", required_argument, NULL, OPT_SERIAL},
      {"configfs", required_argument, NULL, OPT_CONFIGFS},
      {"invert-x", no_argument, NULL, OPT_INVERT_X},
      {"invert-y", no_argument, NULL, OPT_INVERT_Y},
      {"swap-x-y", no_argument, NULL, OPT_SWAP_X_Y},
      {"max-tps", required_argument, NULL, OPT_MAX_TPS},
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
        unsigned long v = strtoul(optarg, (char**)NULL, 0);
        if (v > 0xFFFF) {
          fprintf(stderr, "invalid vid: %s\n", optarg);
          return EXIT_FAILURE;
        }
        vid = (uint16_t)v;
        break;
      }
      case OPT_PID: {
        unsigned long v = strtoul(optarg, (char**)NULL, 0);
        if (v > 0xFFFF) {
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
      case OPT_INVERT_X:
        x_invert = true;
        break;
      case OPT_INVERT_Y:
        y_invert = true;
        break;
      case OPT_SWAP_X_Y:
        x_y_swap = true;
        break;
      case OPT_MAX_TPS:
        max_tps = strtol(optarg, (char**)NULL, 10);
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
      .coord_config = {.x_invert = x_invert,
                       .y_invert = y_invert,
                       .x_y_swap = x_y_swap,
                       .max_tps = max_tps},
      .polling_mode = true,
      .custom_controller_config = custom_config[0] ? custom_config : NULL,
  };

  ret = u2hts_init(&cfg);
  if (ret) exit(EXIT_FAILURE);

  while (run) u2hts_task();

  return EXIT_SUCCESS;
}
