#include <string.h>
#include "rackmon_parser.h"


#define PRINTDESC(reg, desc) \
    case reg: \
      buf_printf(wb, "  <0x%04X> %-32s : ", reg, desc); \
    break

static int table_description(write_buf_t *wb,int address,int value);
int rackmon_print_info(write_buf_t *wb, psu_datastore_t *data, uint16_t num_intervals){

  //Address 101R RSPP
  // RR - rack number 0-2
  // S  - shelf number 0-1
  // PP - PSU number 0-3
  buf_printf(wb, "PSU Addr %02x\n",data->addr);
  buf_printf(wb, " - Rack%d\n",  (data->addr>>3 & 0x3) + 1 );
  buf_printf(wb, " - Shelf%d\n", (data->addr>>2 & 0x1) + 1 );
  buf_printf(wb, " - PSU%d\n",   (data->addr & 0x3) + 1 );

  for (int i = 0; i < num_intervals; i++)
  {
    uint32_t time;
    float fvalue = 0, N = 0;
    int value = 0;
    reg_range_data_t *rd = &data->range_data[i];
    char *mem_pos = rd->mem_begin;
    int reg_addr = rd->i->begin;
    uint16_t flags = rd->i->flags;
    uint16_t len = rd->i->len;
    switch( reg_addr )
    {
      PRINTDESC(0x00, "MFG_MODEL");
      PRINTDESC(0x10, "MFG_DATE");
      PRINTDESC(0x20, "FB part#");
      PRINTDESC(0x30, "HW Revision");
      PRINTDESC(0x38, "FW Revision");
      PRINTDESC(0x40, "MFR_SERIAL");
      PRINTDESC(0x60, "Workorder #");
      PRINTDESC(0x68, "PSU Status register");
      PRINTDESC(0x69, "Battery Status register");
      PRINTDESC(0x6B, "BBU Battery Mode");
      PRINTDESC(0x6C, "BBU Battery Status");
      PRINTDESC(0x6D, "BBU Cell Voltage 1");
      PRINTDESC(0x6E, "BBU Cell Voltage 2");
      PRINTDESC(0x6F, "BBU Cell Voltage 3");
      PRINTDESC(0x70, "BBU Cell Voltage 4");
      PRINTDESC(0x71, "BBU Cell Voltage 5");
      PRINTDESC(0x72, "BBU Cell Voltage 6");
      PRINTDESC(0x73, "BBU Cell Voltage 7");
      PRINTDESC(0x74, "BBU Cell Voltage 8");
      PRINTDESC(0x75, "BBU Cell Voltage 9");
      PRINTDESC(0x76, "BBU Cell Voltage 10");
      PRINTDESC(0x77, "BBU Cell Voltage 11");
      PRINTDESC(0x78, "BBU Cell Voltage 12");
      PRINTDESC(0x79, "BBU Cell Voltage 13");
      PRINTDESC(0x7A, "BBU Temp 1");
      PRINTDESC(0x7B, "BBU Temp 2");
      PRINTDESC(0x7C, "BBU Temp 3");
      PRINTDESC(0x7D, "BBU Temp 4");
      PRINTDESC(0x7E, "BBU Relative State of Charge");
      PRINTDESC(0x7F, "BBU Absolute State of Charge");
      PRINTDESC(0x80, "Input VAC");
      PRINTDESC(0x81, "BBU Battery Voltage");
      PRINTDESC(0x82, "Input Current AC");
      PRINTDESC(0x83, "BBU Battery Current");
      PRINTDESC(0x84, "Battery Voltage");
      PRINTDESC(0x85, "BBU Average Current");
      PRINTDESC(0x86, "Battery Current output");
      PRINTDESC(0x87, "BBU Remaining Capacity");
      PRINTDESC(0x88, "Battery Current Input");
      PRINTDESC(0x89, "BBU Full Charge Capacity");
      PRINTDESC(0x8A, "Output Voltage (main converter)");
      PRINTDESC(0x8B, "BBU Run Time to Empty");
      PRINTDESC(0x8C, "Output Current (main converter)");
      PRINTDESC(0x8D, "BBU Average Time to Empty");
      PRINTDESC(0x8E, "IT load Voltage Output");
      PRINTDESC(0x8F, "BBU Charging Current");
      PRINTDESC(0x90, "IT load Current Output");
      PRINTDESC(0x91, "BBU Charging Voltage");
      PRINTDESC(0x92, "Bulk Cap Voltage");
      PRINTDESC(0x93, "BBU Cycle Count");
      PRINTDESC(0x94, "Input Power");
      PRINTDESC(0x95, "BBU Design Capacity");
      PRINTDESC(0x96, "Output Power");
      PRINTDESC(0x97, "BBU Design Voltage");
      PRINTDESC(0x98, "RPM fan0");
      PRINTDESC(0x99, "BBU At Rate");
      PRINTDESC(0x9A, "RPM fan1");
      PRINTDESC(0x9B, "BBU At Rate Time to Full");
      PRINTDESC(0x9C, "Set fan speed");
      PRINTDESC(0x9D, "BBU At Rate OK");
      PRINTDESC(0x9E, "Temp0");
      PRINTDESC(0x9F, "BBU Temp");
      PRINTDESC(0xA0, "Temp1");
      PRINTDESC(0xA1, "BBU Max Error");
      PRINTDESC(0xA3, "Communication baud rate");
      PRINTDESC(0xA4, "Charging constant current level override");
      PRINTDESC(0xA5, "Computed charging constant current level");
      PRINTDESC(0xD0, "General Alarm");
      PRINTDESC(0xD1, "PFC Alarm");
      PRINTDESC(0xD2, "LLC Alarm");
      PRINTDESC(0xD3, "Current Feed Alarm");
      PRINTDESC(0xD4, "Auxiliary Alarm");
      PRINTDESC(0xD5, "Battery Charger Alarm");
      PRINTDESC(0xD7, "Temperature Alarm");
      PRINTDESC(0xD8, "Fan Alarm");
      PRINTDESC(0xD9, "Communication Alarm Status Register");
      PRINTDESC(0x106,"BBU Specification Info");
      PRINTDESC(0x107,"BBU Manufacturer Date");
      PRINTDESC(0x108,"BBU Serial Number");
      PRINTDESC(0x109,"BBU Device Chemistry");
      PRINTDESC(0x10B,"BBU Manufacturer Data");
      PRINTDESC(0x10D,"BBU Manufacturer Name");
      PRINTDESC(0x115,"BBU Device Name");
      PRINTDESC(0x11D,"FB Battery Status");
      PRINTDESC(0x121,"SoH results");
      PRINTDESC(0x122,"Fan RPM Override");
      PRINTDESC(0x123,"Rack_monitor_BBU_control_enable");
      PRINTDESC(0x124,"LED Override");
      PRINTDESC(0x125,"PSU input frequency AC");
      PRINTDESC(0x126,"PSU power factor");
      PRINTDESC(0x127,"PSU iTHD");
      PRINTDESC(0x128,"PSU CC charge failure timeout");
      PRINTDESC(0x12A,"Time stamping for blackbox");
      PRINTDESC(0x12C,"Variable Charger Override timeout");
      PRINTDESC(0x12E,"Configurable BBU backup time(90s-1200s)");
      PRINTDESC(0x12F,"Configurable PLS timing(1s-300s)");
      PRINTDESC(0x130,"Forced discharge timeout");
      default:
        buf_printf(wb, "  <0x%04X>  ---        UNDEFINED        --- : ", reg_addr);
      break;
    }
    memcpy(&time, mem_pos, sizeof(time));
    for ( int j = 0; j < rd->i->keep && time != 0; j++)
    {
      mem_pos += sizeof(time);
      switch (flags & 0xF000)
      {
        case 0x8000: // ASCII
          for (int c = 0; c < len * 2; c++)
          {
            buf_printf(wb, "%c", *mem_pos);
            mem_pos++;
          }
          buf_printf(wb, "  "); // End data
        break;
        case 0x4000: //2's complement
          N = (flags >> 8) & 0x0F;
          fvalue = 0;
          for (int c = 0; c < len * 2; c++)
          {
            fvalue *= 256.0;
            fvalue += *mem_pos;
            mem_pos++;
          }
          for (int c = 0; c < N; c++)
          {
            fvalue /= 2.0;
          }
          buf_printf(wb, "%.2f  ", fvalue);
        break;
        case 0x2000: //decimal
          value = 0;
          for (int c = 0; c < len * 2; c++)
          {
            value <<= 8;
            value += *mem_pos;
            mem_pos++;
          }
          buf_printf(wb, "%d  ", value);
        break;
        case 0x1000: // table
          for (int c = 0; c < len * 2; c++)
          {
            value <<= 8;
            value += *mem_pos;
            mem_pos++;
          }
          buf_printf(wb, "%04x\n", value);
          table_description(wb ,reg_addr ,value);
        break;
        default: // No type
          value = 0;
          for (int c = 0; c < len * 2; c++)
          {
            buf_printf(wb, "%02x", *mem_pos);
            mem_pos++;
          }
          buf_printf(wb, "  ");
        break;
      }
      memcpy(&time, mem_pos, sizeof(time));
      if (time == 0)
      {
        break;
      }
    }
    if ((flags & 0xF000) != 0x1000)
    {
      buf_printf(wb, "\n"); // End Register
    }
  }
  buf_printf(wb, "\n"); // End PSU
  return 0;
}

