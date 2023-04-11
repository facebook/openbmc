#if defined CONFIG_POSTCODE_AMD

#include "postcode-amd.h"

void
pal_parse_amd_sec_pei_dxe(uint32_t post_code, char *str )
{
  switch (post_code){
    case 0x00:
      strcat(str, "Not used");
      break;
    case 0x01:
      strcat(str, "Power on. Reset type detection (soft/hard)");
      break;
    case 0x02:
      strcat(str, "AP initialization before microcode loading");
      break;
    case 0x03:
      strcat(str, "North Bridge initialization before microcode loading");
      break;
    case 0x04:
      strcat(str, "South Bridge initialization before microcode loading");
      break;
    case 0x05:
      strcat(str, "OEM initialization before microcode loading");
      break;
    case 0x06:
      strcat(str, "Microcode loading");
      break;
    case 0x07:
      strcat(str, "AP initialization after microcode loading");
      break;
    case 0x08:
      strcat(str, "North Bridge initialization after microcode loading");
      break;
    case 0x09:
      strcat(str, "South Bridge initialization after microcode loading");
      break;
    case 0x0A:
      strcat(str, "OEM initialization after microcode loading");
      break;
    case 0x0B:
      strcat(str, "Cache initialization");
      break;
    case 0x0C:
    case 0x0D:
      strcat(str, "Reserved for future AMI SEC error codes");
      break;
    case 0x0E:
      strcat(str, "Microcode not found");
      break;
    case 0x0F:
      strcat(str, "Microcode not loaded");
      break;
    case 0x10:
      strcat(str, "PEI Core is started");
      break;
    case 0x11:
      strcat(str, "Pre-memory CPU initialization is started");
      break;
    case 0x12:
    case 0x13:
    case 0x14:
      strcat(str, "Pre-memory CPU initialization (CPU module specific)");
      break;
    case 0x15:
      strcat(str, "Pre-memory North Bridge initialization is started");
      break;
    case 0x16:
    case 0x17:
    case 0x18:
      strcat(str, "Pre-Memory North Bridge initialization (North Bridge module specific)");
      break;
    case 0x19:
      strcat(str, "Pre-memory South Bridge initialization is started");
      break;
    case 0x1A:
    case 0x1B:
    case 0x1C:
      strcat(str, "Pre-memory South Bridge initialization (South Bridge module specific)");
      break;
    case 0x1D:
    case 0x1E:
    case 0x1F:
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2A:
      strcat(str, "OEM pre-memory initialization codes");
      break;
    case 0x2B:
      strcat(str, "Memory initialization. Serial Presence Detect (SPD) data reading");
      break;
    case 0x2C:
      strcat(str, "Memory initialization. Memory presence detection");
      break;
    case 0x2D:
      strcat(str, "Memory initialization. Programming memory timing information");
      break;
    case 0x2E:
      strcat(str, "Memory initialization. Configuring memory");
      break;
    case 0x2F:
      strcat(str, "Memory initialization (other)");
      break;
    case 0x30:
      strcat(str, "Reserved for ASL (see ASL Status Codes section below)");
      break;
    case 0x31:
      strcat(str, "Memory Installed");
      break;
    case 0x32:
      strcat(str, "CPU post-memory initialization is started");
      break;
    case 0x33:
      strcat(str, "CPU post-memory initialization. Cache initialization");
      break;
    case 0x34:
      strcat(str, "CPU post-memory initialization. Application Processor(s) (AP) initialization");
      break;
    case 0x35:
      strcat(str, "CPU post-memory initialization. Boot Strap Processor (BSP) selection");
      break;
    case 0x36:
      strcat(str, "CPU post-memory initialization. System Management Mode (SMM) initialization");
      break;
    case 0x37:
      strcat(str, "Post-Memory North Bridge initialization is started");
      break;
    case 0x38:
    case 0x39:
    case 0x3A:
      strcat(str, "Post-Memory North Bridge initialization (North Bridge module specific)");
      break;
    case 0x3B:
      strcat(str, "Post-Memory South Bridge initialization is started");
      break;
    case 0x3C:
    case 0x3D:
    case 0x3E:
      strcat(str, "Post-Memory South Bridge initialization (South Bridge module specific)");
      break;
    case 0x3F:
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
    case 0x44:
    case 0x45:
    case 0x46:
    case 0x47:
    case 0x48:
    case 0x49:
    case 0x4A:
    case 0x4B:
    case 0x4C:
    case 0x4D:
    case 0x4E:
      strcat(str, "OEM post memory initialization codes");
      break;
    case 0x4F:
      strcat(str, "DXE IPL is started");
      break;
    case 0x50:
      strcat(str, "Memory initialization error. Invalid memory type or incompatible memory speed");
      break;
    case 0x51:
      strcat(str, "Memory initialization error. SPD reading has failed");
      break;
    case 0x52:
      strcat(str, "Memory initialization error. Invalid memory size or memory modules do not match");
      break;
    case 0x53:
      strcat(str, "Memory initialization error. No usable memory detected");
      break;
    case 0x54:
      strcat(str, "Unspecified memory initialization error");
      break;
    case 0x55:
      strcat(str, "Memory not installed");
      break;
    case 0x56:
      strcat(str, "Invalid CPU type or Speed");
      break;
    case 0x57:
      strcat(str, "CPU mismatch");
      break;
    case 0x58:
      strcat(str, "CPU self test failed or possible CPU cache error");
      break;
    case 0x59:
      strcat(str, "CPU micro-code is not found or micro-code update is failed");
      break;
    case 0x5A:
      strcat(str, "Internal CPU error");
      break;
    case 0x5B:
      strcat(str, "reset PPI is not available");
      break;
    case 0x5C:
      strcat(str, "PEI phase BMC self-test failure");
      break;
    case 0x5D:
    case 0x5E:
    case 0x5F:
      strcat(str, "Reserved for future AMI error codes");
      break;
    case 0x60:
      strcat(str, "DXE Core is started");
      break;
    case 0x61:
      strcat(str, "NVRAM initialization");
      break;
    case 0x62:
      strcat(str, "Installation of the South Bridge Runtime Services");
      break;
    case 0x63:
      strcat(str, "CPU DXE initialization is started");
      break;
    case 0x64:
    case 0x65:
    case 0x66:
    case 0x67:
      strcat(str, "CPU DXE initialization (CPU module specific)");
      break;
    case 0x68:
      strcat(str, "PCI host bridge initialization");
      break;
    case 0x69:
      strcat(str, "North Bridge DXE initialization is started");
      break;
    case 0x6A:
      strcat(str, "North Bridge DXE SMM initialization is started");
      break;
    case 0x6B:
    case 0x6C:
    case 0x6D:
    case 0x6E:
    case 0x6F:
      strcat(str, "North Bridge DXE initialization (North Bridge module specific)");
      break;
    case 0x70:
      strcat(str, "South Bridge DXE initialization is started");
      break;
    case 0x71:
      strcat(str, "South Bridge DXE SMM initialization is started");
      break;
    case 0x72:
      strcat(str, "South Bridge devices initialization");
      break;
    case 0x73:
    case 0x74:
    case 0x75:
    case 0x76:
    case 0x77:
      strcat(str, "South Bridge DXE Initialization (South Bridge module specific)");
      break;
    case 0x78:
      strcat(str, "ACPI module initialization");
      break;
    case 0x79:
      strcat(str, "CSM initialization");
      break;
    case 0x7A:
    case 0x7B:
    case 0x7C:
    case 0x7D:
    case 0x7E:
    case 0x7F:
      strcat(str, "Reserved for future AMI DXE codes");
      break;
    case 0x80:
    case 0x81:
    case 0x82:
    case 0x83:
    case 0x84:
    case 0x85:
    case 0x86:
    case 0x87:
    case 0x88:
    case 0x89:
    case 0x8A:
    case 0x8B:
    case 0x8C:
    case 0x8D:
    case 0x8E:
    case 0x8F:
      strcat(str, "OEM DXE initialization codes");
      break;
    case 0x90:
      strcat(str, "Boot Device Selection (BDS) phase is started");
      break;
    case 0x91:
      strcat(str, "Driver connecting is started");
      break;
    case 0x92:
      strcat(str, "PCI Bus initialization is started");
      break;
    case 0x93:
      strcat(str, "PCI Bus Hot Plug Controller Initialization");
      break;
    case 0x94:
      strcat(str, "PCI Bus Enumeration");
      break;
    case 0x95:
      strcat(str, "PCI Bus Request Resources");
      break;
    case 0x96:
      strcat(str, "PCI Bus Assign Resources");
      break;
    case 0x97:
      strcat(str, "Console Output devices connect");
      break;
    case 0x98:
      strcat(str, "Console input devices connect");
      break;
    case 0x99:
      strcat(str, "Super IO Initialization");
      break;
    case 0x9A:
      strcat(str, "USB initialization is started");
      break;
    case 0x9B:
      strcat(str, "USB Reset");
      break;
    case 0x9C:
      strcat(str, "USB Detect");
      break;
    case 0x9D:
      strcat(str, "USB Enable");
      break;
    case 0x9E:
    case 0x9F:
      strcat(str, "Reserved for future AMI codes");
      break;
    case 0xA0:
      strcat(str, "IDE initialization is started");
      break;
    case 0xA1:
      strcat(str, "IDE Reset");
      break;
    case 0xA2:
      strcat(str, "IDE Detect");
      break;
    case 0xA3:
      strcat(str, "IDE Enable");
      break;
    case 0xA4:
      strcat(str, "SCSI initialization is started");
      break;
    case 0xA5:
      strcat(str, "SCSI Reset");
      break;
    case 0xA6:
      strcat(str, "SCSI Detect");
      break;
    case 0xA7:
      strcat(str, "SCSI Enable");
      break;
    case 0xA8:
      strcat(str, "Setup Verifying Password");
      break;
    case 0xA9:
      strcat(str, "Start of Setup");
      break;
    case 0xAA:
      strcat(str, "Reserved for ASL (see ASL Status Codes section below)");
      break;
    case 0xAB:
      strcat(str, "Setup Input Wait");
      break;
    case 0xAC:
      strcat(str, "Reserved for ASL (see ASL Status Codes section below)");
      break;
    case 0xAD:
      strcat(str, "Ready To Boot event");
      break;
    case 0xAE:
      strcat(str, "Legacy Boot event");
      break;
    case 0xAF:
      strcat(str, "Exit Boot Services event");
      break;
    case 0xB0:
      strcat(str, "Runtime Set Virtual Address MAP Begin");
      break;
    case 0xB1:
      strcat(str, "Runtime Set Virtual Address MAP End");
      break;
    case 0xB2:
      strcat(str, "Legacy Option ROM Initialization");
      break;
    case 0xB3:
      strcat(str, "System Reset");
      break;
    case 0xB4:
      strcat(str, "USB hot plug");
      break;
    case 0xB5:
      strcat(str, "PCI bus hot plug");
      break;
    case 0xB6:
      strcat(str, "Clean-up of NVRAM");
      break;
    case 0xB7:
      strcat(str, "Configuration Reset (reset of NVRAM settings)");
      break;
    case 0xB8:
    case 0xB9:
    case 0xBA:
    case 0xBB:
    case 0xBC:
    case 0xBD:
    case 0xBE:
    case 0xBF:
      strcat(str, "Reserved for future AMI codes");
      break;
    case 0xC0:
    case 0xC1:
    case 0xC2:
    case 0xC3:
    case 0xC4:
    case 0xC5:
    case 0xC6:
    case 0xC7:
    case 0xC8:
    case 0xC9:
    case 0xCA:
    case 0xCB:
    case 0xCC:
    case 0xCD:
    case 0xCE:
    case 0xCF:
      strcat(str, "OEM BDS initialization codes");
      break;
    case 0xD0:
      strcat(str, "CPU initialization error");
      break;
    case 0xD1:
      strcat(str, "North Bridge initialization error");
      break;
    case 0xD2:
      strcat(str, "South Bridge initialization error");
      break;
    case 0xD3:
      strcat(str, "Some of the Architectural Protocols are not available");
      break;
    case 0xD4:
      strcat(str, "PCI resource allocation error. Out of Resources");
      break;
    case 0xD5:
      strcat(str, "No Space for Legacy Option ROM");
      break;
    case 0xD6:
      strcat(str, "No Console Output Devices are found");
      break;
    case 0xD7:
      strcat(str, "No Console Input Devices are found");
      break;
    case 0xD8:
      strcat(str, "Invalid password");
      break;
    case 0xD9:
      strcat(str, "Error loading Boot Option (LoadImage returned error)");
      break;
    case 0xDA:
      strcat(str, "Boot Option is failed (StartImage returned error)");
      break;
    case 0xDB:
      strcat(str, "Flash update is failed");
      break;
    case 0xDC:
      strcat(str, "Reset protocol is not available");
      break;
    case 0xDD:
      strcat(str, "DXE phase BMC self-test failure");
      break;
    case 0xE0:
      strcat(str, "S3 Resume is stared (S3 Resume PPI is called by the DXE IPL)");
      break;
    case 0xE1:
      strcat(str, "S3 Boot Script execution");
      break;
    case 0xE2:
      strcat(str, "Video repost");
      break;
    case 0xE3:
      strcat(str, "OS S3 wake vector call");
      break;
    case 0xE4:
    case 0xE5:
    case 0xE6:
    case 0xE7:
      strcat(str, "Reserved for future AMI progress codes");
      break;
    case 0xE8:
      strcat(str, "S3 Resume Failed");
      break;
    case 0xE9:
      strcat(str, "S3 Resume PPI not Found");
      break;
    case 0xEA:
      strcat(str, "S3 Resume Boot Script Error");
      break;
    case 0xEB:
      strcat(str, "S3 OS Wake Error");
      break;
    case 0xEC:
    case 0xED:
    case 0xEE:
    case 0xEF:
      strcat(str, "Reserved for future AMI error codes");
      break;
    case 0xF0:
      strcat(str, "Recovery condition triggered by firmware (Auto recovery)");
      break;
    case 0XF1:
      strcat(str, "Recovery condition triggered by user (Forced recovery)");
      break;
    case 0xF2:
      strcat(str, "Recovery process started");
      break;
    case 0xF3:
      strcat(str, "Recovery firmware image is found");
      break;
    case 0xF4:
      strcat(str, "Recovery firmware image is loaded");
      break;
    case 0xF5:
    case 0xF6:
    case 0xF7:
      strcat(str, "Reserved for future AMI progress codes");
      break;
    case 0xF8:
      strcat(str, "Recovery PPI is not available");
      break;
    case 0xFA:
      strcat(str, "Invalid recovery capsule");
      break;
    default:
      strcat(str, "Reserved for future AMI error codes");
  }
}

