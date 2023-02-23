#include <openbmc/hgx.h>
#include <openbmc/libgpio.h>
#include <iostream>
#include <syslog.h>
#include "fw-util.h"
#include <openbmc/pal.h>

const char * GPU_PRESENT = "GPU_HMC_PRSNT_ISO_R_N";

class HGXComponent : public Component {
  std::string _hgxcomp{};

 public:
  HGXComponent(
      const std::string& fru,
      const std::string& comp,
      const std::string& hgxcomp = "")
      : Component(fru, comp), _hgxcomp(hgxcomp) {}
  int update(std::string image) {
    try {
      if (component() == "patch") {
        hgx::patch_bf_update();
      } else {
        if (_hgxcomp == "") {
          syslog(LOG_CRIT, "HGX FW upgrade initiated, image: %s", image.c_str());
        }
        else {
          syslog(LOG_CRIT, "HGX Component %s upgrade initiated", component().c_str());
        }

        if (hgx::update(_hgxcomp, image) == 0) {
          syslog(LOG_CRIT, "HGX upgrade completed");
          sleep(3);
          if (hgx::getHMCPhase() != BMC_FW_DVT) {
            hgx::factoryReset();
            return 0;
          }
        }
      }
    } catch (std::exception& e) {
      std::cerr << e.what() << std::endl;
      syslog(LOG_CRIT, "HGX upgrade failed: %s", e.what());
    }

    return 0;
  }

  int get_version(json& j) {
    if (_hgxcomp == "") {
      return FW_STATUS_NOT_SUPPORTED;
    }
    try {
      j["VERSION"] = hgx::version(_hgxcomp);
    } catch (std::exception& e) {
      std::cerr << e.what() << std::endl;
      return FW_STATUS_FAILURE;
    }
    return FW_STATUS_SUCCESS;
  }
};