#define GETBIT(reg, bit) ((reg >> bit) & 0x1)
#define PRINTSTAT(reg, bit, des)                                                             \
  do                                                                                         \
  {                                                                                          \
    buf_printf(wb, "       %c [%d] " des "\n", GETBIT(reg, bit) ? '+' : ' ', GETBIT(reg, bit)); \
  } while (0)

static int table_description(write_buf_t *wb,int address,int value){
  switch(address){
    case 0x68:
      PRINTSTAT(value, 8, "SoH Dicharge");
      PRINTSTAT(value, 7, "SoH Requested");
      PRINTSTAT(value, 6, "Battery Alarm Set for BBU Fail or BBU voltage =< 26 VDC");
      PRINTSTAT(value, 5, "Fan Alarm");
      PRINTSTAT(value, 4, "Temp Alarm   Alarm set at shutdown temp");
      PRINTSTAT(value, 3, "Current Feed (Boost Converter) Fail");
      PRINTSTAT(value, 2, "Battery Charger Fail");
      PRINTSTAT(value, 1, "Aux 54V Converter Fail");
      PRINTSTAT(value, 0, "Main Converter Fail");
    break;
    case 0x69:
      PRINTSTAT(value, 2, "End of Life");
      PRINTSTAT(value, 1, "Low Voltage  BBU Voltage =< 33.8");
      PRINTSTAT(value, 0, "BBU Fail");
    break;
    case 0xD0:
      PRINTSTAT(value, 7, "Com");
      PRINTSTAT(value, 6, "Fan");
      PRINTSTAT(value, 5, "Temp");
      PRINTSTAT(value, 4, "BC");
      PRINTSTAT(value, 3, "Auxiliary");
      PRINTSTAT(value, 2, "CF");
      PRINTSTAT(value, 1, "LLC");
      PRINTSTAT(value, 0, "PFC");
    break;
    case 0xD1:
      PRINTSTAT(value, 11, "LLC Enabled");
      PRINTSTAT(value, 10, "Input Relay on");
      PRINTSTAT(value, 9, "!(Bulk_OK)");
      PRINTSTAT(value, 8, "AC_OK");
      PRINTSTAT(value, 3, "LLC Enabled");
      PRINTSTAT(value, 1, "OVP (AC input over voltage protection asserted)");
      PRINTSTAT(value, 0, "UVP (AC input under voltage protection asserted)");
    break;
    case 0xD2:
      PRINTSTAT(value, 10, "Oring Fail");
      PRINTSTAT(value, 9, "2ndary DSP Failure");
      PRINTSTAT(value, 8, "DC/DC failure");
      PRINTSTAT(value, 2, "OCP (Output Over Current protection asserted)");
      PRINTSTAT(value, 1, "OVP (Output Over Voltage protection asserted)");
      PRINTSTAT(value, 0, "UVP (Output Under Voltage protection asserted)");
    break;
    case 0xD3:
      PRINTSTAT(value, 8, "CF (Failure Failure with CF)");
      PRINTSTAT(value, 4, "OPP (Over Power Protection [more than 4200W input to CF])");
      PRINTSTAT(value, 3, "Battery_UVP");
      PRINTSTAT(value, 2, "Battery_OVP");
      PRINTSTAT(value, 1, "Bulk_UVP");
      PRINTSTAT(value, 0, "Bulk_OVP");
    break;
    case 0xD4:
      PRINTSTAT(value, 8, "Aux alarm (Failure with auxiliary converter)");
      PRINTSTAT(value, 2, "OCP (Over Current Protection asserted)");
      PRINTSTAT(value, 1, "OVP (Output Over Voltage protection asserted)");
      PRINTSTAT(value, 0, "UVP (Output Under Voltage Protection asserted)");
    break;
    case 0xD5:
      PRINTSTAT(value, 8, "Charger alarm (Charger failure)");
      PRINTSTAT(value, 2, "Timeout alarm (Charger ON for more than 5hrs)");
      PRINTSTAT(value, 1, "UVP (Output Under Voltage Protection asserted)");
      PRINTSTAT(value, 0, "OVP (Output Overvoltage Protection asserted)");
    break;
    case 0xD7:
      PRINTSTAT(value, 7, "PFC temp alarm");
      PRINTSTAT(value, 6, "LLC temp alarm");
      PRINTSTAT(value, 5, "CF temp alarm");
      PRINTSTAT(value, 4, "Aux temp alarm");
      PRINTSTAT(value, 3, "Sync Rectifier temp alarm");
      PRINTSTAT(value, 2, "Oring temp alarm");
      PRINTSTAT(value, 1, "Inlet temp alarm");
      PRINTSTAT(value, 0, "Outlet temp alarm");
    break;
    case 0xD8:
      PRINTSTAT(value, 0, "Fan alarm (Fan failure)");
    break;
    case 0xD9:
      PRINTSTAT(value, 0, "Internal Communication alarm (Internal Communication fail)");
    break;
    default:
      buf_printf(wb, "Didn't implement parser yet.\n");
    break;
  }

  return 0;
}