void
pal_parse_amd_agesa(uint32_t post_code, char *str )
{
  switch (post_code){
    case 0xA001:
      strcat(str, "Universal ACPI entry");
      break;
    case 0xA002:
      strcat(str, "Universal ACPI exit");
      break;
    case 0xA003:
      strcat(str, "Universal ACPI abort");
      break;
    case 0xA004:
      strcat(str, "Universal SMBIOS entry");
      break;
    case 0xA005:
      strcat(str, "Universal SMBIOS exit");
      break;
    case 0xA006:
      strcat(str, "Universal SMBIOS abort");
      break;
    case 0xA101:
      strcat(str, "Memory structure initialization");
      break;
    case 0xA102:
      strcat(str, "SPD Data processing");
      break;
    case 0xA103:
      strcat(str, "Memory configuration");
      break;
    case 0xA104:
      strcat(str, "DRAM initialization");
      break;
    case 0xA105:
      strcat(str, "TpProcMemSPDChecking");
      break;
    case 0xA106:
      strcat(str, "TpProcMemModeChecking");
      break;
    case 0xA107:
      strcat(str, "Speed and TCL configuration");
      break;
    case 0xA108:
      strcat(str, "TpProcMemSpdTiming");
      break;
    case 0xA109:
      strcat(str, "TpProcMemDramMapping");
      break;
    case 0xA10A:
      strcat(str, "TpProcMemPlatformSpecificConfig");
      break;
    case 0xA10B:
      strcat(str, "TPProcMemPhyCompensation");
      break;
    case 0xA10C:
      strcat(str, "TpProcMemStartDcts");
      break;
    case 0xA10D:
      strcat(str, "Public interface");
      break;
    case 0xA10E:
      strcat(str, "TpProcMemPhyFenceTraining");
      break;
    case 0xA10F:
      strcat(str, "TpProcMemSynchronizeDcts");
      break;
    case 0xA110:
      strcat(str, "TpProcMemSystemMemoryMapping");
      break;
    case 0xA111:
      strcat(str, "TpProcMemMtrrConfiguration");
      break;
    case 0xA112:
      strcat(str, "TpProcMemDramTraining");
      break;
    case 0xA113:
      strcat(str, "Public interface");
      break;
    case 0xA114:
      strcat(str, "TpProcMemWriteLevelizationTraining");
      break;
    case 0xA115:
      strcat(str, "Below 800Mhz first pass start");
      break;
    case 0xA116:
      strcat(str, "Above 800Mhz second pass start");
      break;
    case 0xA117:
      strcat(str, "Target DIMM configured");
      break;
    case 0xA118:
      strcat(str, "Prepare DIMMS for WL");
      break;
    case 0xA119:
      strcat(str, "Configure DIMMS for WL");
      break;
    case 0xA11A:
      strcat(str, "TpProcMemReceiverEnableTraining");
      break;
    case 0xA11B:
      strcat(str, "Start sweep loop");
      break;
    case 0xA11C:
      strcat(str, "Set receiver Delay");
      break;
    case 0xA11D:
      strcat(str, "Write test pattern");
      break;
    case 0xA11E:
      strcat(str, "Read test pattern");
      break;
    case 0xA11F:
      strcat(str, "Compare test pattern");
      break;
    case 0xA120:
      strcat(str, "Calculate MaxRdLatency per channel");
      break;
    case 0xA121:
      strcat(str, "TpProcMemReceiveDqsTraining");
      break;
    case 0xA122:
      strcat(str, "Set Write Data delay");
      break;
    case 0xA123:
      strcat(str, "Write test pattern");
      break;
    case 0xA124:
      strcat(str, "Start read sweep");
      break;
    case 0xA125:
      strcat(str, "Set Receive DQS delay");
      break;
    case 0xA126:
      strcat(str, "Read Test pattern");
      break;
    case 0xA127:
      strcat(str, "Compare Test pattern");
      break;
    case 0xA128:
      strcat(str, "Update results");
      break;
    case 0xA129:
      strcat(str, "Start Find passing window");
      break;
    case 0xA12A:
      strcat(str, "TpProcMemTransmitDqsTraining");
      break;
    case 0xA12B:
      strcat(str, "Start write sweep");
      break;
    case 0xA12C:
      strcat(str, "Set Transmit DQ delay");
      break;
    case 0xA12D:
      strcat(str, "Write test pattern");
      break;
    case 0xA12E:
      strcat(str, "Read Test pattern");
      break;
    case 0xA12F:
      strcat(str, "Compare Test pattern");
      break;
    case 0xA130:
      strcat(str, "Update results");
      break;
    case 0xA131:
      strcat(str, "Start Find passing window");
      break;
    case 0xA132:
      strcat(str, "TpProcMemMaxRdLatencyTraining");
      break;
    case 0xA133:
      strcat(str, "Start sweep");
      break;
    case 0xA134:
      strcat(str, "Set delay");
      break;
    case 0xA135:
      strcat(str, "Write test pattern");
      break;
    case 0xA136:
      strcat(str, "Read Test pattern");
      break;
    case 0xA137:
      strcat(str, "Compare Test pattern");
      break;
    case 0xA138:
      strcat(str, "Online Spare init");
      break;
    case 0xA139:
      strcat(str, "Bank Interleave Init");
      break;
    case 0xA13A:
      strcat(str, "Node Interleave Init");
      break;
    case 0xA13B:
      strcat(str, "Channel Interleave Init");
      break;
    case 0xA13C:
      strcat(str, "ECC initialization");
      break;
    case 0xA13D:
      strcat(str, "Platform Specific Init");
      break;
    case 0xA13E:
      strcat(str, "Before callout for AgesaReadSpd");
      break;
    case 0xA13F:
      strcat(str, "After callout for AgesaReadSpd");
      break;
    case 0xA140:
      strcat(str, "Before optional callout AgesaHookBeforeDramInit");
      break;
    case 0xA141:
      strcat(str, "After optional callout AgesaHookBeforeDramInit");
      break;
    case 0xA142:
      strcat(str, "Before optional callout AgesaHookBeforeDQSTraining");
      break;
    case 0xA143:
      strcat(str, "After optional callout AgesaHookBeforeDQSTraining");
      break;
    case 0xA144:
      strcat(str, "Before optional callout AgesaHookBeforeDramInit");
      break;
    case 0xA145:
      strcat(str, "After optional callout AgesaHookBeforeDramInit");
      break;
    case 0xA146:
      strcat(str, "After MemDataInit");
      break;
    case 0xA147:
      strcat(str, "Before InitializeMCT");
      break;
    case 0xA148:
      strcat(str, "Before LV DDR3");
      break;
    case 0xA149:
      strcat(str, "Before InitMCT");
      break;
    case 0xA14A:
      strcat(str, "Before OtherTiming");
      break;
    case 0xA14B:
      strcat(str, "Before UMAMemTyping");
      break;
    case 0xA14C:
      strcat(str, "Before SetDqsEccTmgs");
      break;
    case 0xA14D:
      strcat(str, "Before MemClr");
      break;
    case 0xA14E:
      strcat(str, "Before On DIMM Thermal");
      break;
    case 0xA14F:
      strcat(str, "Before DMI");
      break;
    case 0xA150:
      strcat(str, "End of memory code");
      break;
    case 0xA151:
      strcat(str, "Entry point S3Init");
      break;
    case 0xA180:
      strcat(str, "Sending MRS2");
      break;
    case 0xA181:
      strcat(str, "Sedding MRS3");
      break;
    case 0xA182:
      strcat(str, "Sending MRS1");
      break;
    case 0xA183:
      strcat(str, "Sending MRS0");
      break;
    case 0xA184:
      strcat(str, "Continuous Pattern Read");
      break;
    case 0xA185:
      strcat(str, "Continuous Pattern Write");
      break;
    case 0xA186:
      strcat(str, "Mem: 2d RdDqs Training begin");
      break;
    case 0xA187:
      strcat(str, "Mem: Before optional callout to platform BIOS to change External Vref during 2d Training");
      break;
    case 0xA188:
      strcat(str, "Mem: After optional callout to platform BIOS to change External Vref during 2d Training");
      break;
    case 0xA189:
      strcat(str, "Configure DCT For General use begin");
      break;
    case 0xA18A:
      strcat(str, "Configure DCT For training begin");
      break;
    case 0xA18B:
      strcat(str, "Configure DCT For Non-Explicit");
      break;
    case 0xA18C:
      strcat(str, "Configure to Sync channels");
      break;
    case 0xA18D:
      strcat(str, "Allocate C6 Storage");
      break;
    case 0xA18E:
      strcat(str, "Before LV DDR4");
      break;
    case 0xA190:
      strcat(str, "BR before AP launch");
      break;
    case 0xA191:
      strcat(str, "Install AP launched PPI");
      break;
    case 0xA192:
      strcat(str, "BR after AP launch");
      break;
    case 0xA193:
      strcat(str, "Before CPU PM");
      break;
    case 0xA194:
      strcat(str, "Enable IO Cstate");
      break;
    case 0xA195:
      strcat(str, "Enable C6");
      break;
    case 0xA196:
      strcat(str, "Install CCX PEI complete PPI");
      break;
    case 0xA197:
      strcat(str, "BR CPU memory done call back entry");
      break;
    case 0xA198:
      strcat(str, "Before APM weights");
      break;
    case 0xA199:
      strcat(str, "After APM weights");
      break;
    case 0xA19A:
      strcat(str, "BR CPU memory done call back end");
      break;
    case 0xA19B:
      strcat(str, "BR Init Mid entry");
      break;
    case 0xA19C:
      strcat(str, "BR enable APM");
      break;
    case 0xA19D:
      strcat(str, "BR Init Mid install protocol");
      break;
    case 0xA19E:
      strcat(str, "BR Init Mid end");
      break;
    case 0xA19F:
      strcat(str, "BR Init Late entry");
      break;
    case 0xA1A0:
      strcat(str, "BR Init Late install protocol");
      break;
    case 0xA1A1:
      strcat(str, "BR Init Late end");
      break;
    case 0xA1A2:
      strcat(str, "BR DXE install complete protocol");
      break;
    case 0xA1A3:
      strcat(str, "UNB install complete PPI");
      break;
    case 0xA1A4:
      strcat(str, "UNB AfterApLaunch callback entry");
      break;
    case 0xA1A5:
      strcat(str, "UNB AfterApLaunch callback end");
      break;
    case 0xA1EC:
      strcat(str, "Before the S3 save code calls out to allocate a buffer");
      break;
    case 0xA1ED:
      strcat(str, "After the S3 save code calls out to allocate a buffer");
      break;
    case 0xA1EE:
      strcat(str, "Before the memory S3 save code calls out to allocate a buffer");
      break;
    case 0xA1EF:
      strcat(str, "After the memory S3 save code calls out to allocate a buffer");
      break;
    case 0xA1F0:
      strcat(str, "Before the memory code calls out to locate a buffer");
      break;
    case 0xA1F1:
      strcat(str, "After the memory code calls out to locate a buffer");
      break;
    case 0xA1F2:
      strcat(str, "Before the memory code calls out to locate a buffer");
      break;
    case 0xA1F3:
      strcat(str, "After the memory code calls out to locate a buffer");
      break;
    case 0xA1F4:
      strcat(str, "Before the memory code calls out to locate a buffer");
      break;
    case 0xA1F5:
      strcat(str, "After the memory code calls out to locate a buffer");
      break;
    case 0xA1F6:
      strcat(str, "Before the memory code calls out to locate a buffer");
      break;
    case 0xA1F7:
      strcat(str, "After the memory code calls out to locate a buffer");
      break;
    case 0xA1F9:
      strcat(str, "Failed PMU training.");
      break;
    case 0xA501:
      strcat(str, "PspPeiV1 entry");
      break;
    case 0xA502:
      strcat(str, "PspPeiV1 exit");
      break;
    case 0xA503:
      strcat(str, "MemoryDiscoveredPpiCallback entry");
      break;
    case 0xA504:
      strcat(str, "MemoryDiscoveredPpiCallback exit");
      break;
    case 0xA507:
      strcat(str, "PspDxeV1 entry");
      break;
    case 0xA508:
      strcat(str, "PspDxeV1 exit");
      break;
    case 0xA50A:
      strcat(str, "PspDxeV1 PspPciEnumerationCompleteCallBack entry");
      break;
    case 0xA50B:
      strcat(str, "PspDxeV1 PspPciEnumerationCompleteCallBack exit");
      break;
    case 0xA50C:
      strcat(str, "PspDxeV1 ready to boot entry");
      break;
    case 0xA50D:
      strcat(str, "PspDxeV1 ready to boot exit");
      break;
    case 0xA50E:
      strcat(str, "PspSmmV1 entry");
      break;
    case 0xA50F:
      strcat(str, "PspSmmV1 exit");
      break;
    case 0xA510:
      strcat(str, "PspSmmV1 SwSmiCallBack entry");
      break;
    case 0xA511:
      strcat(str, "PspSmmV1 SwSmiCallBack exit");
      break;
    case 0xA512:
      strcat(str, "PspSmmV1 BspSmmResumeVector entry");
      break;
    case 0xA513:
      strcat(str, "PspSmmV1 BspSmmResumeVector exit");
      break;
    case 0xA514:
      strcat(str, "PspSmmV1 ApSmmResumeVector entry");
      break;
    case 0xA515:
      strcat(str, "PspSmmV1 ApSmmResumeVector exit");
      break;
    case 0xA516:
      strcat(str, "PspP2CmboxV1 entry");
      break;
    case 0xA517:
      strcat(str, "PspP2CmboxV1 exit");
      break;
    case 0xA521:
      strcat(str, "PspPeiV2 entry");
      break;
    case 0xA522:
      strcat(str, "PspPeiV2 exit");
      break;
    case 0xA523:
      strcat(str, "PspDxeV2 entry");
      break;
    case 0xA524:
      strcat(str, "PspDxeV2 exit");
      break;
    case 0xA525:
      strcat(str, "PspDxeV2 PspMpServiceCallBack entry");
      break;
    case 0xA526:
      strcat(str, "PspDxeV2 PspMpServiceCallBack exit");
      break;
    case 0xA527:
      strcat(str, "PspDxeV2 FlashAccCallBack entry");
      break;
    case 0xA528:
      strcat(str, "PspDxeV2 FlashAccCallBack exit");
      break;
    case 0xA529:
      strcat(str, "PspDxeV2 ready to boot entry");
      break;
    case 0xA52A:
      strcat(str, "PspDxeV2 ready to boot exit");
      break;
    case 0xA52B:
      strcat(str, "PspDxeV2 exit boot serivce entry");
      break;
    case 0xA52C:
      strcat(str, "PspDxeV2 exit boot serivce exit");
      break;
    case 0xA52D:
      strcat(str, "PspSmmV2 entry");
      break;
    case 0xA52E:
      strcat(str, "PspSmmV2 exit");
      break;
    case 0xA52F:
      strcat(str, "PspSmmV2 SwSmiCallBack entry");
      break;
    case 0xA530:
      strcat(str, "PspSmmV2 SwSmiCallBack exit");
      break;
    case 0xA531:
      strcat(str, "PspSmmV2 BspSmmResumeVector entry");
      break;
    case 0xA532:
      strcat(str, "PspSmmV2 BspSmmResumeVector exit");
      break;
    case 0xA533:
      strcat(str, "PspSmmV2 ApSmmResumeVector entry");
      break;
    case 0xA534:
      strcat(str, "PspSmmV2 ApSmmResumeVector exit");
      break;
    case 0xA535:
      strcat(str, "PspP2CmboxV2 entry");
      break;
    case 0xA536:
      strcat(str, "PspP2CmboxV2 exit");
      break;
    case 0xA537:
      strcat(str, "TpPspRecoverApcbFail");
      break;
    case 0xA539:
      strcat(str, "PspDxeV2 ApcbAccCallBack entry");
      break;
    case 0xA53A:
      strcat(str, "PspDxeV2 ApcbAccCallBack exit");
      break;
    case 0xA53B:
      strcat(str, "Enable Rd Instruction Fail");
      break;
    case 0xA53C:
      strcat(str, "ScpcAutoEnablement failed due a variable with larger size detected");
      break;
    case 0xA53D:
      strcat(str, "ScpcAutoEnablement failed due SetVariabl");
      break;
    case 0xA53E:
      strcat(str, "ScpcAutoEnablement failed due delete variable");
      break;
    case 0xA53F:
      strcat(str, "C2P mailbox status field return with error, non-zero value");
      break;
    case 0xA540:
      strcat(str, "PspfTpmPei entry");
      break;
    case 0xA541:
      strcat(str, "PspfTpmPei exit");
      break;
    case 0xA542:
      strcat(str, "PspfTpmPei memory callback entry");
      break;
    case 0xA543:
      strcat(str, "PspfTpmPei memory callback exit");
      break;
    case 0xA544:
      strcat(str, "PspfTpmDxe entry");
      break;
    case 0xA545:
      strcat(str, "PspfTpmDxe exit");
      break;
    case 0xA546:
      strcat(str, "PspdTpmPei entry");
      break;
    case 0xA547:
      strcat(str, "PspdTpmPei exit");
      break;
    case 0xA548:
      strcat(str, "HspfTpmPei entry");
      break;
    case 0xA549:
      strcat(str, "HspfTpmPei exit");
      break;
    case 0xA54A:
      strcat(str, "HspfTpmPei memory callback entry");
      break;
    case 0xA54B:
      strcat(str, "HspfTpmPei memory callback exit");
      break;
    case 0xA54C:
      strcat(str, "HspfTpmDxe entry");
      break;
    case 0xA54D:
      strcat(str, "HspfTpmDxe exit");
      break;
    case 0xA560:
      strcat(str, "PspIntrusionDetectionPei Entry");
      break;
    case 0xA561:
      strcat(str, "PspIntrusionDetectionPei Exit");
      break;
    case 0xA562:
      strcat(str, "PspIntrusionDetectionDxe Entry");
      break;
    case 0xA563:
      strcat(str, "PspIntrusionDetectionDxe Exit");
      break;
    case 0xA564:
      strcat(str, "Psp DoIntrusionDetectionAction in Dxe");
      break;
    case 0xA565:
      strcat(str, "Psp IntrusionDetection Clear Tpm");
      break;
    case 0xA566:
      strcat(str, "PspIntrusionDetectionSmm Entry");
      break;
    case 0xA567:
      strcat(str, "PspIntrusionDetectionSmm Exit");
      break;
    case 0xA568:
      strcat(str, "Psp IntrusionDetectionSmiCallback");
      break;
    case 0xA569:
      strcat(str, "Psp DoIntrusionDetectionAction in Smm");
      break;
    case 0xA56E:
      strcat(str, "Psp Intrusion Event Logging Error");
      break;
    case 0xA56F:
      strcat(str, "Psp Intrusion Get Event Log Error");
      break;
    case 0xA570:
      strcat(str, "Psp has got Async Cmd from the mailbox, but the routine is still running now");
      break;
    case 0xA591:
      strcat(str, "PspP2Cmbox Command SpiGetAttrib Handling entry");
      break;
    case 0xA592:
      strcat(str, "PspP2Cmbox Command SpiSetAttrib Handling entry");
      break;
    case 0xA593:
      strcat(str, "PspP2Cmbox Command SpiGetBlockSize Handling entry");
      break;
    case 0xA594:
      strcat(str, "PspP2Cmbox Command SpiReadFV Handling entry");
      break;
    case 0xA595:
      strcat(str, "PspP2Cmbox Command SpiWriteFV Handling entry");
      break;
    case 0xA596:
      strcat(str, "PspP2Cmbox Command SpiEraseFV Handling entry");
      break;
    case 0xA597:
      strcat(str, "PspP2Cmbox Command MboxPspCmdRpmcIncMc entry");
      break;
    case 0xA598:
      strcat(str, "PspP2Cmbox Command TpMboxPspCmdRpmcReqMc entry");
      break;
    case 0xA599:
      strcat(str, "PspP2Cmbox Command TpMboxPspCmdArsStatus entry");
      break;
    case 0xA59A:
      strcat(str, "PspP2Cmbox Command TpMboxPspCmdArsStatus entry");
      break;
    case 0xA59B:
      strcat(str, "PspP2Cmbox Command TpMboxPspCmdRtPprDone entry");
      break;
    case 0xA59E:
      strcat(str, "PspP2Cmbox Command Handling exit");
      break;
    case 0xA59F:
      strcat(str, "PspP2Cmbox Command Handling Fail exit");
      break;
    case 0xA5F0:
      strcat(str, "PspfTpm send TPM command entry");
      break;
    case 0xA5F1:
      strcat(str, "PspfTpm send TPM command exit");
      break;
    case 0xA5F2:
      strcat(str, "PspfTpm receive TPM command entry");
      break;
    case 0xA5F3:
      strcat(str, "PspfTpm receive TPM command exit");
      break;
    case 0xA5F4:
      strcat(str, "HspfTpm send TPM command entry");
      break;
    case 0xA5F5:
      strcat(str, "HspfTpm send TPM command exit");
      break;
    case 0xA5F6:
      strcat(str, "HspfTpm receive TPM command entry");
      break;
    case 0xA5F7:
      strcat(str, "HspfTpm receive TPM command exit");
      break;
    case 0xA600:
      strcat(str, "PSP C2P mailbox entry base");
      break;
    case 0xA601:
      strcat(str, "Before send C2P command MboxBiosCmdDramInfo");
      break;
    case 0xA602:
      strcat(str, "Before send C2P command MboxBiosCmdSmmInfo");
      break;
    case 0xA603:
      strcat(str, "Before send C2P command MboxBiosCmdSleep SxInfo");
      break;
    case 0xA604:
      strcat(str, "Before send C2P command MboxBiosCmdRsmInfo");
      break;
    case 0xA605:
      strcat(str, "Before send C2P command MboxBiosCmdFtpmQuery");
      break;
    case 0xA606:
      strcat(str, "Before send C2P command MboxBiosCmdBootDone");
      break;
    case 0xA607:
      strcat(str, "Before send C2P command MboxBiosCmdClearS3Sts");
      break;
    case 0xA608:
      strcat(str, "Before send C2P command MboxBiosCmdS3DataInfo");
      break;
    case 0xA609:
      strcat(str, "Before send C2P command MboxBiosCmdNop");
      break;
    case 0xA614:
      strcat(str, "Before send C2P command MboxBiosCmdHSTIQuery");
      break;
    case 0xA617:
      strcat(str, "Before send C2P command MboxBiosCmdClrSmmLock");
      break;
    case 0xA618:
      strcat(str, "Before send C2P command MboxBiosCmdPcieInfo");
      break;
    case 0xA619:
      strcat(str, "Before send C2P command MboxBiosCmdGetVersion");
      break;
    case 0xA61B:
      strcat(str, "Before send C2P command MboxBiosCmdLockDFReg");
      break;
    case 0xA61C:
      strcat(str, "Before send C2P command MboxBiosCmdClrSmmLock");
      break;
    case 0xA61D:
      strcat(str, "Before send C2P command MboxBiosCmdSetApCsBase");
      break;
    case 0xA61E:
      strcat(str, "Before send C2P command MboxBiosCmdKvmInfo");
      break;
    case 0xA61F:
      strcat(str, "Before send C2P command MboxBiosCmdLockSpi");
      break;
    case 0xA620:
      strcat(str, "Before send C2P command MboxBiosCmdScreenOnGpio");
      break;
    case 0xA621:
      strcat(str, "Before send C2P command MboxBiosCmdSpiOpWhiteList");
      break;
    case 0xA622:
      strcat(str, "Before send C2P command MboxBiosCmdRasEinj");
      break;
    case 0xA624:
      strcat(str, "Before send C2P command MboxBiosCmdStartArs");
      break;
    case 0xA625:
      strcat(str, "Before send C2P command MboxBiosCmdStopArs");
      break;
    case 0xA626:
      strcat(str, "Before send C2P command MboxBiosCmdSetBootPartitionId");
      break;
    case 0xA627:
      strcat(str, "Before send C2P command MboxBiosCmdPspCapsQuery");
      break;
    case 0xA628:
      strcat(str, "Before send C2P command MboxBiosCmdArmorEnterSmmOnlyMode");
      break;
    case 0xA629:
      strcat(str, "Before send C2P command MboxBiosCmdArmorEnforceWhitelist");
      break;
    case 0xA62A:
      strcat(str, "Before send C2P command MboxBiosCmdArmorExecuteSpiCommand");
      break;
    case 0xA62B:
      strcat(str, "Before send C2P command MboxBiosCmdArmorSwitchCsMode");
      break;
    case 0xA62C:
      strcat(str, "Before send C2P command MboxBiosCmdDrtmInfoId");
      break;
    case 0xA62D:
      strcat(str, "Before send C2P command MboxBiosCmdLaterSplFuse");
      break;
    case 0xA62E:
      strcat(str, "Before send C2P command MboxBiosCmdDtpmInfo");
      break;
    case 0xA62F:
      strcat(str, "Before send C2P command MboxBiosCmdValidateManOsSignature");
      break;
    case 0xA630:
      strcat(str, "Before send C2P command MboxBiosCmdLockFCHReg");
      break;
    case 0xA631:
      strcat(str, "Before send C2P command MboxBiosCmdPostDrtmInfoQuery");
      break;
    case 0xA634:
      strcat(str, "Before send C2P command MboxBiosCmdSignValidateHmacDataPreSmm");
      break;
    case 0xA635:
      strcat(str, "Before send C2P command MboxBiosCmdSignValidateHmacDataSmm");
      break;
    case 0xA636:
      strcat(str, "Before send C2P command MboxBiosCmdGetBootPartitionId");
      break;
    case 0xA639:
      strcat(str, "Before send C2P command MboxBiosCmdSetRpmcAddress");
      break;
    case 0xA63A:
      strcat(str, "Before send C2P command MboxBiosCmdSetGpioFencing");
      break;
    case 0xA63F:
      strcat(str, "Before send C2P command MboxBiosCmdSendIvrsAcpiTable");
      break;
    case 0xA640:
      strcat(str, "Before send C2P command MboxBiosCmdTa");
      break;
    case 0xA641:
      strcat(str, "Before send C2P command MboxBiosCmdAcpiRasEinj");
      break;
    case 0xA642:
      strcat(str, "Before send C2P command MboxBiosCmdQueryTCGLog");
      break;
    case 0xA647:
      strcat(str, "Before send C2P command MboxBiosCmdQuerySplFuse");
      break;
    case 0xA649:
      strcat(str, "Before send C2P command MboxBiosCmdManageabilityCfg");
      break;
    case 0xA64A:
      strcat(str, "Before send C2P command MboxBiosCmdDisablePsb");
      break;
    case 0xA64B:
      strcat(str, "Before send C2P command MboxBiosCmdUsbConfig");
      break;
    case 0xA64D:
      strcat(str, "Before send C2P command MboxBiosCmdMpmPciAccess");
      break;
    case 0xA64F:
      strcat(str, "Before send C2P command MboxBiosCmdStbVerbosity");
      break;
    case 0xA650:
      strcat(str, "Before send C2P command MboxBiosCmdArmorEnterSmmOnlyMode2");
      break;
    case 0xA651:
      strcat(str, "Before send C2P command MboxBiosCmdArmorSpiTransaction");
      break;
    case 0xA652:
      strcat(str, "Before send C2P command MboxBiosCmdDeferredPsbFuse");
      break;
    case 0xA653:
      strcat(str, "Before send C2P command MboxBiosCmdLoadWlanFw");
      break;
    case 0xA654:
      strcat(str, "Before send C2P command MboxBiosCmdQuerySplValue");
      break;
    case 0xA680:
      strcat(str, "PSP C2P mailbox exit base");
      break;
    case 0xA681:
      strcat(str, "Wait C2P command MboxBiosCmdDramInfo finished");
      break;
    case 0xA682:
      strcat(str, "Wait C2P command MboxBiosCmdSmmInfo finished");
      break;
    case 0xA683:
      strcat(str, "Wait C2P command MboxBiosCmdSleep SxInfo finished");
      break;
    case 0xA684:
      strcat(str, "Wait C2P command MboxBiosCmdRsmInfo finished");
      break;
    case 0xA685:
      strcat(str, "Wait C2P command MboxBiosCmdFtpmQuery finished");
      break;
    case 0xA686:
      strcat(str, "Wait C2P command MboxBiosCmdBootDone finished");
      break;
    case 0xA687:
      strcat(str, "Wait C2P command MboxBiosCmdClearS3Sts finished");
      break;
    case 0xA688:
      strcat(str, "Wait C2P command MboxBiosCmdS3DataInfo finished");
      break;
    case 0xA689:
      strcat(str, "Wait C2P command MboxBiosCmdNop finished");
      break;
    case 0xA694:
      strcat(str, "Wait C2P command MboxBiosCmdHSTIQuery finished");
      break;
    case 0xA697:
      strcat(str, "Wait C2P command MboxBiosCmdClrSmmLock finished");
      break;
    case 0xA698:
      strcat(str, "Wait C2P command MboxBiosCmdPcieInfo finished");
      break;
    case 0xA699:
      strcat(str, "Wait C2P command MboxBiosCmdGetVersion finished");
      break;
    case 0xA69B:
      strcat(str, "Wait C2P command MboxBiosCmdLockDFReg finished");
      break;
    case 0xA69C:
      strcat(str, "Wait C2P command MboxBiosCmdClrSmmLock finished");
      break;
    case 0xA69D:
      strcat(str, "Wait C2P command MboxBiosCmdSetApCsBase finished");
      break;
    case 0xA69E:
      strcat(str, "Wait C2P command MboxBiosCmdKvmInfo finished");
      break;
    case 0xA69F:
      strcat(str, "Wait C2P command MboxBiosCmdLockSpi finished");
      break;
    case 0xA6A0:
      strcat(str, "Wait C2P command MboxBiosCmdScreenOnGpio finished");
      break;
    case 0xA6A1:
      strcat(str, "Wait C2P command MboxBiosCmdSpiOpWhiteList finished");
      break;
    case 0xA6A2:
      strcat(str, "Wait C2P command MboxBiosCmdRasEinj finished");
      break;
    case 0xA6A4:
      strcat(str, "Wait C2P command MboxBiosCmdStartArs finished");
      break;
    case 0xA6A5:
      strcat(str, "Wait C2P command MboxBiosCmdStopArs finished");
      break;
    case 0xA6A6:
      strcat(str, "Wait C2P command MboxBiosCmdSetBootPartitionId finished");
      break;
    case 0xA6A7:
      strcat(str, "Wait C2P command MboxBiosCmdPspCapsQuery finished");
      break;
    case 0xA6A8:
      strcat(str, "Wait C2P command MboxBiosCmdArmorEnterSmmOnlyMode finished");
      break;
    case 0xA6A9:
      strcat(str, "Wait C2P command MboxBiosCmdArmorEnforceWhitelist finished");
      break;
    case 0xA6AA:
      strcat(str, "Wait C2P command MboxBiosCmdArmorExecuteSpiCommand finished");
      break;
    case 0xA6AB:
      strcat(str, "Wait C2P command MboxBiosCmdArmorSwitchCsMode finished");
      break;
    case 0xA6AC:
      strcat(str, "Wait C2P command MboxBiosCmdDrtmInfoId finished");
      break;
    case 0xA6AD:
      strcat(str, "Wait C2P command MboxBiosCmdLaterSplFuse finished");
      break;
    case 0xA6AE:
      strcat(str, "Wait C2P command MboxBiosCmdDtpmInfo finished");
      break;
    case 0xA6AF:
      strcat(str, "Wait C2P command MboxBiosCmdValidateManOsSignature finished");
      break;
    case 0xA6B0:
      strcat(str, "Wait C2P command MboxBiosCmdLockFCHReg finished");
      break;
    case 0xA6B1:
      strcat(str, "Wait C2P command MboxBiosCmdPostDrtmInfoQuery finished");
      break;
    case 0xA6B4:
      strcat(str, "Wait C2P command MboxBiosCmdSignValidateHmacDataPreSmm finished");
      break;
    case 0xA6B5:
      strcat(str, "Wait C2P command MboxBiosCmdSignValidateHmacDataSmm finished");
      break;
    case 0xA6B6:
      strcat(str, "Wait C2P command MboxBiosCmdGetBootPartitionId finished");
      break;
    case 0xA6B9:
      strcat(str, "Wait C2P command MboxBiosCmdSetRpmcAddress finished");
      break;
    case 0xA6BA:
      strcat(str, "Wait C2P command MboxBiosCmdSetGpioFencing finished");
      break;
    case 0xA6BF:
      strcat(str, "Wait C2P command MboxBiosCmdSendIvrsAcpiTable finished");
      break;
    case 0xA6C0:
      strcat(str, "Wait C2P command MboxBiosCmdTa finished");
      break;
    case 0xA6C1:
      strcat(str, "Wait C2P command MboxBiosCmdAcpiRasEinj finished");
      break;
    case 0xA6C2:
      strcat(str, "Wait C2P command MboxBiosCmdQueryTCGLog finished");
      break;
    case 0xA6C7:
      strcat(str, "Wait C2P command MboxBiosCmdQuerySplFuse finished");
      break;
    case 0xA6C9:
      strcat(str, "Wait C2P command MboxBiosCmdManageabilityCfg finished");
      break;
    case 0xA6CA:
      strcat(str, "Wait C2P command MboxBiosCmdDisablePsb finished");
      break;
    case 0xA6CB:
      strcat(str, "Wait C2P command MboxBiosCmdUsbConfig finished");
      break;
    case 0xA6CD:
      strcat(str, "Wait C2P command MboxBiosCmdMpmPciAccess finished");
      break;
    case 0xA6CF:
      strcat(str, "Wait C2P command MboxBiosCmdStbVerbosity finished");
      break;
    case 0xA6CD0:
      strcat(str, "Wait C2P command MboxBiosCmdArmorEnterSmmOnlyMode2 finished");
      break;
    case 0xA6CD1:
      strcat(str, "Wait C2P command MboxBiosCmdArmorSpiTransaction finished");
      break;
    case 0xA6CD2:
      strcat(str, "Wait C2P command MboxBiosCmdDeferredPsbFuse finished");
      break;
    case 0xA6CD3:
      strcat(str, "Wait C2P command MboxBiosCmdLoadWlanFw finished");
      break;
    case 0xA6CD4:
      strcat(str, "Wait C2P command MboxBiosCmdQuerySplValue finished");
      break;
    case 0xA700:
      strcat(str, "MpmPei Driver entry");
      break;
    case 0xA701:
      strcat(str, "MpmPei Driver exit");
      break;
    case 0xA702:
      strcat(str, "MpmDxe Driver entry");
      break;
    case 0xA703:
      strcat(str, "MpmDxe Driver exit");
      break;
    case 0xA704:
      strcat(str, "MPM RTB event entry");
      break;
    case 0xA705:
      strcat(str, "MPM RTB event exit");
      break;
    case 0xA706:
      strcat(str, "Mpm SerialIo Entry");
      break;
    case 0xA707:
      strcat(str, "Mpm SerialIo Exit");
      break;
    case 0xA708:
      strcat(str, "TpMpmMoselleDxe Driver Entry");
      break;
    case 0xA709:
      strcat(str, "TpMpmMoselleDxe Driver Exit");
      break;
    case 0xA780:
      strcat(str, "Usb4Pei Driver entry");
      break;
    case 0xA781:
      strcat(str, "Usb4Pei Driver exit");
      break;
    case 0xA782:
      strcat(str, "Usb4Pei MemoryDicoverPpiCallback entry");
      break;
    case 0xA783:
      strcat(str, "Usb4Pei MemoryDicoverPpiCallback exit");
      break;
    case 0xA784:
      strcat(str, "Usb4Dxe Driver entry");
      break;
    case 0xA785:
      strcat(str, "Usb4Dxe Driver exit");
      break;
    case 0xAA00:
      strcat(str, "SmuMessageID");
      break;
    case 0xA900:
      strcat(str, "AmdNbioBase PEIM driver entry");
      break;
    case 0xA901:
      strcat(str, "AmdNbioBase PEIM driver exit");
      break;
    case 0xA902:
      strcat(str, "AmdNbioBase DXE driver entry");
      break;
    case 0xA903:
      strcat(str, "AmdNbioBase DXE driver exit");
      break;
    case 0xA904:
      strcat(str, "AmdNbioPcie PEIM driver entry");
      break;
    case 0xA905:
      strcat(str, "AmdNbioPcie PEIM driver exit");
      break;
    case 0xA906:
      strcat(str, "AmdNbioPcie DXE driver entry");
      break;
    case 0xA907:
      strcat(str, "AmdNbioPcie DXE driver exit");
      break;
    case 0xA908:
      strcat(str, "AmdNbioGfx PEIM driver entry");
      break;
    case 0xA909:
      strcat(str, "AmdNbioGfx PEIM driver exit");
      break;
    case 0xA90A:
      strcat(str, "AmdNbioGfx DXE driver entry");
      break;
    case 0xA90B:
      strcat(str, "AmdNbioGfx DXE driver exit");
      break;
    case 0xA90C:
      strcat(str, "AmdNbioIommu DXE driver entry");
      break;
    case 0xA90D:
      strcat(str, "AmdNbioIommu DXE driver exit");
      break;
    case 0xA90E:
      strcat(str, "AmdNbioALIB DXE driver entry");
      break;
    case 0xA90F:
      strcat(str, "AmdNbioALIB DXE driver exit");
      break;
    case 0xA910:
      strcat(str, "AmdSmu PEI driver entry");
      break;
    case 0xA911:
      strcat(str, "AmdSmu PEI driver exit");
      break;
    case 0xA912:
      strcat(str, "AmdSmu DXE driver entry");
      break;
    case 0xA913:
      strcat(str, "AmdSmu DXE driver exit");
      break;
    case 0xA914:
      strcat(str, "SmuServiceRequest entry");
      break;
    case 0xA915:
      strcat(str, "SmuServiceRequest exit");
      break;
    case 0xA916:
      strcat(str, "AmdNbioIoApic PEI driver entry");
      break;
    case 0xA917:
      strcat(str, "AmdNbioIoApic PEI driver exit");
      break;
    case 0xA918:
      strcat(str, "AmdSmuV10 PEIM driver entry");
      break;
    case 0xA919:
      strcat(str, "AmdSmuV10 PEIM driver exit");
      break;
    case 0xA91A:
      strcat(str, "AmdSmuV10 DXE driver entry");
      break;
    case 0xA91B:
      strcat(str, "AmdSmuV10 DXE driver exit");
      break;
    case 0xA91C:
      strcat(str, "AmdSmuV13 PEIM driver entry");
      break;
    case 0xA91D:
      strcat(str, "AmdSmuV13 PEIM driver exit");
      break;
    case 0xA91E:
      strcat(str, "AmdSmuV13 DXE driver entry");
      break;
    case 0xA91F:
      strcat(str, "AmdSmuV13 DXE driver exit");
      break;
    case 0xA920:
      strcat(str, "AmdNbioIommu PEIM driver entry");
      break;
    case 0xA921:
      strcat(str, "AmdNbioIommu PEIM driver exit");
      break;
    case 0xA922:
      strcat(str, "APCB DXE Entry");
      break;
    case 0xA923:
      strcat(str, "APCB DXE Exit");
      break;
    case 0xA924:
      strcat(str, "APCB SMM Entry");
      break;
    case 0xA925:
      strcat(str, "APCB SMM Exit");
      break;
    case 0xA930:
      strcat(str, "Early Exit");
      break;
    case 0xA931:
      strcat(str, "Early Exit");
      break;
    case 0xA932:
      strcat(str, "Early Training is completed");
      break;
    case 0xA950:
      strcat(str, "NbioTopologyConfigureCallback entry");
      break;
    case 0xA951:
      strcat(str, "NbioTopologyConfigureCallback exit");
      break;
    case 0xA952:
      strcat(str, "MemoryConfigDoneCallbackPpi entry");
      break;
    case 0xA953:
      strcat(str, "MemoryConfigDoneCallbackPpi exit");
      break;
    case 0xA954:
      strcat(str, "DxioInitializationCallbackPpi entry");
      break;
    case 0xA955:
      strcat(str, "DxioInitializationCallbackPpi exit");
      break;
    case 0xA956:
      strcat(str, "DispatchSmuV9Callback entry");
      break;
    case 0xA957:
      strcat(str, "DispatchSmuV9Callback exit");
      break;
    case 0xA958:
      strcat(str, "DispatchSmuV10Callback entry");
      break;
    case 0xA959:
      strcat(str, "DispatchSmuV10Callback exit");
      break;
    case 0xA95A:
      strcat(str, "AmdPcieMiscInit Event entry");
      break;
    case 0xA95B:
      strcat(str, "AmdPcieMiscInit Event exit");
      break;
    case 0xA95C:
      strcat(str, "NbioBaseHookReadyToBoot Event entry");
      break;
    case 0xA95D:
      strcat(str, "NbioBaseHookReadyToBoot Event exit");
      break;
    case 0xA95E:
      strcat(str, "NbioBaseHookPciIO Event entry");
      break;
    case 0xA95F:
      strcat(str, "NbioBaseHookPciIO Event exit");
      break;
    case 0xA960:
      strcat(str, "DispatchSmuV13Callback entry");
      break;
    case 0xA961:
      strcat(str, "DispatchSmuV13Callback exit");
      break;
    case 0xA970:
      strcat(str, "GnbEarlyInterfaceCZ entry");
      break;
    case 0xA971:
      strcat(str, "GnbEarlyInterfaceCZ exit");
      break;
    case 0xA972:
      strcat(str, "PcieConfigurationInit entry");
      break;
    case 0xA973:
      strcat(str, "PcieConfigurationInit exit");
      break;
    case 0xA974:
      strcat(str, "GnbEarlierInterfaceCZ entry");
      break;
    case 0xA975:
      strcat(str, "GnbEarlierInterfaceCZ exit");
      break;
    case 0xA976:
      strcat(str, "PcieEarlyInterfaceCZ entry");
      break;
    case 0xA977:
      strcat(str, "PcieEarlyInterfaceCZ exit");
      break;
    case 0xA978:
      strcat(str, "PciePostEarlyInterfaceCZ entry");
      break;
    case 0xA979:
      strcat(str, "PciePostEarlyInterfaceCZ exit");
      break;
    case 0xA97A:
      strcat(str, "GfxConfigPostInterfaceCZ entry");
      break;
    case 0xA97B:
      strcat(str, "GfxConfigPostInterfaceCZ exit");
      break;
    case 0xA97C:
      strcat(str, "GfxPostInterfaceCZ entry");
      break;
    case 0xA97D:
      strcat(str, "GfxPostInterfaceCZ exit");
      break;
    case 0xA97E:
      strcat(str, "GnbPostInterfaceCZ entry");
      break;
    case 0xA97F:
      strcat(str, "GnbPostInterfaceCZ exit");
      break;
    case 0xA980:
      strcat(str, "PciePostInterfaceCZ entry");
      break;
    case 0xA981:
      strcat(str, "PciePostInterfaceCZ exit");
      break;
    case 0xA982:
      strcat(str, "GnbEnvInterfaceCZ entry");
      break;
    case 0xA983:
      strcat(str, "GnbEnvInterfaceCZ exit");
      break;
    case 0xA984:
      strcat(str, "GfxConfigEnvInterface entry");
      break;
    case 0xA985:
      strcat(str, "GfxConfigEnvInterface exit");
      break;
    case 0xA986:
      strcat(str, "GfxEnvInterfaceCZ entry");
      break;
    case 0xA987:
      strcat(str, "GfxEnvInterfaceCZ exit");
      break;
    case 0xA988:
      strcat(str, "GfxMidInterfaceCZ entry");
      break;
    case 0xA989:
      strcat(str, "GfxMidInterfaceCZ exit");
      break;
    case 0xA98A:
      strcat(str, "GfxIntInfoTableInterfaceCZ entry");
      break;
    case 0xA98B:
      strcat(str, "GfxIntInfoTableInterfaceCZ exit");
      break;
    case 0xA98C:
      strcat(str, "PcieMidInterfaceCZ entry");
      break;
    case 0xA98D:
      strcat(str, "PcieMidInterfaceCZ exit");
      break;
    case 0xA98E:
      strcat(str, "GnbMidInterfaceCZ entry");
      break;
    case 0xA98F:
      strcat(str, "GnbMidInterfaceCZ exit");
      break;
    case 0xA990:
      strcat(str, "GnbSmuMidInterfaceCZ entry");
      break;
    case 0xA991:
      strcat(str, "GnbSmuMidInterfaceCZ exit");
      break;
    case 0xA992:
      strcat(str, "InvokeAmdInitLate entry");
      break;
    case 0xA993:
      strcat(str, "InvokeAmdInitLate exit");
      break;
    case 0xA994:
      strcat(str, "GnbSmuServiceRequestV8 entry");
      break;
    case 0xA995:
      strcat(str, "GnbSmuServiceRequestV8 exit");
      break;
    case 0xAA80:
      strcat(str, "ALib 0 entry");
      break;
    case 0xAA81:
      strcat(str, "ALib 0 exit");
      break;
    case 0xAA82:
      strcat(str, "ALib 1 entry");
      break;
    case 0xAA83:
      strcat(str, "ALib 1 exit");
      break;
    case 0xAA84:
      strcat(str, "ALib 2 entry");
      break;
    case 0xAA85:
      strcat(str, "ALib 2 exit");
      break;
    case 0xAA86:
      strcat(str, "ALib 3 entry");
      break;
    case 0xAA87:
      strcat(str, "ALib 3 exit");
      break;
    case 0xAA88:
      strcat(str, "ALib 6 entry");
      break;
    case 0xAA89:
      strcat(str, "ALib 6 exit");
      break;
    case 0xAA8A:
      strcat(str, "ALib A entry");
      break;
    case 0xAA8B:
      strcat(str, "ALib A exit");
      break;
    case 0xAA8C:
      strcat(str, "ALib B entry");
      break;
    case 0xAA8D:
      strcat(str, "ALib B exit");
      break;
    case 0xAA8E:
      strcat(str, "ALib C entry");
      break;
    case 0xAA8F:
      strcat(str, "ALib C exit");
      break;
    case 0xAA90:
      strcat(str, "ALib 10 entry");
      break;
    case 0xAA91:
      strcat(str, "ALib 10 exit");
      break;
    case 0xAA92:
      strcat(str, "ALib 11 entry");
      break;
    case 0xAA93:
      strcat(str, "ALib 11 exit");
      break;
    case 0xAA94:
      strcat(str, "ALib 12 entry");
      break;
    case 0xAA95:
      strcat(str, "ALib 12 exit");
      break;
    case 0xAA96:
      strcat(str, "ALib 13 entry");
      break;
    case 0xAA97:
      strcat(str, "ALib 13 exit");
      break;
    case 0xAA98:
      strcat(str, "ALib AA entry");
      break;
    case 0xAA99:
      strcat(str, "ALib AA exit");
      break;
    case 0xAC10:
      strcat(str, "CCX IDS IDS_HOOK_CCX_AFTER_AP_LAUNCH");
      break;
    case 0xAC50:
      strcat(str, "CCX PEI entry");
      break;
    case 0xAC51:
      strcat(str, "CCX downcore entry");
      break;
    case 0xAC55:
      strcat(str, "CCX DXE entry");
      break;
    case 0xAC56:
      strcat(str, "CCX MP service callback entry");
      break;
    case 0xAC57:
      strcat(str, "CCX Ready To Boot callback entry");
      break;
    case 0xAC58:
      strcat(str, "CCX oc service callback entry");
      break;
    case 0xAC5D:
      strcat(str, "CCX SMM entry");
      break;
    case 0xAC70:
      strcat(str, "CCX PEI start to launch APs for S3");
      break;
    case 0xAC71:
      strcat(str, "CCX PEI end of launching APs for S3");
      break;
    case 0xAC90:
      strcat(str, "CCX start to launch AP");
      break;
    case 0xAC91:
      strcat(str, "CCX launch AP is ended");
      break;
    case 0xAC92:
      strcat(str, "CCX launch AP abort");
      break;
    case 0xAC93:
      strcat(str, "CCX MP service abort");
      break;
    case 0xAC94:
      strcat(str, "CCX cac weights");
      break;
    case 0xAC95:
      strcat(str, "CCX before install CcxDone");
      break;
    case 0xAC96:
      strcat(str, "CCX after install CcxDone");
      break;
    case 0xAC97:
      strcat(str, "CCX loaded microcode");
      break;
    case 0xACE0:
      strcat(str, "CCX PEI exit");
      break;
    case 0xACE1:
      strcat(str, "CCX downcore exit");
      break;
    case 0xACE5:
      strcat(str, "CCX DXE exit");
      break;
    case 0xACE6:
      strcat(str, "CCX MP service callback exit");
      break;
    case 0xACE7:
      strcat(str, "CCX Ready To Boot callback exit");
      break;
    case 0xACE8:
      strcat(str, "CCX OC service callback exit");
      break;
    case 0xACED:
      strcat(str, "CCX SMM exit");
      break;
    case 0xAD50:
      strcat(str, "DF PEI entry");
      break;
    case 0xAD55:
      strcat(str, "DF DXE entry");
      break;
    case 0xAD56:
      strcat(str, "DF Ready to Boot entry");
      break;
    case 0xAD57:
      strcat(str, "DF NbioSmuServicesPpiCallback entry");
      break;
    case 0xAD58:
      strcat(str, "DF NbioSmuServicesProtocolCallback entry");
      break;
    case 0xAD59:
      strcat(str, "DF SMM entry");
      break;
    case 0xAD60:
      strcat(str, "DF FabricPciEnumerationCompleteCallBack entry");
      break;
    case 0xADE0:
      strcat(str, "DF PEI exit");
      break;
    case 0xADE5:
      strcat(str, "DF DXE exit");
      break;
    case 0xADE6:
      strcat(str, "DF Ready to Boot exit");
      break;
    case 0xADE7:
      strcat(str, "DF NbioSmuServicesPpiCallback exit");
      break;
    case 0xADE8:
      strcat(str, "DF NbioSmuServicesProtocolCallback exit");
      break;
    case 0xADE9:
      strcat(str, "DF SMM exit");
      break;
    case 0xADEA:
      strcat(str, "DF FabricPciEnumerationCompleteCallBack exit");
      break;
    case 0xAF01:
      strcat(str, "FCH InitReset dispatch point");
      break;
    case 0xAF06:
      strcat(str, "FCH InitEnv dispatch point");
      break;
    case 0xAF07:
      strcat(str, "FCH InitMid dispatch point");
      break;
    case 0xAF08:
      strcat(str, "FCH InitLate dispatch point");
      break;
    case 0xAF0B:
      strcat(str, "FCH InitS3Early dispatch point");
      break;
    case 0xAF0C:
      strcat(str, "FCH InitS3Late dispatch point");
      break;
    case 0xAF0D:
      strcat(str, "FCH InitS3Early dispatch finished");
      break;
    case 0xAF0E:
      strcat(str, "FCH InitS3Late dispatch finished");
      break;
    case 0xAF10:
      strcat(str, "FCH Pei Entry");
      break;
    case 0xAF11:
      strcat(str, "FCH Pei Exit");
      break;
    case 0xAF12:
      strcat(str, "FCH MultiFch Pei Entry");
      break;
    case 0xAF13:
      strcat(str, "FCH MultiFch Pei Exit");
      break;
    case 0xAF14:
      strcat(str, "FCH Dxe Entry");
      break;
    case 0xAF15:
      strcat(str, "FCH Dxe Exit");
      break;
    case 0xAF16:
      strcat(str, "FCH MultiFch Dxe Entry");
      break;
    case 0xAF17:
      strcat(str, "FCH MultiFch Dxe Exit");
      break;
    case 0xAF18:
      strcat(str, "FCH Smm Entry");
      break;
    case 0xAF19:
      strcat(str, "FCH Smm Exit");
      break;
    case 0xAF20:
      strcat(str, "FCH Smm Dispatcher Entry");
      break;
    case 0xAF21:
      strcat(str, "FCH Smm Dispatcher Exit");
      break;
    case 0xAF40:
      strcat(str, "FCH InitReset HwAcpi");
      break;
    case 0xAF41:
      strcat(str, "FCH InitReset AB Link");
      break;
    case 0xAF42:
      strcat(str, "FCH InitReset LPC");
      break;
    case 0xAF43:
      strcat(str, "FCH InitReset SPI");
      break;
    case 0xAF44:
      strcat(str, "FCH InitReset eSPI");
      break;
    case 0xAF45:
      strcat(str, "FCH InitReset SD");
      break;
    case 0xAF46:
      strcat(str, "FCH InitReset eMMC");
      break;
    case 0xAF47:
      strcat(str, "FCH InitReset SATA");
      break;
    case 0xAF48:
      strcat(str, "FCH InitReset USB");
      break;
    case 0xAF49:
      strcat(str, "FCH InitReset xGbE");
      break;
    case 0xAF4A:
      strcat(str, "FCH InitReset USB Ready");
      break;
    case 0xAF4B:
      strcat(str, "FCH InitReset USB4 Ready");
      break;
    case 0xAF4E:
      strcat(str, "FCH InitReset USB4");
      break;
    case 0xAF4F:
      strcat(str, "FCH InitReset HwAcpiP");
      break;
    case 0xAF50:
      strcat(str, "FCH InitEnv HwAcpi");
      break;
    case 0xAF51:
      strcat(str, "FCH InitEnv AB Link");
      break;
    case 0xAF52:
      strcat(str, "FCH InitEnv LPC");
      break;
    case 0xAF53:
      strcat(str, "FCH InitEnv SPI");
      break;
    case 0xAF54:
      strcat(str, "FCH InitEnv eSPI");
      break;
    case 0xAF55:
      strcat(str, "FCH InitEnv SD");
      break;
    case 0xAF56:
      strcat(str, "FCH InitEnv eMMC");
      break;
    case 0xAF57:
      strcat(str, "FCH InitEnv SATA");
      break;
    case 0xAF58:
      strcat(str, "FCH InitEnv USB");
      break;
    case 0xAF59:
      strcat(str, "FCH InitEnv xGbE");
      break;
    case 0xAF5E:
      strcat(str, "FCH InitEnv USB4");
      break;
    case 0xAF5F:
      strcat(str, "FCH InitEnv HwAcpiP");
      break;
    case 0xAF60:
      strcat(str, "FCH InitMid HwAcpi");
      break;
    case 0xAF61:
      strcat(str, "FCH InitMid AB Link");
      break;
    case 0xAF62:
      strcat(str, "FCH InitMid LPC");
      break;
    case 0xAF63:
      strcat(str, "FCH InitMid SPI");
      break;
    case 0xAF64:
      strcat(str, "FCH InitMid eSPI");
      break;
    case 0xAF65:
      strcat(str, "FCH InitMid SD");
      break;
    case 0xAF66:
      strcat(str, "FCH InitMid eMMC");
      break;
    case 0xAF67:
      strcat(str, "FCH InitMid SATA");
      break;
    case 0xAF68:
      strcat(str, "FCH InitMid USB");
      break;
    case 0xAF69:
      strcat(str, "FCH InitMid xGbE");
      break;
    case 0xAF6E:
      strcat(str, "FCH InitMid USB4");
      break;
    case 0xAF70:
      strcat(str, "FCH InitLate HwAcpi");
      break;
    case 0xAF71:
      strcat(str, "FCH InitLate AB Link");
      break;
    case 0xAF72:
      strcat(str, "FCH InitLate LPC");
      break;
    case 0xAF73:
      strcat(str, "FCH InitLate SPI");
      break;
    case 0xAF74:
      strcat(str, "FCH InitLate eSPI");
      break;
    case 0xAF75:
      strcat(str, "FCH InitLate SD");
      break;
    case 0xAF76:
      strcat(str, "FCH InitLate eMMC");
      break;
    case 0xAF77:
      strcat(str, "FCH InitLate SATA");
      break;
    case 0xAF78:
      strcat(str, "FCH InitLate USB");
      break;
    case 0xAF79:
      strcat(str, "FCH InitLate xGbE");
      break;
    case 0xAF7A:
      strcat(str, "FCH PT load FW Entry");
      break;
    case 0xAF7B:
      strcat(str, "FCH PT load FW Exit");
      break;
    case 0xAF7C:
      strcat(str, "FCH PT_Plus load FW Entry");
      break;
    case 0xAF7D:
      strcat(str, "FCH PT_Plus load FW Exit");
      break;
    case 0xAF80:
      strcat(str, "FCH Device Enter Dx Status");
      break;
    case 0xAF81:
      strcat(str, "FCH PT load FW Delay 1 sec");
      break;
    case 0xAF82:
      strcat(str, "FCH PT load FW Delay 2 sec");
      break;
    case 0xAF83:
      strcat(str, "FCH PT load FW Delay 3 sec");
      break;
    case 0xAF84:
      strcat(str, "FCH PT load FW Delay 4 sec");
      break;
    case 0xAF85:
      strcat(str, "FCH PT load FW Delay 5 sec");
      break;
    case 0xAF86:
      strcat(str, "FCH PT load FW Delay 6 sec");
      break;
    case 0xAF87:
      strcat(str, "FCH PT load FW Delay 7 sec");
      break;
    case 0xAF88:
      strcat(str, "FCH PT load FW Delay 8 sec");
      break;
    case 0xAF89:
      strcat(str, "FCH PT load FW Delay 9 sec");
      break;
    case 0xAF8A:
      strcat(str, "FCH PT load FW Delay 10 sec");
      break;
    case 0xAF8B:
      strcat(str, "FCH Secondary PT Start to load FW");
      break;
    case 0xAF90:
      strcat(str, "FCH Turner Load FW Entry");
      break;
    case 0xAF91:
      strcat(str, "FCH Turner Load FW Exit");
      break;
    case 0xAF92:
      strcat(str, "FCH Turner Start to Load FW");
      break;
    case 0xAFB0:
      strcat(str, "Bixby Pei Entry");
      break;
    case 0xAFB1:
      strcat(str, "Bixby Pei Exit");
      break;
    case 0xAFB2:
      strcat(str, "Bixby Dxe Entry");
      break;
    case 0xAFB3:
      strcat(str, "Bixby Dxe Exit");
      break;
    case 0xAFB4:
      strcat(str, "Bixby Smm Entry");
      break;
    case 0xAFB5:
      strcat(str, "Bixby Smm Exit");
      break;
    case 0xAFB6:
      strcat(str, "Bixby InitReset dispatch point");
      break;
    case 0xAFB7:
      strcat(str, "Bixby InitMid dispatch point");
      break;
    case 0xAFB8:
      strcat(str, "Bixby InitEnv dispatch point");
      break;
    case 0xAFB9:
      strcat(str, "Bixby InitLate dispatch point");
      break;
    case 0xAFBA:
      strcat(str, "Bixby InitS3Early dispatch point");
      break;
    case 0xAFBB:
      strcat(str, "Bixby InitS3Late dispatch point");
      break;
    case 0xAFBC:
      strcat(str, "Bixby InitS3Early dispatch finished");
      break;
    case 0xAFBD:
      strcat(str, "Bixby InitS3Late dispatch finished");
      break;
    case 0xAFBE:
      strcat(str, "Bixby InitReset SATA Entry");
      break;
    case 0xAFBF:
      strcat(str, "Bixby InitReset SATA Exit");
      break;
    case 0xAFC0:
      strcat(str, "Bixby InitMid SATA Entry");
      break;
    case 0xAFC1:
      strcat(str, "Bixby InitMid SATA Exit");
      break;
    case 0xAFC2:
      strcat(str, "Bixby InitEnv SATA Entry");
      break;
    case 0xAFC3:
      strcat(str, "Bixby InitEnv SATA Exit");
      break;
    case 0xAFC4:
      strcat(str, "Bixby InitLate SATA Entry");
      break;
    case 0xAFC5:
      strcat(str, "Bixby InitLate SATA Exit");
      break;
    case 0xAFC6:
      strcat(str, "Bixby InitReset USB Entry");
      break;
    case 0xAFC7:
      strcat(str, "Bixby InitReset USB Exit");
      break;
    case 0xAFC8:
      strcat(str, "Bixby InitMid USB Entry");
      break;
    case 0xAFC9:
      strcat(str, "Bixby InitMid USB Exit");
      break;
    case 0xAFCA:
      strcat(str, "Bixby InitEnv USB Entry");
      break;
    case 0xAFCB:
      strcat(str, "Bixby InitEnv USB Exit");
      break;
    case 0xAFCC:
      strcat(str, "Bixby InitLate USB Entry");
      break;
    case 0xAFCD:
      strcat(str, "Bixby InitLate USB Exit");
      break;
    case 0xAFCE:
      strcat(str, "Bixby InitEnv HwAcpiP");
      break;
    case 0xAFCF:
      strcat(str, "Bixby InitReset HwAcpiP");
      break;
    case 0xAFFF:
      strcat(str, "End of TP range for FCH");
      break;
    case 0xFFFF:
      strcat(str, "Last defined AGESA PCs");
      break;
    // CPM POST CODE
    case 0x0C01:
      strcat(str, "No free table item or no enough size in Hob Buffer");
      break;
    case 0x0C02:
      strcat(str, "Table number is greater than AMD_TABLE_LIST_ITEM_SIZE");
      break;
    case 0x0C03:
      strcat(str, "Main Table pointer is invalid");
      break;
    case 0x0C30:
      strcat(str, "Begin of GPIO Init PEIM driver");
      break;
    case 0x0C31:
      strcat(str, "End of GPIO Init PEIM driver");
      break;
    case 0x0C32:
      strcat(str, "Begin to Reset Device in GPIO Init PEIM driver");
      break;
    case 0x0C33:
      strcat(str, "End to Reset Device in GPIO Init PEIM driver");
      break;
    case 0x0C34:
      strcat(str, "Begin to Set Mem Voltage in GPIO Init PEIM driver");
      break;
    case 0x0C35:
      strcat(str, "End to Set Mem Voltage in GPIO Init PEIM driver");
      break;
    case 0x0C36:
      strcat(str, "Begin to Set Vddp/Vddr Voltage in GPIO Init PEIM driver");
      break;
    case 0x0C37:
      strcat(str, "End to Set Vddp/Vddr Voltage in GPIO Init PEIM driver");
      break;
    case 0x0C38:
      strcat(str, "Begin of GPIO Init DXE driver");
      break;
    case 0x0C39:
      strcat(str, "Begin of GPIO Init DXE driver");
      break;
    case 0x0C3A:
      strcat(str, "Begin to init PCIe Clock in GPIO Init DXE driver");
      break;
    case 0x0C3B:
      strcat(str, "Begin to init PCIe Clock in GPIO Init DXE driver");
      break;
    case 0x0C3C:
      strcat(str, "Begin of GPIO Init SMM driver");
      break;
    case 0x0C3D:
      strcat(str, "Begin of GPIO Init SMM driver");
      break;
    case 0x0C3E:
      strcat(str, "Begin of PCIE Init PEIM driver");
      break;
    case 0x0C3F:
      strcat(str, "End of PCIE Init PEIM driver");
      break;
    case 0x0C40:
      strcat(str, "Begin of PCIE Init DXE driver");
      break;
    case 0x0C41:
      strcat(str, "End of PCIE Init DXE driver");
      break;
    default:
      strcat(str, "AGESA Unknown");
  }
}