class HGXSystemConfig {
  static bool isEVT() {
    try {
      hgx::version("HMC_Firmware");
      return true;
    } catch (std::exception&) {
      return false;
    }
  }
  public:
    HGXSystemConfig() {
      if (pal_is_artemis()) {
        return;
      }
      static HGXComponent autocomp("hgx", "auto");
      static HGXComponent patchcomp("hgx", "patch", "");
      
      if (gpio_get_value_by_shadow(GPU_PRESENT) == 0) {
        if (isEVT()) {
          static HGXComponent hmc("hgx", "hmc", "HMC_Firmware");
          static HGXComponent gpu0("hgx", "gpu0-fw", "GPU0Firmware");
          static HGXComponent gpu1("hgx", "gpu1-fw", "GPU1Firmware");
          static HGXComponent gpu2("hgx", "gpu2-fw", "GPU2Firmware");
          static HGXComponent gpu3("hgx", "gpu3-fw", "GPU3Firmware");
          static HGXComponent gpu4("hgx", "gpu4-fw", "GPU4Firmware");
          static HGXComponent gpu5("hgx", "gpu5-fw", "GPU5Firmware");
          static HGXComponent gpu6("hgx", "gpu6-fw", "GPU6Firmware");
          static HGXComponent gpu7("hgx", "gpu7-fw", "GPU7Firmware");
          static HGXComponent gpuinfo0("hgx", "gpu0-inforom", "GPU0InfoROM");
          static HGXComponent gpuinfo1("hgx", "gpu1-inforom", "GPU1InfoROM");
          static HGXComponent gpuinfo2("hgx", "gpu2-inforom", "GPU2InfoROM");
          static HGXComponent gpuinfo3("hgx", "gpu3-inforom", "GPU3InfoROM");
          static HGXComponent gpuinfo4("hgx", "gpu4-inforom", "GPU4InfoROM");
          static HGXComponent gpuinfo5("hgx", "gpu5-inforom", "GPU5InfoROM");
          static HGXComponent gpuinfo6("hgx", "gpu6-inforom", "GPU6InfoROM");
          static HGXComponent gpuinfo7("hgx", "gpu7-inforom", "GPU7InfoROM");
          static HGXComponent fpga("hgx", "fpga", "FPGAFirmware");
          static HGXComponent nvsw0("hgx", "nvswitch0-fw", "NVSwitch0Firmware");
          static HGXComponent nvsw1("hgx", "nvswitch1-fw", "NVSwitch1Firmware");
          static HGXComponent nvsw2("hgx", "nvswitch2-fw", "NVSwitch2Firmware");
          static HGXComponent nvsw3("hgx", "nvswitch3-fw", "NVSwitch3Firmware");
          static HGXComponent nvswinfo0("hgx", "nvswitch0-inforom", "NVSwitch0InfoROM");
          static HGXComponent nvswinfo1("hgx", "nvswitch1-inforom", "NVSwitch1InfoROM");
          static HGXComponent nvswinfo2("hgx", "nvswitch2-inforom", "NVSwitch2InfoROM");
          static HGXComponent nvswinfo3("hgx", "nvswitch3-inforom", "NVSwitch3InfoROM");
          static HGXComponent pretimer0("hgx", "pcieretimer0", "PCIeRetimer0Firmware");
          static HGXComponent pretimer1("hgx", "pcieretimer1", "PCIeRetimer1Firmware");
          static HGXComponent pretimer2("hgx", "pcieretimer2", "PCIeRetimer2Firmware");
          static HGXComponent pretimer3("hgx", "pcieretimer3", "PCIeRetimer3Firmware");
          static HGXComponent pretimer4("hgx", "pcieretimer4", "PCIeRetimer4Firmware");
          static HGXComponent pretimer5("hgx", "pcieretimer5", "PCIeRetimer5Firmware");
          static HGXComponent pretimer6("hgx", "pcieretimer6", "PCIeRetimer6Firmware");
          static HGXComponent pretimer7("hgx", "pcieretimer7", "PCIeRetimer7Firmware");
          static HGXComponent pex0("hgx", "pcie-switch0", "PCIeSwitch0Firmware");
          static HGXComponent efpga("hgx", "erot-fpga", "ERoT_FPGA_Firmware");
          static HGXComponent egpu0("hgx", "erot-gpu0", "ERoT_GPU0_Firmware");
          static HGXComponent egpu1("hgx", "erot-gpu1", "ERoT_GPU1_Firmware");
          static HGXComponent egpu2("hgx", "erot-gpu2", "ERoT_GPU2_Firmware");
          static HGXComponent egpu3("hgx", "erot-gpu3", "ERoT_GPU3_Firmware");
          static HGXComponent egpu4("hgx", "erot-gpu4", "ERoT_GPU4_Firmware");
          static HGXComponent egpu5("hgx", "erot-gpu5", "ERoT_GPU5_Firmware");
          static HGXComponent egpu6("hgx", "erot-gpu6", "ERoT_GPU6_Firmware");
          static HGXComponent egpu7("hgx", "erot-gpu7", "ERoT_GPU7_Firmware");
          static HGXComponent ehmc("hgx", "erot-hmc", "ERoT_HMC_Firmware");
          static HGXComponent envsw0("hgx", "erot-nvswitch0", "ERoT_NVSwitch0_Firmware");
          static HGXComponent envsw1("hgx", "erot-nvswitch1", "ERoT_NVSwitch1_Firmware");
          static HGXComponent envsw2("hgx", "erot-nvswitch2", "ERoT_NVSwitch2_Firmware");
          static HGXComponent envsw3("hgx", "erot-nvswitch3", "ERoT_NVSwitch3_Firmware");
          static HGXComponent epax("hgx", "erot-pax", "ERoT_PCIeSwitch_Firmware");
        } else {
          // Newer HMC uses "BMC" as the component name
          bool isHMC;
          try {
            hgx::version("HGX_FW_HMC_0");
            isHMC = true;
          } catch (std::exception&) {
            isHMC = false;
          };
          if (isHMC) {
            static HGXComponent hmc("hgx", "hmc", "HGX_FW_HMC_0");
          } else {
            static HGXComponent hmc("hgx", "hmc", "HGX_FW_BMC_0");
          }
          static HGXComponent gpu0("hgx", "gpu1-fw", "HGX_FW_GPU_SXM_1");
          static HGXComponent gpu1("hgx", "gpu2-fw", "HGX_FW_GPU_SXM_2");
          static HGXComponent gpu2("hgx", "gpu3-fw", "HGX_FW_GPU_SXM_3");
          static HGXComponent gpu3("hgx", "gpu4-fw", "HGX_FW_GPU_SXM_4");
          static HGXComponent gpu4("hgx", "gpu5-fw", "HGX_FW_GPU_SXM_5");
          static HGXComponent gpu5("hgx", "gpu6-fw", "HGX_FW_GPU_SXM_6");
          static HGXComponent gpu6("hgx", "gpu7-fw", "HGX_FW_GPU_SXM_7");
          static HGXComponent gpu7("hgx", "gpu8-fw", "HGX_FW_GPU_SXM_8");
          static HGXComponent gpuinfo0("hgx", "gpu1-inforom", "HGX_InfoROM_GPU_SXM_1");
          static HGXComponent gpuinfo1("hgx", "gpu2-inforom", "HGX_InfoROM_GPU_SXM_2");
          static HGXComponent gpuinfo2("hgx", "gpu3-inforom", "HGX_InfoROM_GPU_SXM_3");
          static HGXComponent gpuinfo3("hgx", "gpu4-inforom", "HGX_InfoROM_GPU_SXM_4");
          static HGXComponent gpuinfo4("hgx", "gpu5-inforom", "HGX_InfoROM_GPU_SXM_5");
          static HGXComponent gpuinfo5("hgx", "gpu6-inforom", "HGX_InfoROM_GPU_SXM_6");
          static HGXComponent gpuinfo6("hgx", "gpu7-inforom", "HGX_InfoROM_GPU_SXM_7");
          static HGXComponent gpuinfo7("hgx", "gpu8-inforom", "HGX_InfoROM_GPU_SXM_8");
          static HGXComponent fpga("hgx", "fpga", "HGX_FW_FPGA_0");
          static HGXComponent nvsw0("hgx", "nvswitch0-fw", "HGX_FW_NVSwitch_0");
          static HGXComponent nvsw1("hgx", "nvswitch1-fw", "HGX_FW_NVSwitch_1");
          static HGXComponent nvsw2("hgx", "nvswitch2-fw", "HGX_FW_NVSwitch_2");
          static HGXComponent nvsw3("hgx", "nvswitch3-fw", "HGX_FW_NVSwitch_3");
          static HGXComponent nvswinfo0("hgx", "nvswitch0-inforom", "HGX_InfoROM_NVSwitch_0");
          static HGXComponent nvswinfo1("hgx", "nvswitch1-inforom", "HGX_InfoROM_NVSwitch_1");
          static HGXComponent nvswinfo2("hgx", "nvswitch2-inforom", "HGX_InfoROM_NVSwitch_2");
          static HGXComponent nvswinfo3("hgx", "nvswitch3-inforom", "HGX_InfoROM_NVSwitch_3");

          static HGXComponent pretimer0("hgx", "pcieretimer0", "HGX_FW_PCIeRetimer_0");
          static HGXComponent pretimer1("hgx", "pcieretimer1", "HGX_FW_PCIeRetimer_1");
          static HGXComponent pretimer2("hgx", "pcieretimer2", "HGX_FW_PCIeRetimer_2");
          static HGXComponent pretimer3("hgx", "pcieretimer3", "HGX_FW_PCIeRetimer_3");
          static HGXComponent pretimer4("hgx", "pcieretimer4", "HGX_FW_PCIeRetimer_4");
          static HGXComponent pretimer5("hgx", "pcieretimer5", "HGX_FW_PCIeRetimer_5");
          static HGXComponent pretimer6("hgx", "pcieretimer6", "HGX_FW_PCIeRetimer_6");
          static HGXComponent pretimer7("hgx", "pcieretimer7", "HGX_FW_PCIeRetimer_7");
          static HGXComponent pex0("hgx", "pcie-switch0", "HGX_FW_PCIeSwitch_0");
          static HGXComponent efpga("hgx", "erot-fpga", "HGX_FW_ERoT_FPGA_0");
          static HGXComponent egpu0("hgx", "erot-gpu1", "HGX_FW_ERoT_GPU_SXM_1");
          static HGXComponent egpu1("hgx", "erot-gpu2", "HGX_FW_ERoT_GPU_SXM_2");
          static HGXComponent egpu2("hgx", "erot-gpu3", "HGX_FW_ERoT_GPU_SXM_3");
          static HGXComponent egpu3("hgx", "erot-gpu4", "HGX_FW_ERoT_GPU_SXM_4");
          static HGXComponent egpu4("hgx", "erot-gpu5", "HGX_FW_ERoT_GPU_SXM_5");
          static HGXComponent egpu5("hgx", "erot-gpu6", "HGX_FW_ERoT_GPU_SXM_6");
          static HGXComponent egpu6("hgx", "erot-gpu7", "HGX_FW_ERoT_GPU_SXM_7");
          static HGXComponent egpu7("hgx", "erot-gpu8", "HGX_FW_ERoT_GPU_SXM_8");
          if (isHMC) {
            static HGXComponent ehmc("hgx", "erot-hmc", "HGX_FW_ERoT_HMC_0");
          } else {
            static HGXComponent ehmc("hgx", "erot-hmc", "HGX_FW_ERoT_BMC_0");
          }
          static HGXComponent envsw0("hgx", "erot-nvswitch0", "HGX_FW_ERoT_NVSwitch_0");
          static HGXComponent envsw1("hgx", "erot-nvswitch1", "HGX_FW_ERoT_NVSwitch_1");
          static HGXComponent envsw2("hgx", "erot-nvswitch2", "HGX_FW_ERoT_NVSwitch_2");
          static HGXComponent envsw3("hgx", "erot-nvswitch3", "HGX_FW_ERoT_NVSwitch_3");
          static HGXComponent epax("hgx", "erot-pax", "HGX_FW_ERoT_PCIeSwitch_0");
        }
      }
    }
};

HGXSystemConfig hgxConfig;