void
pal_parse_amd_abl(uint32_t post_code, char *str )
{
  switch (post_code){
    case 0xE000:
      strcat(str, "Entry used for range testing for @b Processor related TPs");
      break;
    case 0xE001:
      strcat(str, "Memory structure initialization (Public interface)");
      break;
    case 0xE002:
      strcat(str, "SPD Data processing  (Public interface)");
      break;
    case 0xE003:
      strcat(str, "Memory configuration  (Public interface) Phase 1");
      break;
    case 0xE004:
      strcat(str, "DRAM initialization");
      break;
    case 0xE005:
      strcat(str, "ProcMemSPDChecking");
      break;
    case 0xE006:
      strcat(str, "ProcMemModeChecking");
      break;
    case 0xE007:
      strcat(str, "Speed and TCL configuration");
      break;
    case 0xE008:
      strcat(str, "ProcMemSpdTiming");
      break;
    case 0xE009:
      strcat(str, "ProcMemDramMapping");
      break;
    case 0xE00A:
      strcat(str, "ProcMemPlatformSpecificConfig");
      break;
    case 0xE00B:
      strcat(str, "ProcMemPhyCompensation");
      break;
    case 0xE00C:
      strcat(str, "ProcMemStartDcts");
      break;
    case 0xE00D:
      strcat(str, "ProcMemBeforeDramInit (Public interface)");
      break;
    case 0xE00E:
      strcat(str, "ProcMemPhyFenceTraining");
      break;
    case 0xE00F:
      strcat(str, "ProcMemSynchronizeDcts");
      break;
    case 0xE010:
      strcat(str, "ProcMemSystemMemoryMapping");
      break;
    case 0xE011:
      strcat(str, "ProcMemMtrrConfiguration");
      break;
    case 0xE012:
      strcat(str, "ProcMemDramTraining");
      break;
    case 0xE013:
      strcat(str, "ProcMemBeforeAnyTraining(Public interface)");
      break;
    case 0xE014:
      strcat(str, "ABL Mem - PMU - Before PMU Firmware load");
      break;
    case 0xE015:
      strcat(str, "ABL Mem - PMU - After PMU Firmware load");
      break;
    case 0xE016:
      strcat(str, "ABL Mem - PMU Populate SRAM Timing");
      break;
    case 0xE017:
      strcat(str, "ABL Mem - PMU Populate SRAM Config");
      break;
    case 0xE018:
      strcat(str, "ABL Mem - PMU Write SRAM Msg Block");
      break;
    case 0xE019:
      strcat(str, "ABL Mem - Wait for Phy Cal Complete");
      break;
    case 0xE01A:
      strcat(str, "ABL Mem - Phy Cal Complete");
      break;
    case 0xE01B:
      strcat(str, "ABL Mem - PMU Start");
      break;
    case 0xE01C:
      strcat(str, "ABL Mem - PMU Started");
      break;
    case 0xE01D:
      strcat(str, "ABL Mem - PMU Waiting for Complete");
      break;
    case 0xE01E:
      strcat(str, "ABL Mem - PMU Stage Dec Init");
      break;
    case 0xE01F:
      strcat(str, "ABL Mem - PMU Stage Training Wr Lvl");
      break;
    case 0xE020:
      strcat(str, "ABL Mem - PMU Stage Training Rx En");
      break;
    case 0xE021:
      strcat(str, "ABL Mem - PMU Stage Training Rd Dqs");
      break;
    case 0xE022:
      strcat(str, "ABL Mem - PMU Stage Traning Rd 2D");
      break;
    case 0xE023:
      strcat(str, "ABL Mem - PMU Stage Training Wr 2D");
      break;
    case 0xE024:
      strcat(str, "ABL Mem - PMU Queue Empty");
      break;
    case 0xE025:
      strcat(str, "ABL Mem - PMU US message Start");
      break;
    case 0xE026:
      strcat(str, "ABL Mem - PMU US message End");
      break;
    case 0xE027:
      strcat(str, "ABL Mem - PMU Complete");
      break;
    case 0xE028:
      strcat(str, "ABL Mem - PMU - After PMU Training");
      break;
    case 0xE029:
      strcat(str, "ABL Mem - PMU - Before Disable PMU");
      break;
    case 0xE02A:
      strcat(str, "ABL Mem - ProcMemTransmitDqsTraining");
      break;
    case 0xE02B:
      strcat(str, "ABL Mem - Start write sweep");
      break;
    case 0xE02C:
      strcat(str, "ABL Mem - Set Transmit DQ delay");
      break;
    case 0xE02D:
      strcat(str, "ABL Mem - Write test pattern");
      break;
    case 0xE02E:
      strcat(str, "ABL Mem - Read Test pattern");
      break;
    case 0xE02F:
      strcat(str, "ABL Mem - Compare Test pattern");
      break;
    case 0xE030:
      strcat(str, "ABL Mem - Update results");
      break;
    case 0xE031:
      strcat(str, "ABL Mem - Start Find passing window");
      break;
    case 0xE032:
      strcat(str, "ABL Mem - ProcMemMaxRdLatencyTraining");
      break;
    case 0xE033:
      strcat(str, "ABL Mem - Start sweep");
      break;
    case 0xE034:
      strcat(str, "ABL Mem - Set delay");
      break;
    case 0xE035:
      strcat(str, "ABL Mem - Write test pattern");
      break;
    case 0xE036:
      strcat(str, "ABL Mem - Read Test pattern");
      break;
    case 0xE037:
      strcat(str, "ABL Mem - Compare Test pattern");
      break;
    case 0xE038:
      strcat(str, "ABL Mem - Online Spare init");
      break;
    case 0xE039:
      strcat(str, "ABL Mem - Chip select Interleave Init");
      break;
    case 0xE03A:
      strcat(str, "ABL Mem - Node Interleave Init");
      break;
    case 0xE03B:
      strcat(str, "ABL Mem - Channel Interleave Init");
      break;
    case 0xE03C:
      strcat(str, "ABL Mem - ECC initialization");
      break;
    case 0xE03D:
      strcat(str, "ABL Mem - Platform Specific Init");
      break;
    case 0xE03E:
      strcat(str, "ABL Mem - Before callout for AgesaReadSpd");
      break;
    case 0xE03F:
      strcat(str, "ABL Mem - After callout for AgesaReadSpd");
      break;
    case 0xE040:
      strcat(str, "ABL Mem - Before optional callout AgesaHookBeforeDramInit");
      break;
    case 0xE041:
      strcat(str, "ABL Mem - After optional callout AgesaHookBeforeDramInit");
      break;
    case 0xE042:
      strcat(str, "ABL Mem - Before optional callout AgesaHookBeforeDQSTraining");
      break;
    case 0xE043:
      strcat(str, "ABL Mem - After optional callout AgesaHookBeforeDQSTraining");
      break;
    case 0xE044:
      strcat(str, "ABL Mem - Before optional callout AgesaHookBeforeDramInit");
      break;
    case 0xE045:
      strcat(str, "ABL Mem - After optional callout AgesaHookBeforeDramInit");
      break;
    case 0xE046:
      strcat(str, "ABL Mem - After MemDataInit");
      break;
    case 0xE047:
      strcat(str, "ABL Mem - Before InitializeMCT");
      break;
    case 0xE048:
      strcat(str, "ABL Mem - Before LV DDR3");
      break;
    case 0xE049:
      strcat(str, "ABL Mem - Before InitMCT");
      break;
    case 0xE04A:
      strcat(str, "ABL Mem - Before OtherTiming");
      break;
    case 0xE04B:
      strcat(str, "ABL Mem - Before UMAMemTyping");
      break;
    case 0xE04C:
      strcat(str, "ABL Mem - Before SetDqsEccTmgs");
      break;
    case 0xE04D:
      strcat(str, "ABL Mem - Before MemClr");
      break;
    case 0xE04E:
      strcat(str, "ABL Mem - Before On DIMM Thermal");
      break;
    case 0xE04F:
      strcat(str, "ABL Mem - Before DMI");
      break;
    case 0xE050:
      strcat(str, "ABL MEM - End of phase 3 memory code");
      break;
    case 0xE051:
      strcat(str, "Entry point CPU init after training");
      break;
    case 0xE052:
      strcat(str, "Exit point CPU init after training");
      break;
    case 0xE053:
      strcat(str, "Entry point CPU APOB data init");
      break;
    case 0xE054:
      strcat(str, "Exit point CPU APOB data init");
      break;
    case 0xE055:
      strcat(str, "Entry point CPU Optimized boot init");
      break;
    case 0xE056:
      strcat(str, "Exit point CPU Optimized boot init");
      break;
    case 0xE057:
      strcat(str, "Entry point CPU APOB EDC info init");
      break;
    case 0xE058:
      strcat(str, "Exit point CPU APOB EDC info init");
      break;
    case 0xE059:
      strcat(str, "Entry point CPU APOB CCD map data init");
      break;
    case 0xE05A:
      strcat(str, "Exit point CPU APOB CCD map data init");
      break;
    case 0xE080:
      strcat(str, "ProcMemSendMRS2");
      break;
    case 0xE081:
      strcat(str, "Sedding MRS3");
      break;
    case 0xE082:
      strcat(str, "Sending MRS1");
      break;
    case 0xE083:
      strcat(str, "Sending MRS0");
      break;
    case 0xE084:
      strcat(str, "Continuous Pattern Read");
      break;
    case 0xE085:
      strcat(str, "Continuous Pattern Write");
      break;
    case 0xE086:
      strcat(str, "Mem: 2d RdDqs Training begin");
      break;
    case 0xE087:
      strcat(str, "Mem: Before optional callout to platform BIOS to change External Vref during 2d Training");
      break;
    case 0xE088:
      strcat(str, "Mem: After optional callout to platform BIOS to change External Vref during 2d Training");
      break;
    case 0xE089:
      strcat(str, "Configure DCT For General use begin");
      break;
    case 0xE08A:
      strcat(str, "Configure DCT For training begin");
      break;
    case 0xE08B:
      strcat(str, "Configure DCT For Non-Explicit");
      break;
    case 0xE08C:
      strcat(str, "Configure to Sync channels");
      break;
    case 0xE08D:
      strcat(str, "Allocate C6 Storage");
      break;
    case 0xE08E:
      strcat(str, "Before LV DDR4");
      break;
    case 0xE08F:
      strcat(str, "Before LV DDR3");
      break;
    case 0xE090:
      strcat(str, "TP0x90");
      break;
    case 0xE091:
      strcat(str, "GNB earlier interface");
      break;
    case 0xE092:
      strcat(str, "GNB Early VGA entry");
      break;
    case 0xE093:
      strcat(str, "GNB Early VGA exit");
      break;
    case 0xE094:
      strcat(str, "GNB Initialization entry");
      break;
    case 0xE095:
      strcat(str, "GNB Initialization exit");
      break;
    case 0xE096:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE097:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE098:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE099:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE09A:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE09B:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE09C:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE09D:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE09E:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE09F:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE0A0:
      strcat(str, "TP0xA0");
      break;
    case 0xE0A1:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE0A2:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE0A3:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE0A4:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE0A5:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE0A6:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE0A7:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE0A8:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE0A9:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE0AA:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE0AB:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE0AC:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE0AD:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE0AE:
      strcat(str, "GNB internal debug code");
      break;
    case 0xE0AF:
      strcat(str, "GNB internal debug code");
      break;
    case 0xEA00:
      strcat(str, "ABL Begin");
      break;
    case 0xEA01:
      strcat(str, "ABL End");
      break;
    case 0xEA10:
      strcat(str, "ABL Debug Synchronization");
      break;
    case 0xE0B0:
      strcat(str, "Abl1Begin");
      break;
    case 0xE0B1:
      strcat(str, "ABL 1 Initialization");
      break;
    case 0xE0B2:
      strcat(str, "ABL 1 DF Early");
      break;
    case 0xE0B3:
      strcat(str, "ABL 1 DF Pre Training");
      break;
    case 0xE0B4:
      strcat(str, "ABL 1 Debug Synchronization");
      break;
    case 0xE0B5:
      strcat(str, "ABL 1 Error Detected");
      break;
    case 0xE0B6:
      strcat(str, "ABL 1 Global memory error detected");
      break;
    case 0xE0B7:
      strcat(str, "ABL 1 End");
      break;
    case 0xE0B8:
      strcat(str, "ABL 2 Begin");
      break;
    case 0xE0B9:
      strcat(str, "ABL 2 Initialization");
      break;
    case 0xE0BA:
      strcat(str, "ABL 2 After Training");
      break;
    case 0xE0BB:
      strcat(str, "ABL 2 Debug Synchronization");
      break;
    case 0xE0BC:
      strcat(str, "ABL 2 Error detected");
      break;
    case 0xE0BD:
      strcat(str, "ABL 2 Global memory error detected");
      break;
    case 0xE0BE:
      strcat(str, "ABL 2 End");
      break;
    case 0xE0BF:
      strcat(str, "ABL 3 Begin");
      break;
    case 0xE0C0:
      strcat(str, "ABL 3 Initialziation");
      break;
    case 0xE1C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 1");
      break;
    case 0xB1C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 1 Warning");
      break;
    case 0xF1C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 2 Error");
      break;
    case 0xE2C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 2");
      break;
    case 0xB2C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 2 Warning");
      break;
    case 0xF2C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 2 Error");
      break;
    case 0xE3C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 3");
      break;
    case 0xB3C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 3 Warning");
      break;
    case 0xF3C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 3 Error");
      break;
    case 0xE4C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 4");
      break;
    case 0xB4C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 4 Warning");
      break;
    case 0xF4C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 4 Error");
      break;
    case 0xE5C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 5");
      break;
    case 0xB5C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 5 Warning");
      break;
    case 0xF5C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 5 Error");
      break;
    case 0xE6C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 6");
      break;
    case 0xB6C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 6 Warning");
      break;
    case 0xF6C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 6 Error");
      break;
    case 0xE7C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 7");
      break;
    case 0xE8C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 8");
      break;
    case 0xE9C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 9");
      break;
    case 0xF9C0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 9 Error");
      break;
    case 0xEAC0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 10");
      break;
    case 0xFAC0:
      strcat(str, "ABL 3 GMI/xGMI Initialization Stage 10 Error");
      break;
    case 0xE0C1:
      strcat(str, "Abl3ProgramUmcKeys");
      break;
    case 0xE0C2:
      strcat(str, "ABL 3 DF Finial Initalization");
      break;
    case 0xE0C3:
      strcat(str, "ABL 3 Execute Synchronization Function");
      break;
    case 0xE0C4:
      strcat(str, "ABL 3 Debug Synchronization Function");
      break;
    case 0xE0C5:
      strcat(str, "ABL 3 Error Detected");
      break;
    case 0xE0C6:
      strcat(str, "ABL 3 Global memroy error detected");
      break;
    case 0xE0C7:
      strcat(str, "ABL 4 Initialiation - cold boot");
      break;
    case 0xE0C8:
      strcat(str, "ABL 4 Memory test - cold boot");
      break;
    case 0xE0C9:
      strcat(str, "ABL 4 APOB Initialzation - cold boot");
      break;
    case 0xE0CA:
      strcat(str, "ABL 4 Finalize memory settings - cold boot");
      break;
    case 0xE0CB:
      strcat(str, "ABL 4 CPU Initialize Optimized Boot - cold boot");
      break;
    case 0xE0CC:
      strcat(str, "ABL 4 Gmi Pcie Training - cold boot");
      break;
    case 0xE0CD:
      strcat(str, "ABL 4 Cold boot End");
      break;
    case 0xE0CE:
      strcat(str, "ABL 4 Initialization - Resume boot");
      break;
    case 0xE0CF:
      strcat(str, "ABL 4 Resume End");
      break;
    case 0xE0D0:
      strcat(str, "ABL 4 End Cold/Resume boot");
      break;
    case 0xE0D1:
      strcat(str, "ABL 2 memory initialization");
      break;
    case 0xE0D2:
      strcat(str, "ABL 3 memory initialization");
      break;
    case 0xE0D3:
      strcat(str, "ABL 3 End");
      break;
    case 0xE0D4:
      strcat(str, "ABL 1 Enter Memory Flow");
      break;
    case 0xE0D5:
      strcat(str, "Memorry flow memory clock synchronization");
      break;
    case 0xE0E0:
      strcat(str, "Before IDS calls out to get IDS data");
      break;
    case 0xE0E1:
      strcat(str, "After IDS calls out to get IDS data");
      break;
    case 0xE0F9:
      strcat(str, "Failed PMU training.");
      break;
    case 0xE0FA:
      strcat(str, "End of phase 1 memory code");
      break;
    case 0xE0FB:
      strcat(str, "End of phase 2 memory code");
      break;
    case 0xE0FC:
      strcat(str, "Abl0Begin");
      break;
    case 0xE0FD:
      strcat(str, "ABL 0 End");
      break;
    case 0xE0FE:
      strcat(str, "Abl0 Begin with Fatal Mode");
      break;
    case 0xE0FF:
      strcat(str, "ABL 0 End with Fatal Mode");
      break;
    case 0xE100:
      strcat(str, "ABL 7 End");
      break;
    case 0xE101:
      strcat(str, "ABL 7 Resume boot");
      break;
    case 0xE102:
      strcat(str, "ABL 6 End");
      break;
    case 0xE103:
      strcat(str, "ABL 6 Initialization");
      break;
    case 0xE104:
      strcat(str, "End of phase 1b memory code");
      break;
    case 0xE105:
      strcat(str, "ABL 1b memory initialization");
      break;
    case 0xE106:
      strcat(str, "ABL 6 Global memroy error detected");
      break;
    case 0xE107:
      strcat(str, "ABL 1b Debug Synchronization Function");
      break;
    case 0xE108:
      strcat(str, "ABL 4b Debug Synchronization Function");
      break;
    case 0xE109:
      strcat(str, "AblbBegin");
      break;
    case 0xE10A:
      strcat(str, "Ab4bBegin");
      break;
    case 0xE10B:
      strcat(str, "BSP encountered HMAC fail on APOB Header");
      break;
    case 0xE10C:
      strcat(str, "ABL 18 End");
      break;
    case 0xE10D:
      strcat(str, "ABL 18 Resume boot");
      break;
    case 0xE10E:
      strcat(str, "ABL 15 End");
      break;
    case 0xE10F:
      strcat(str, "ABL 15 Initialization");
      break;
    case 0xE110:
      strcat(str, "Before UMC based device initialization");
      break;
    case 0xE111:
      strcat(str, "After UMC based device initialization");
      break;
    case 0xE2A0:
      strcat(str, "ABL Error General ASSERT");
      break;
    case 0xE2A1:
      strcat(str, "Unknown Error");
      break;
    case 0xE2A3:
      strcat(str, "ABL Error Log Inig Error");
      break;
    case 0xE2A4:
      strcat(str, "ABL Error for On DIMM thermal Heap allocation error");
      break;
    case 0xE2A5:
      strcat(str, "ABL Error for memory test error");
      break;
    case 0xE2A6:
      strcat(str, "ABL Error while executing memory test error");
      break;
    case 0xE2A7:
      strcat(str, "ABL Error DDR Post Packge Repair Mem Auto Heap Alloc error");
      break;
    case 0xE2A8:
      strcat(str, "ABL Error for DDR Post Package repair Apob Heap Alloc error");
      break;
    case 0xE2A9:
      strcat(str, "ABL Error for DDR Post Package Repair No PPR Table Heap Aloc error");
      break;
    case 0xE2AA:
      strcat(str, "ABL Error for Ecc Mem Auto Aloc Error error");
      break;
    case 0xE2AB:
      strcat(str, "ABL Error for Soc Scan Heap Aloc error");
      break;
    case 0xE2AC:
      strcat(str, "ABL Error for Soc Scan No Die error");
      break;
    case 0xE2AD:
      strcat(str, "ABL Error for Nb Tech Heap Aloc error");
      break;
    case 0xE2AE:
      strcat(str, "ABL Error for No Nb Constructor error");
      break;
    case 0xE2B0:
      strcat(str, "ABL Error for No Tech Constructor error");
      break;
    case 0xE2B1:
      strcat(str, "ABL Error for ABL1b Auto Alocation error");
      break;
    case 0xE2B2:
      strcat(str, "ABL Error for ABL1b No NB Constructor error");
      break;
    case 0xE2B3:
      strcat(str, "ABL Error for ABL2 No Nb Constructor error");
      break;
    case 0xE2B4:
      strcat(str, "ABL Error for ABL3 Auto Allocation error");
      break;
    case 0xE2B5:
      strcat(str, "ABL Error for ABL3 No Nb Constructor error");
      break;
    case 0xE2B6:
      strcat(str, "ABL Error for ABL1b General error");
      break;
    case 0xE2B7:
      strcat(str, "ABL Error for ABL2 General error");
      break;
    case 0xE2B8:
      strcat(str, "ABL Error for ABL3 General error");
      break;
    case 0xE2B9:
      strcat(str, "ABL Error for Get Target Speed error");
      break;
    case 0xE2BA:
      strcat(str, "ABL Error for Flow P1 Family Support error");
      break;
    case 0xE2BB:
      strcat(str, "ABL Error for No Valid Ddr4 Dimms error");
      break;
    case 0xE2BC:
      strcat(str, "ABL Error for No Dimm Present error");
      break;
    case 0xE2BD:
      strcat(str, "ABL Error for Flow P2 Family Supprot error");
      break;
    case 0xE2BE:
      strcat(str, "ABL Error for Heap Deallocation for PMU Sram Msg Block error");
      break;
    case 0xE2BF:
      strcat(str, "ABL Error for DDR Recovery error");
      break;
    case 0xEBC0:
      strcat(str, "ABL Error for RRW Test error");
      break;
    case 0xE2C1:
      strcat(str, "ABL Error for On Die Thermal error");
      break;
    case 0xE2C2:
      strcat(str, "ABL Error for Heap Allocation For Dct Struct Amd Ch Def structure error");
      break;
    case 0xE2C3:
      strcat(str, "ABL Error for Heap Allocation for PMU SRAM Msg block error");
      break;
    case 0xE2C4:
      strcat(str, "ABL Error for Heap Phy PLL lock Flure error");
      break;
    case 0xE2C5:
      strcat(str, "ABL Error for Pmu Training error");
      break;
    case 0xE2C6:
      strcat(str, "ABL Error for Failure to Load or Verify PMU FW error");
      break;
    case 0xE2C7:
      strcat(str, "ABL Error for Allocate for PMU SRAM Msg Block No Init error");
      break;
    case 0xE2C8:
      strcat(str, "ABL Error for Failure BIOS PMU FW Mismatch AGESA PMU FW version error");
      break;
    case 0xE2C9:
      strcat(str, "ABL Error for Agesa memory test error");
      break;
    case 0xE2CA:
      strcat(str, "ABL Error for Deallocate for PMU SRAM Msg Block error");
      break;
    case 0xE2CB:
      strcat(str, "ABL Error for Module Type Mismatch RDIMM error");
      break;
    case 0xE2CC:
      strcat(str, "ABL Error for Module type Mismatch LRDIMM error");
      break;
    case 0xE2CD:
      strcat(str, "ABL Error for MEm Auto NVDIM error");
      break;
    case 0xE2CE:
      strcat(str, "ABL Error for Unknowm Responce error");
      break;
    case 0xE2CF:
      strcat(str, "ABL Error for Over Clock Error RRW Test Results Error");
      break;
    case 0xE2D0:
      strcat(str, "ABL Error for Over Clock Error PMU Training Error");
      break;
    case 0xE2D1:
      strcat(str, "ABL Error for ABL1 General Error");
      break;
    case 0xE2D2:
      strcat(str, "ABL Error for ABL2 General Error");
      break;
    case 0xE2D3:
      strcat(str, "ABL Error for ABL3 General Error");
      break;
    case 0xE2D4:
      strcat(str, "ABL Error for ABL4 General Error");
      break;
    case 0xE2D5:
      strcat(str, "ABL Error over clock Mem Init Error");
      break;
    case 0xE2D6:
      strcat(str, "ABL Error over clock Mem Other Error");
      break;
    case 0xE2D7:
      strcat(str, "ABL Error for ABL6 General Error");
      break;
    case 0xE2D8:
      strcat(str, "ABL Error Event Log Error");
      break;
    case 0xE2D9:
      strcat(str, "ABL Error FATAL ABL1 Log Error");
      break;
    case 0xE2DA:
      strcat(str, "ABL Error FATAL ABL2 Log Error");
      break;
    case 0xE2DB:
      strcat(str, "ABL Error FATAL ABL3 Log Error");
      break;
    case 0xE2DC:
      strcat(str, "ABL Error FATAL ABL4 Log Error");
      break;
    case 0xE2DD:
      strcat(str, "ABL Error Slave Sync function execution Error");
      break;
    case 0xE2DE:
      strcat(str, "ABL Error Slave Sync communicaton with data set to master Error");
      break;
    case 0xE2DF:
      strcat(str, "ABL Error Slave broadcast communication from master to slave Error");
      break;
    case 0xE2E0:
      strcat(str, "ABL Error FATAL ABL6 Log Error");
      break;
    case 0xE2E1:
      strcat(str, "ABL Error Slave Offline Error");
      break;
    case 0xE2E2:
      strcat(str, "ABL Error Slave Informs Master Error Info Error");
      break;
    case 0xE2E3:
      strcat(str, "ABL Error Error Heap Locate for PMU SRAM Msg Block Error");
      break;
    case 0xE2E4:
      strcat(str, "ABL Error ABL2 Auto Error");
      break;
    case 0xE2E5:
      strcat(str, "ABL Error Flow P3 Family support Error");
      break;
    // case 0xE2E5:
    //   strcat(str, "ABL Error Abl 4 Gen Error");
    //   break;
    case 0xE2E7:
      strcat(str, "ABL Error Mix RDIMM & LRDIMM in a channel");
      break;
    case 0xE2E8:
      strcat(str, "ABL Error Memory Present on Disconnected Channel");
      break;
    case 0xE2EB:
      strcat(str, "ABL Error MBIST Heap Allocation Error");
      break;
    case 0xE2EC:
      strcat(str, "ABL Error MBIST Results Error");
      break;
    case 0xE2ED:
      strcat(str, "ABL Error NO Dimm Smcus Info Error");
      break;
    case 0xE2EE:
      strcat(str, "ABL Error Por Max Freq Table Error");
      break;
    case 0xE2EF:
      strcat(str, "ABL Error Unsupproted DIMM Config Error");
      break;
    case 0xE2F0:
      strcat(str, "ABL Error No Ps Table Error");
      break;
    case 0xE2F1:
      strcat(str, "ABL Error Cad Bus Timing Not Found Error");
      break;
    case 0xE2F2:
      strcat(str, "ABL Error Data Bus Timing Not Found Error");
      break;
    case 0xE2F3:
      strcat(str, "ABL Error LrDIMM IBT Not Found Error");
      break;
    case 0xE2F4:
      strcat(str, "ABL Error Unsupprote Dimm Config Max Freq Error Error");
      break;
    case 0xE2F5:
      strcat(str, "ABL Error Mr0 Not Found Error");
      break;
    case 0xE2F6:
      strcat(str, "ABL Error Obt Pattern Not found Error");
      break;
    case 0xE2F7:
      strcat(str, "ABL Error Rc10 Op Speed Not FOund Error");
      break;
    case 0xE2F8:
      strcat(str, "ABL Error Rc2 Ibt Not Found Error");
      break;
    case 0xE2F9:
      strcat(str, "ABL Error Rtt Not Found Error");
      break;
    case 0xE2FA:
      strcat(str, "ABL Error Checksum ReStrt Results Error");
      break;
    case 0xE2FB:
      strcat(str, "ABL Error No Chipselect Results Error");
      break;
    case 0xE2FC:
      strcat(str, "ABL Error No Common Cas Latency Results Error");
      break;
    case 0xE2FD:
      strcat(str, "ABL Error Cas Latecncy exceeds Taa Max Error");
      break;
    case 0xE2FE:
      strcat(str, "ABL Error Nvdimm Arm Missmatch Power Policy Error");
      break;
    case 0xE2FF:
      strcat(str, "ABL Error Nvdimm Arm Missmatch Power Source Error");
      break;
    case 0xE300:
      strcat(str, "ABL Error ABL 1 Mem Init Error");
      break;
    case 0xE301:
      strcat(str, "ABL Error ABL 2 Mem Init Error");
      break;
    case 0xE302:
      strcat(str, "ABL Error ABL 4 Mem Init Error");
      break;
    case 0xE303:
      strcat(str, "ABL Error ABL 6 Mem Init Error");
      break;
    case 0xE304:
      strcat(str, "ABL Error ABL 1 error repor Error");
      break;
    case 0xE305:
      strcat(str, "ABL Error ABL 2 error repor Error");
      break;
    case 0xE306:
      strcat(str, "ABL Error ABL 3 error repor Error");
      break;
    case 0xE307:
      strcat(str, "ABL Error ABL 4 error repor Error");
      break;
    case 0xE308:
      strcat(str, "ABL Error ABL 6 error repor Error");
      break;
    case 0xE30A:
      strcat(str, "ABL Error message slave sync function execution Error");
      break;
    case 0xE30B:
      strcat(str, "ABL Error slave offline Error");
      break;
    case 0xE30C:
      strcat(str, "ABL Error Sync Master Error");
      break;
    case 0xE30D:
      strcat(str, "ABL Error Slave Informs Master Info Message Error");
      break;
    case 0xE30E:
      strcat(str, "ABL Error Mix Hynix LRDIMM with other vendor LRDIMM in a channel");
      break;
    case 0xE30F:
      strcat(str, "ABL Error General Assert Error");
      break;
    case 0xE310:
      strcat(str, "ABL ErrorNo Dimms On Any Channel in sysem");
      break;
    case 0xE311:
      strcat(str, "ABL Error for Shared Heap Aloc error");
      break;
    case 0xE312:
      strcat(str, "ABL Error for Main Heap Aloc error");
      break;
    case 0xE313:
      strcat(str, "ABL Error for Shared Heap loc error");
      break;
    case 0xE314:
      strcat(str, "ABL Error for Main Heap loc error");
      break;
    case 0xE316:
      strcat(str, "ABL Error No memory available in system");
      break;
    case 0xE320:
      strcat(str, "ABL Error Mixed Ecc and Non-Ecc DIMM in a channel");
      break;
    case 0xE321:
      strcat(str, "ABL Error Mixed 3DS and Non-3DS DIMM in a channel");
      break;
    case 0xE322:
      strcat(str, "ABL Error Mixed x4 and x8 DIMM in a channel");
      break;
    case 0xE323:
      strcat(str, "ABL Memory MBIST Rrw default test");
      break;
    case 0xE324:
      strcat(str, "ABL Memory MBIST Interface test");
      break;
    case 0xE325:
      strcat(str, "ABL Memory MBIST DataEye");
      break;
    case 0xE326:
      strcat(str, "ABL Memory Post Package Repair");
      break;
    case 0xE327:
      strcat(str, "ABL Error S0i3 DF restore buffer Error");
      break;
    case 0xE328:
      strcat(str, "ABL Error CPU OPN Mismatch in case of Multi Socket population");
      break;
    case 0xE329:
      strcat(str, "Recoverable APCB Checksum Error");
      break;
    case 0xE32A:
      strcat(str, "Fatal APCB Checksum Error");
      break;
    case 0xE32B:
      strcat(str, "ABL Error BIST Failure");
      break;
    case 0xE32C:
      strcat(str, "ABL Error DDR Type Mismatch DDR5 Error");
      break;
    case 0xE32D:
      strcat(str, "ABL Error Mix DIMM with different ECC bit size in a channel");
      break;
    case 0xE32E:
      strcat(str, "ABL Error No ability to recover I2C bus without power cycling the platform");
      break;
    case 0xE32F:
      strcat(str, "ABL Error I2C reset failure");
      break;
    case 0xE330:
      strcat(str, "ABL Error DDR Module Type Mismatch");
      break;
    case 0xE331:
      strcat(str, "ABL Error PMIC Error");
      break;
    case 0xE332:
      strcat(str, "ABL Error Incompatible OPN");
      break;
    case 0xE333:
      strcat(str, "ABL Error No ability to recover I3C bus without power cycling the platform");
      break;
    case 0xE334:
      strcat(str, "ABL Error I3C reset failure");
      break;
    case 0xE335:
      strcat(str, "ABL Error Absence of either or both AC-Power GPIO or WLAN GPIO Apcb Data");
      break;
    case 0xE336:
      strcat(str, "ABL Memory Heal BIST Start");
      break;
    case 0xE337:
      strcat(str, "ABL Memory Heal BIST End");
      break;
    case 0xE338:
      strcat(str, "ABL Memory Heal BIST Write");
      break;
    case 0xE339:
      strcat(str, "ABL Memory Heal BIST Read");
      break;
    case 0xE33A:
      strcat(str, "ABL Memory Heal BIST Repair");
      break;
    case 0xE33B:
      strcat(str, "Timeout at PMFW SwitchToMemoryTrainingState");
      break;
    case 0xE33C:
      strcat(str, "DIMM with RCD Montage version B1 detected");
      break;
    case 0xE33D:
      strcat(str, "ABL DDR PMU training complete");
      break;
    case 0xE33E:
      strcat(str, "Timeout at PMFW SwitchToMemoryTrainingState");
      break;
    case 0xE33F:
      strcat(str, "DIMM with TI PMIC revision 1.1 (XTPS) detected");
      break;
    case 0xE343:
      strcat(str, "ABL DDR DIMM SPD verify CRC failure");
      break;
    case 0xE344:
      strcat(str, "ABL DDR DIMM SPD Invalid Field Value");
      break;
    case 0xE350:
      strcat(str, "ABL DDR Runtime Post Package Repair Begin");
      break;
    case 0xE351:
      strcat(str, "ABL DDR Runtime Post Package Repair End");
      break;
    case 0xE60B:
      strcat(str, "ABL Functions execute Before");
      break;
    case 0xE60C:
      strcat(str, "ABL Functions execute");
      break;
    case 0xEFFF:
      strcat(str, "EndAgesas");
      break;
    default:
      strcat(str, "ABL Unknown");
  }
}

void
pal_parse_amd_posrt_80(uint32_t post_code, char *str )
{
  switch (post_code & 0xff){
    case 0x00:
      strcat(str, "General - Success");
      break;
    case 0x01:
      strcat(str, "Generic Error Code");
      break;
    case 0x02:
      strcat(str, "Generic Memory Error");
      break;
    case 0x03:
      strcat(str, "Buffer Overflow");
      break;
    case 0x04:
      strcat(str, "Invalid Parameter(s)");
      break;
    case 0x05:
      strcat(str, "Invalid Data Length");
      break;
    case 0x06:
      strcat(str, "Data Alignment Error");
      break;
    case 0x07:
      strcat(str, "Null Pointer Error");
      break;
    case 0x08:
      strcat(str, "Unsupported Function");
      break;
    case 0x09:
      strcat(str, "Invalid Service ID");
      break;
    case 0x0A:
      strcat(str, "Invalid Address");
      break;
    case 0x0B:
      strcat(str, "Out of Resource Error");
      break;
    case 0x0C:
      strcat(str, "Timeout");
      break;
    case 0x0D:
      strcat(str, "data abort exception");
      break;
    case 0x0E:
      strcat(str, "prefetch abort exception");
      break;
    case 0x0F:
      strcat(str, "Out of Boundary Condition Reached");
      break;
    case 0x10:
      strcat(str, "Data corruption");
      break;
    case 0x11:
      strcat(str, "Invalid command");
      break;
    case 0x12:
      strcat(str, "The package type provided by BR is incorrect");
      break;
    case 0x13:
      strcat(str, "Failed to retrieve FW header during FW validation");
      break;
    case 0x14:
      strcat(str, "Key size not supported");
      break;
    case 0x15:
      strcat(str, "Agesa0 verification error");
      break;
    case 0x16:
      strcat(str, "SMU FW verification error");
      break;
    case 0x17:
      strcat(str, "OEM SINGING KEY verification error");
      break;
    case 0x18:
      strcat(str, "Generic FW Validation error");
      break;
    case 0x19:
      strcat(str, "RSA operation fail - bootloader");
      break;
    case 0x1A:
      strcat(str, "CCP Passthrough operation failed - internal status");
      break;
    case 0x1B:
      strcat(str, "AES operation fail");
      break;
    case 0x1C:
      strcat(str, "CCP state save failed");
      break;
    case 0x1D:
      strcat(str, "CCP state restore failed");
      break;
    case 0x1E:
      strcat(str, "SHA256/384 operation fail - internal status");
      break;
    case 0x1F:
      strcat(str, "ZLib Decompression operation fail");
      break;
    case 0x20:
      strcat(str, "HMAC-SHA256/384 operation fail - internal status");
      break;
    case 0x21:
      strcat(str, "Booted from boot source not recognized by PSP");
      break;
    case 0x22:
      strcat(str, "PSP directory entry not found");
      break;
    case 0x23:
      strcat(str, "PSP failed to set the write enable latch");
      break;
    case 0x24:
      strcat(str, "PSP timed out because spirom took too long");
      break;
    case 0x25:
      strcat(str, "Cannot find BIOS directory");
      break;
    case 0x26:
      strcat(str, "SpiRom is not valid");
      break;
    case 0x27:
      strcat(str, "slave die has different security state from master");
      break;
    case 0x28:
      strcat(str, "SMI interface init failure");
      break;
    case 0x29:
      strcat(str, "SMI interface generic error");
      break;
    case 0x2A:
      strcat(str, "invalid die ID executes MCM related function");
      break;
    case 0x2B:
      strcat(str, "invalid MCM configuration table read from bootrom");
      break;
    case 0x2C:
      strcat(str, "Valid boot mode wasn't detected");
      break;
    case 0x2D:
      strcat(str, "NVStorage init failure");
      break;
    case 0x2E:
      strcat(str, "NVStorage generic error");
      break;
    case 0x2F:
      strcat(str, "MCM 'error' to indicate slave has more data to send");
      break;
    case 0x30:
      strcat(str, "MCM error if data size exceeds 32B");
      break;
    case 0x31:
      strcat(str, "Invalid client id for SVC MCM call");
      break;
    case 0x32:
      strcat(str, "MCM slave status register contains bad bits");
      break;
    case 0x33:
      strcat(str, "MCM call was made in a single die environment");
      break;
    case 0x34:
      strcat(str, "PSP secure mapped to invalid segment");
      break;
    case 0x35:
      strcat(str, "No physical x86 cores were found on die");
      break;
    case 0x36:
      strcat(str, "Insufficient space for secure OS (range of free SRAM to SVC stack base)");
      break;
    case 0x37:
      strcat(str, "SYSHUB mapping memory target type is not supported");
      break;
    case 0x38:
      strcat(str, "Attempt to unmap permanently mapped TLB to PSP secure region");
      break;
    case 0x39:
      strcat(str, "Unable to map an SMN address to AXI space");
      break;
    case 0x3A:
      strcat(str, "Unable to map a SYSHUB address to AXI space");
      break;
    case 0x3B:
      strcat(str, "The count of CCXs or cores provided by bootrom is not consistent");
      break;
    case 0x3C:
      strcat(str, "Uncompressed image size doesn't match value in compressed header");
      break;
    case 0x3D:
      strcat(str, "Compressed option used in case where not supported");
      break;
    case 0x3E:
      strcat(str, "Fuse info on all dies don't match");
      break;
    case 0x3F:
      strcat(str, "PSP sent message to SMU; SMU reported an error");
      break;
    case 0x40:
      strcat(str, "Function RunPostX86ReleaseUnitTests failed in memcmp()");
      break;
    case 0x41:
      strcat(str, "Interface between PSP to SMU not available");
      break;
    case 0x42:
      strcat(str, "Timer wait parameter too large");
      break;
    case 0x43:
      strcat(str, "Test harness module reported an error");
      break;
    case 0x44:
      strcat(str, "x86 wrote C2PMSG_0 interrupting PSP, but the command has an invalid format");
      break;
    case 0x45:
      strcat(str, "Failed to read from SPI the Bios Directory or Bios Combo Directory");
      break;
    case 0x46:
      strcat(str, "Failed to find FW entry in SPL Table");
      break;
    case 0x47:
      strcat(str, "Failed to read the combo bios header");
      break;
    case 0x48:
      strcat(str, "SPL version missmatch");
      break;
    case 0x49:
      strcat(str, "Error in Validate and Loading AGESA APOB SVC call");
      break;
    case 0x4A:
      strcat(str, "Correct fuse bits for DIAG_BL loading not set");
      break;
    case 0x4B:
      strcat(str, "The UmcProgramKeys() function was not called by AGESA");
      break;
    case 0x4C:
      strcat(str, "Unconditional Unlock based on serial numbers failure");
      break;
    case 0x4D:
      strcat(str, "Syshub register programming mismatch during readback");
      break;
    case 0x4E:
      strcat(str, "Family ID in MP0_SFUSE_SEC[7:3] not correct");
      break;
    case 0x4F:
      strcat(str, "An operation was invoked that can only be performed by the GM");
      break;
    case 0x50:
      strcat(str, "Failed to acquire host controller semaphore to claim ownership of SMB");
      break;
    case 0x51:
      strcat(str, "Timed out waiting for host to complete pending transactions");
      break;
    case 0x52:
      strcat(str, "Timed out waiting for slave to complete pending transactions");
      break;
    case 0x53:
      strcat(str, "Unable to kill current transaction on host to force idle");
      break;
    case 0x54:
      strcat(str, "One of: Illegal command, Unclaimed cycle, or Host time out");
      break;
    case 0x55:
      strcat(str, "An smbus transaction collision detected operation restarted");
      break;
    case 0x56:
      strcat(str, "Transaction failed to be started or processed by host or not completed");
      break;
    case 0x57:
      strcat(str, "An unsolicited smbus interrupt was received");
      break;
    case 0x58:
      strcat(str, "An attempt to send an unsupported PSP-SMU message was made");
      break;
    case 0x59:
      strcat(str, "An error/data corruption detected on response from SMU for sent msg");
      break;
    case 0x5A:
      strcat(str, "MCM Steady-state unit test failed");
      break;
    case 0x5B:
      strcat(str, "S3 Enter failed");
      break;
    case 0x5C:
      strcat(str, "AGESA BL did not set PSP SMU reserved addresses via SVC call");
      break;
    case 0x5D:
      strcat(str, "Reserved PSP/SMU memory region is invalid");
      break;
    case 0x5E:
      strcat(str, "CcxSecBisiEn not set in fuse RAM");
      break;
    case 0x5F:
      strcat(str, "Received an unexpected result");
      break;
    case 0x60:
      strcat(str, "VMG Storage Init failed");
      break;
    case 0x61:
      strcat(str, "failure in mbedTLS user app");
      break;
    case 0x62:
      strcat(str, "An error occured whilst attempting to SMN map a fuse register");
      break;
    case 0x63:
      strcat(str, "Fuse burn sequence/operation failed due to internal SOC error");
      break;
    case 0x64:
      strcat(str, "Fuse sense operation timed out");
      break;
    case 0x65:
      strcat(str, "Fuse burn sequence/operation timed out waiting for burn done");
      break;
    case 0x66:
      strcat(str, "The PMU FW Public key certificate loading or authentication fails");
      break;
    case 0x67:
      strcat(str, "This PSP FW was revoked");
      break;
    case 0x68:
      strcat(str, "The platform model/vendor id fuse is not matching the BIOS public key token");
      break;
    case 0x69:
      strcat(str, "The BIOS OEM public key of the BIOS was revoked for this platform");
      break;
    case 0x6A:
      strcat(str, "PSP level 2 directory not match expected value");
      break;
    case 0x6B:
      strcat(str, "BIOS level 2 directory not match expected value");
      break;
    case 0x6C:
      strcat(str, "Reset image not found");
      break;
    case 0x6D:
      strcat(str, "Generic error indicating the CCP HAL initialization failed");
      break;
    case 0x6E:
      strcat(str, "failure to copy NVRAM to DRAM");
      break;
    case 0x6F:
      strcat(str, "Invalid key usage flag");
      break;
    case 0x70:
      strcat(str, "Unexpected fuse set");
      break;
    case 0x71:
      strcat(str, "RSMU signaled a security violation");
      break;
    case 0x72:
      strcat(str, "Error programming the WAFL PCS registers");
      break;
    case 0x73:
      strcat(str, "Error setting wafl PCS threshold value");
      break;
    case 0x74:
      strcat(str, "Error loading OEM trustlets");
      break;
    case 0x75:
      strcat(str, "Recovery mode accross all dies is not sync'd");
      break;
    case 0x76:
      strcat(str, "Uncorrectable WAFL error detected");
      break;
    case 0x77:
      strcat(str, "Fatal MP1 error detected");
      break;
    case 0x78:
      strcat(str, "Bootloader failed to find OEM signature");
      break;
    case 0x79:
      strcat(str, "Error copying BIOS to DRAM");
      break;
    case 0x7A:
      strcat(str, "Error validating BIOS image signature");
      break;
    case 0x7B:
      strcat(str, "OEM Key validation failed");
      break;
    case 0x7C:
      strcat(str, "Platform Vendor ID and/or Model ID binding violation");
      break;
    case 0x7D:
      strcat(str, "Bootloader detects BIOS request boot from SPI-ROM which is unsupported for PSB");
      break;
    case 0x7E:
      strcat(str, "Requested fuse is already blown reblow will cause ASIC malfunction");
      break;
    case 0x7F:
      strcat(str, "Error with actual fusing operation");
      break;
    case 0x80:
      strcat(str, "(Local Master PSP on P1 socket) Error reading fuse info");
      break;
    case 0x81:
      strcat(str, "(Local Master PSP on P1 socket) Platform Vendor ID and/or Model ID binding violation");
      break;
    case 0x82:
      strcat(str, "(Local Master PSP on P1 socket) Requested fuse is already blown, reblow will cause ASIC malfunction");
      break;
    case 0x83:
      strcat(str, "(Local Master PSP on P1 socket) Error with actual fusing operation");
      break;
    case 0x84:
      strcat(str, "SEV FW Rollback attempt is detected");
      break;
    case 0x85:
      strcat(str, "SEV download FW command fail to broadcase and clear the IsInSRAM field on slave dies");
      break;
    case 0x86:
      strcat(str, "Agesa error injection failure");
      break;
    case 0x87:
      strcat(str, "Uncorrectable TWIX error detected");
      break;
    case 0x88:
      strcat(str, "Error programming the TWIX PCS registers");
      break;
    case 0x89:
      strcat(str, "Error setting TWIX PCS threshold value");
      break;
    case 0x8A:
      strcat(str, "SW CCP queue is full cannot add more entries");
      break;
    case 0x8B:
      strcat(str, "CCP command description syntax error detected from input");
      break;
    case 0x8C:
      strcat(str, "Return value stating that the command has not yet be scheduled");
      break;
    case 0x8D:
      strcat(str, "The command is scheduled and being worked on");
      break;
    case 0x8E:
      strcat(str, "The DXIO PHY SRAM Public key certificate loading or authentication fails");
      break;
    case 0x8F:
      strcat(str, "fTPM binary size exceeds limit allocated in Private DRAM need to increase the limit");
      break;
    case 0x90:
      strcat(str, "The TWIX link for a particular CCD is not trained Fatal error");
      break;
    case 0x91:
      strcat(str, "Security check failed (not all dies are in same security state)");
      break;
    case 0x92:
      strcat(str, "FW type mismatch between the requested FW type and the FW type embedded in the FW binary header");
      break;
    case 0x93:
      strcat(str, "SVC call input parameter address violation");
      break;
    case 0x94:
      strcat(str, "Firmware Compatibility Level mismatch");
      break;
    case 0x95:
      strcat(str, "Bad status returned by I2CKnollCheck");
      break;
    case 0x96:
      strcat(str, "NACK to general call (no device on Knoll I2C bus)");
      break;
    case 0x97:
      strcat(str, "Null pointer passed to I2CKnollCheck");
      break;
    case 0x98:
      strcat(str, "Invalid device-ID found during Knoll authentication");
      break;
    case 0x99:
      strcat(str, "Error during Knoll/Prom key derivation");
      break;
    case 0x9A:
      strcat(str, "Null pointer passed to Crypto function");
      break;
    case 0x9B:
      strcat(str, "Error in checksum from wrapped Knoll/Prom keys");
      break;
    case 0x9C:
      strcat(str, "Knoll returned an invalid response to a command");
      break;
    case 0x9D:
      strcat(str, "Bootloader failed in Knoll Send Command function");
      break;
    case 0x9E:
      strcat(str, "No Knoll device found by verifying MAC");
      break;
    case 0x9F:
      strcat(str, "The maximum allowable error post code");
      break;
    case 0xA0:
      strcat(str, "Bootloader successfully entered C Main");
      break;
    case 0xA1:
      strcat(str, "Master initialized C2P / slave waited for master to init C2P");
      break;
    case 0xA2:
      strcat(str, "HMAC key successfully derived");
      break;
    case 0xA3:
      strcat(str, "Master got Boot Mode and sent boot mode to all slaves");
      break;
    case 0xA4:
      strcat(str, "SpiRom successfully initialized");
      break;
    case 0xA5:
      strcat(str, "BIOS Directory successfully read from SPI to SRAM");
      break;
    case 0xA6:
      strcat(str, "Early unlock check");
      break;
    case 0xA7:
      strcat(str, "Inline Aes key successfully derived");
      break;
    case 0xA8:
      strcat(str, "Inline-AES key programming is done");
      break;
    case 0xA9:
      strcat(str, "Inline-AES key wrapper derivation is done");
      break;
    case 0xAA:
      strcat(str, "Bootloader successfully loaded HW IP configuration values");
      break;
    case 0xAB:
      strcat(str, "Bootloader successfully programmed MBAT table");
      break;
    case 0xAC:
      strcat(str, "Bootloader successfully loaded SMU FW");
      break;
    case 0xAD:
      strcat(str, "progress code is available");
      break;
    case 0xAE:
      strcat(str, "User mode test Uapp completed successfully");
      break;
    case 0xAF:
      strcat(str, "Bootloader loaded Agesa0 from SpiRom");
      break;
    case 0xB0:
      strcat(str, "AGESA phase has completed");
      break;
    case 0xB1:
      strcat(str, "RunPostDramTrainingTests() completed successfully");
      break;
    case 0xB2:
      strcat(str, "SMU FW Successfully loaded to SMU Secure DRAM");
      break;
    case 0xB3:
      strcat(str, "Sent all required boot time messages to SMU");
      break;
    case 0xB4:
      strcat(str, "Validated and ran Security Gasket binary");
      break;
    case 0xB5:
      strcat(str, "UMC Keys generated and programmed");
      break;
    case 0xB6:
      strcat(str, "Inline AES key wrapper stored in DRAM");
      break;
    case 0xB7:
      strcat(str, "Completed FW Validation step");
      break;
    case 0xB8:
      strcat(str, "Completed FW Validation step");
      break;
    case 0xB9:
      strcat(str, "BIOS copy from SPI to DRAM complete");
      break;
    case 0xBA:
      strcat(str, "Completed FW Validation step");
      break;
    case 0xBB:
      strcat(str, "BIOS load process fully complete");
      break;
    case 0xBC:
      strcat(str, "Bootloader successfully release x86");
      break;
    case 0xBD:
      strcat(str, "Early Secure Debug completed");
      break;
    case 0xBE:
      strcat(str, "GetFWVersion command received from BIOS is completed");
      break;
    case 0xBF:
      strcat(str, "SMIInfo command received from BIOS is completed");
      break;
    case 0xC0:
      strcat(str, "Successfully entered WarmBootResume()");
      break;
    case 0xC1:
      strcat(str, "Successfully copied SecureOS image to SRAM");
      break;
    case 0xC2:
      strcat(str, "Successfully copied trustlets to PSP Secure Memory");
      break;
    case 0xC3:
      strcat(str, "About to jump to Secure OS (SBL about to copy and jump)");
      break;
    case 0xC4:
      strcat(str, "Successfully restored CCP and UMC state on S3 resume");
      break;
    case 0xC5:
      strcat(str, "PSP SRAM HMAC validated by Mini BL");
      break;
    case 0xC6:
      strcat(str, "About to jump to <t-base in Mini BL");
      break;
    case 0xC7:
      strcat(str, "VMG ECDH unit test started");
      break;
    case 0xC8:
      strcat(str, "VMG ECDH unit test passed");
      break;
    case 0xC9:
      strcat(str, "VMG ECC CDH primitive unit test started");
      break;
    case 0xCA:
      strcat(str, "VMG ECC CDH primitive unit test passed");
      break;
    case 0xCB:
      strcat(str, "VMG SP800-108 KDF-CTR HMAC unit test started");
      break;
    case 0xCC:
      strcat(str, "VMG SP800-108 KDF-CTR HMAC unit test passed");
      break;
    case 0xCD:
      strcat(str, "VMG LAUNCH_* test started");
      break;
    case 0xCE:
      strcat(str, "VMG LAUNCH_* test passed");
      break;
    case 0xCF:
      strcat(str, "MP1 has been taken out of reset, and executing SMUFW");
      break;
    case 0xD0:
      strcat(str, "PSP and SMU Reserved Addresses correct");
      break;
    case 0xD1:
      strcat(str, "Reached Naples steady-state WFI loop");
      break;
    case 0xD2:
      strcat(str, "Knoll device successfully initialized");
      break;
    case 0xD3:
      strcat(str, "32-byte RandOut successfully returned from Knoll");
      break;
    case 0xD4:
      strcat(str, "32-byte MAC successfully received from Knoll");
      break;
    case 0xD5:
      strcat(str, "Knoll device verified successfully");
      break;
    case 0xD6:
      strcat(str, "CNLI Keys generated and programmed");
      break;
    case 0xD7:
      strcat(str, "Enter recovery mode due to trustlet validation fail");
      break;
    case 0xD8:
      strcat(str, "Enter recovery mode due to OS validation fail");
      break;
    case 0xD9:
      strcat(str, "Enter recovery mode due to OEM public key not found");
      break;
    case 0xDA:
      strcat(str, "Enter recovery mode with header corruption");
      break;
    case 0xDB:
      strcat(str, "We should not treat this error as blocking");
      break;
    case 0xDC:
      strcat(str, "When same fw image type is already loaded in SRAM");
      break;
    case 0xDD:
      strcat(str, "Bootloader successfully hashed the OEM Truslet Key");
      break;
    case 0xDE:
      strcat(str, "Bootloader successfully loaded OEM Truslet Key");
      break;
    case 0xE0:
      strcat(str, "Unlock return");
      break;
    case 0xE2:
      strcat(str, "Token exipration reset triggered");
      break;
    case 0xE3:
      strcat(str, "Completed DXIO PHY SRAM FW key Validation step");
      break;
    case 0xE4:
      strcat(str, "MP1 firmware load to SRAM success");
      break;
    case 0xE5:
      strcat(str, "Bootloader read the MP1 SRAM successfully");
      break;
    case 0xE6:
      strcat(str, "Bootloader successfully reset MP1");
      break;
    case 0xE7:
      strcat(str, "DF init successfully done (in absence of AGESA)");
      break;
    case 0xE8:
      strcat(str, "UMC init successfully done (in absence of AGESA)");
      break;
    case 0xE9:
      strcat(str, "LX6 Boot ROM code ready");
      break;
    case 0xEA:
      strcat(str, "Bootloader successfully asserted LX6 reset");
      break;
    case 0xEB:
      strcat(str, "LX6 load to SRAM success");
      break;
    case 0xEC:
      strcat(str, "Bootloader successfully set LX6 reset vector to SRAM");
      break;
    case 0xED:
      strcat(str, "Bootloader successfully de-asserted LX6 reset");
      break;
    case 0xEE:
      strcat(str, "LX6 firmware is running and ready");
      break;
    case 0xEF:
      strcat(str, "Loading of S3 image done successfully");
      break;
    case 0xF0:
      strcat(str, "Bootloader successfully verify signed image using 4K/2K key");
      break;
    case 0XF1:
      strcat(str, "Bootloader identified as running on SP32P or multi-socket boot");
      break;
    case 0xF2:
      strcat(str, "Security Policy check successful (only in secure boot)");
      break;
    case 0xF3:
      strcat(str, "Bootloader successfully loaded SS3");
      break;
    case 0xF4:
      strcat(str, "Bootloader successfully load fTPM Driver");
      break;
    case 0xF5:
      strcat(str, "Bootloader successfully loaded sys_drv");
      break;
    case 0xF6:
      strcat(str, "Bootloader successfully loaded secure OS");
      break;
    case 0xF7:
      strcat(str, "Bootloader about to transfer control to secureOS");
      break;
    case 0xFC:
      strcat(str, "Fatal error reported by another socket");
      break;
    case 0xFD:
      strcat(str, "MP0 Data abort exception happened - PSP in debug unlockable state");
      break;
    case 0xFE:
      strcat(str, "Uncorrectable error happened - PSP in debug unlockable state");
      break;
    case 0xFF:
      strcat(str, "Bootloader sequence finished");
      break;
    default:
      strcat(str, "Bootloader Unknown");
  }
}

void
pal_parse_amd_post_code_helper(uint32_t post_code, char *str )
{
  uint16_t post_code_type = (post_code & 0xffff0000) >> 16;
  uint16_t post_code_num = (post_code & 0xffff);
  sprintf(str, "%08X: ", post_code);
  switch (post_code_type) {
    case 0x0000:
      pal_parse_amd_sec_pei_dxe(post_code_num, str);
      break;
    case 0xB000:
      pal_parse_amd_agesa(post_code_num, str);
      break;
    case 0xEA00:
      pal_parse_amd_abl(post_code_num, str);
      break;
    case 0xEE00:
      pal_parse_amd_posrt_80(post_code_num, str);
      break;
    case 0xDDEE:
      strcat(str,"DIMM Loop Error");
      break;
    default:
      strcat(str,"Unknown");
  }
  strcat(str,"\n");
}
#endif
