import fcntl
import sys
import os

from rackmond import configure_rackmond


reglist_V3 = [
    {"begin": 0x00, "length": 8, "flags": 0x8000},  # PSU_FB part               # ascii
    {"begin": 0x08, "length": 8, "flags": 0x8000},  # PSU_MFR_MODEL             # ascii
    {"begin": 0x10, "length": 8, "flags": 0x8000},  # PSU_MFR_DATE              # ascii
    {"begin": 0x18, "length": 16, "flags": 0x8000}, # PSU_MFR_SERIAL            # ascii
    {"begin": 0x28, "length": 4, "flags": 0x8000},  # PSU_Workorder             # ascii
    {"begin": 0x2C, "length": 4, "flags": 0x8000},  # PSU_HW Revision           # ascii
    {"begin": 0x30, "length": 4, "flags": 0x8000},  # PSU_FW Revision           # ascii
    {"begin": 0x34, "length": 2, "flags": 0x2000},  # TOTAL_UP_TIME             # unsign
    {"begin": 0x36, "length": 2, "flags": 0x2000},  # TIME_SINCE_LAST_ON        # unsign
    {"begin": 0x38, "length": 1, "flags": 0x2000},  # AC_Power_Cycle_Counter    # unsign
    {"begin": 0x39, "length": 1, "flags": 0x2000},  # AC_Outage_Counter         # unsign
    {"begin": 0x3A, "length": 2},  # reserve
    {"begin": 0x3C, "length": 1, "flags": 0x1000, "keep": 10,},  # General Alarm Status Register         # bit map
    {"begin": 0x3D, "length": 1, "flags": 0x1000, "keep": 10,},  # PFC Alarm Status Register             # bit map
    {"begin": 0x3E, "length": 1, "flags": 0x1000, "keep": 10,},  # DCDC Alarm Status Register            # bit map
    {"begin": 0x3F, "length": 1, "flags": 0x1000, "keep": 10,},  # Temperature Alarm Status Register     # bit map
    {"begin": 0x40, "length": 1, "flags": 0x1000, "keep": 10,},  # Communication Alarm Status Register   # bit map
    {"begin": 0x41, "length": 2},  # reserve
    {"begin": 0x43, "length": 1, "flags": 0x2000},  # PSU RPM fan0              # unsign
    {"begin": 0x44, "length": 1, "flags": 0x2000},  # PSU RPM fan1              # unsign
    {"begin": 0x45, "length": 1, "flags": 0x4700, "keep": 10,},  # PSU_Temp0 - Inlet         # 2'comp N=7
    {"begin": 0x46, "length": 1, "flags": 0x4700, "keep": 10,},  # PSU_Temp1 - Outlet        # 2'comp N=7
    {"begin": 0x47, "length": 1, "flags": 0x4700},  # PSU_Max_Temp              # 2'comp N=7
    {"begin": 0x48, "length": 1, "flags": 0x4700},  # PSU_Min_Temp              # 2'comp N=7
    {"begin": 0x49, "length": 2, "flags": 0x4000},  # PSU_Position_number       # 2'comp N=0
    {"begin": 0x4B, "length": 2, "flags": 0x2000},  # CRC_error_counter         # unsign 32bit
    {"begin": 0x4D, "length": 2, "flags": 0x2000},  # Timeout_error_counter     # unsign 32bit
    {"begin": 0x4F, "length": 1, "flags": 0x2000, "keep": 10,},  # PSU_Output Voltage        # unsign N=10
    {"begin": 0x50, "length": 1, "flags": 0x4600, "keep": 10,},  # PSU_Output Current        # 2'comp N=6
    {"begin": 0x51, "length": 1, "flags": 0x4600},  # I_share current value     # 2'comp N=6
    {"begin": 0x52, "length": 1, "flags": 0x4300, "keep": 10,},  # PSU_Output Power          # 2'comp N=3
    {"begin": 0x53, "length": 1, "flags": 0x4600},  # PSU_Bulk Cap Voltage      # 2'comp N=6
    {"begin": 0x54, "length": 1, "flags": 0x4000},  # PSU input frequency AC    # 2'comp N=0
    {"begin": 0x55, "length": 1, "flags": 0x4900},  # PSU iTHD                  # 2'comp N=9
    {"begin": 0x56, "length": 1, "flags": 0x4900},  # PSU power factor          # 2'comp N=9
    {"begin": 0x57, "length": 1, "flags": 0x4300},  # PSU_Input Power           # 2'comp N=3
    {"begin": 0x58, "length": 1, "flags": 0x4600, "keep": 10,},  # PSU_Input Voltage AC      # 2'comp N=6
    {"begin": 0x59, "length": 1, "flags": 0x4A00, "keep": 10,},  # PSU_Input Current AC      # 2'comp N=10
    {"begin": 0x5a, "length": 4},  # reserve
    {"begin": 0x5e, "length": 1, "flags": 0x1000},  # PSU setting register                  # bit map
    {"begin": 0x5f, "length": 1, "flags": 0x2000},  # Communication baud rate   # unsign
    {"begin": 0x60, "length": 1, "flags": 0x2000},  # Fan duty-cycle Override   # unsign
    {"begin": 0x61, "length": 1, "flags": 0x1000},  # LED Override                          # bit map
    {"begin": 0x62, "length": 2, "flags": 0x2000},  # Unix time                 # unsign
    {"begin": 0x64, "length": 1, "flags": 0x2000},  # Configurable PLS timing   # unsign
    {"begin": 0x65, "length": 1, "flags": 0x4600},  # Vin_Min                   # 2'comp N=6
    {"begin": 0x66, "length": 1, "flags": 0x4600},  # Vin_Max                   # 2'comp N=6
    {"begin": 0x67, "length": 1, "flags": 0x2000},  # Vout_setpoint_H           # unsign N=10
    {"begin": 0x68, "length": 1, "flags": 0x2000},  # Vout_setpoint_L           # unsign N=10
    {"begin": 0x69, "length": 1, "flags": 0x2000},  # Vout_change_timer         # unsign
    {"begin": 0x6a, "length": 4, "flags": 0x8000},  # PSU_FBL_FW Revision       # ascii
    {"begin": 0x6e, "length": 5},   # reserve
    {"begin": 0x73, "length": 16},  # reserve
]

reglist = [
    {"begin": 0x0, "length": 8, "flags": 0x8000},  # MFR_MODEL     # ascii
    {"begin": 0x10, "length": 8, "flags": 0x8000},  # MFR_DATE      # ascii
    {"begin": 0x20, "length": 8, "flags": 0x8000},  # FB Part #     # ascii
    {"begin": 0x30, "length": 4, "flags": 0x8000},  # HW Revision   # ascii
    {"begin": 0x38, "length": 4, "flags": 0x8000},  # FW Revision   # ascii
    {"begin": 0x40, "length": 16, "flags": 0x8000},  # MFR Serial #  # ascii
    {"begin": 0x60, "length": 4, "flags": 0x8000},  # Workorder #   # ascii
    {
        "begin": 0x68,  # PSU Status
        "length": 1,
        "keep": 10,  # 10-sample ring buffer
        "flags": 0x1000 | 1,  # table
    },
    {
        "begin": 0x69,  # Battery Status
        "length": 1,
        "keep": 10,  # 10-sample ring buffer
        "flags": 0x1000 | 1,  # table
    },
    {
        "begin": 0x6B,  # BBU Battery Mode
        "length": 1,
        "keep": 10,  # 10-sample ring buffer
        "flags": 1,
    },
    {
        "begin": 0x6C,  # BBU Battery Status
        "length": 1,
        "keep": 10,  # 10-sample ring buffer
        "flags": 1,
    },
    {"begin": 0x6D, "length": 1, "keep": 10},  # BBU Cell Voltage 1
    {"begin": 0x6E, "length": 1, "keep": 10},  # BBU Cell Voltage 2
    {"begin": 0x6F, "length": 1, "keep": 10},  # BBU Cell Voltage 3
    {"begin": 0x70, "length": 1, "keep": 10},  # BBU Cell Voltage 4
    {"begin": 0x71, "length": 1, "keep": 10},  # BBU Cell Voltage 5
    {"begin": 0x72, "length": 1, "keep": 10},  # BBU Cell Voltage 6
    {"begin": 0x73, "length": 1, "keep": 10},  # BBU Cell Voltage 7
    {"begin": 0x74, "length": 1, "keep": 10},  # BBU Cell Voltage 8
    {"begin": 0x75, "length": 1, "keep": 10},  # BBU Cell Voltage 9
    {"begin": 0x76, "length": 1, "keep": 10},  # BBU Cell Voltage 10
    {"begin": 0x77, "length": 1, "keep": 10},  # BBU Cell Voltage 11
    {"begin": 0x78, "length": 1, "keep": 10},  # BBU Cell Voltage 12
    {"begin": 0x79, "length": 1, "keep": 10},  # BBU Cell Voltage 13
    {"begin": 0x7A, "length": 1, "keep": 10},  # BBU Temp 1
    {"begin": 0x7B, "length": 1, "keep": 10},  # BBU Temp 2
    {"begin": 0x7C, "length": 1, "keep": 10},  # BBU Temp 3
    {"begin": 0x7D, "length": 1, "keep": 10},  # BBU Temp 4
    {"begin": 0x7E, "length": 1, "keep": 10},  # BBU Relative State of Charge
    {
        "begin": 0x7F,  # BBU Absolute State of Charge
        "length": 1,
        "keep": 10,  # 10-sample ring buffer
        "flags": 0x4600,  # 2'comp N=6
    },
    {
        "begin": 0x80,  # Input VAC
        "length": 1,
        "keep": 10,  # 10-sample ring buffer
        "flags": 0x4600,  # 2'comp N=6
    },
    {"begin": 0x81, "length": 1, "keep": 10},  # BBU Battery Voltage
    {
        "begin": 0x82,  # Input Current AC
        "length": 1,
        "keep": 10,  # 10-sample ring buffer
        "flags": 0x4A00,  # 2'comp N=10
    },
    {"begin": 0x83, "length": 1, "keep": 10},  # BBU Battery Current
    {"begin": 0x84, "length": 1, "keep": 10},  # Battery Voltage
    {"begin": 0x85, "length": 1, "keep": 10},  # BBU Average Current
    {
        "begin": 0x86,  # Battery Current Output
        "length": 1,  # 10-sample ring buffer
        "flags": 0x4800,  # 2'comp N=8
    },
    {"begin": 0x87, "length": 1, "keep": 10},  # BBU Remaining Capacity
    {
        "begin": 0x88,  # Battery Current Input
        "length": 1,  # 10-sample ring buffer
        "flags": 0x4C00,  # 2'comp N=12
    },
    {"begin": 0x89, "length": 1, "keep": 10},  # BBU Full Charge Capacity
    {
        "begin": 0x8A,  # Output Voltage (main converter)
        "length": 1,
        "keep": 10,  # 10-sample ring buffer
        "flags": 0x4B00,  # 2'comp N=11
    },
    {"begin": 0x8B, "length": 1, "keep": 10},  # BBU Run Time to Empty
    {
        "begin": 0x8C,  # Output Current (main converter)
        "length": 1,
        "keep": 10,  # 10-sample ring buffer
        "flags": 0x4600,  # 2'comp N=6
    },
    {"begin": 0x8D, "length": 1, "keep": 10},  # BBU Average Time to Empty
    {
        "begin": 0x8E,  # IT Load Voltage Output
        "length": 1,
        "flags": 0x4900,  # 2'comp N=9
    },
    {"begin": 0x8F, "length": 1},  # BBU Charging Current
    {
        "begin": 0x90,  # IT Load Current Output
        "length": 1,
        "flags": 0x4C00,  # 2'comp N=12
    },
    {"begin": 0x91, "length": 1, "keep": 10},  # BBU Charging Voltage
    {"begin": 0x92, "length": 1},  # Bulk Cap Voltage
    {"begin": 0x93, "length": 1, "keep": 10},  # BBU Cycle Count
    {
        "begin": 0x94,  # Input Power
        "length": 1,
        "keep": 10,  # 10-sample ring buffer
        "flags": 0x4300,  # 2'comp N=3
    },
    {"begin": 0x95, "length": 1},  # BBU Design Capacity
    {
        "begin": 0x96,  # Output Power
        "length": 1,
        "keep": 10,  # 10-sample ring buffer
        "flags": 0x4300,  # 2'comp N=3
    },
    {"begin": 0x97, "length": 1},  # BBU Design Voltage
    {"begin": 0x98, "length": 1, "flags": 0x2000},  # RPM Fan 0  # decimal
    {"begin": 0x99, "length": 1},  # BBU At Rate
    {"begin": 0x9A, "length": 1, "flags": 0x2000},  # RPM Fan 1  # decimal
    {"begin": 0x9B, "length": 1, "keep": 10},  # BBU At Rate Time to Full
    {
        "begin": 0x9C,  # BBU At Rate Time to Empty
        "length": 1,
        "keep": 10,  # 10-sample ring buffer
        "flags": 0x2000,  # decimal
    },
    {"begin": 0x9D, "length": 1, "keep": 10},  # BBU At Rate OK
    {"begin": 0x9E, "length": 1},  # Temp 0
    {"begin": 0x9F, "length": 1},  # BBU Temp
    {"begin": 0xA0, "length": 1},  # Temp 1
    {"begin": 0xA1, "length": 1},  # BBU Max Error
    {"begin": 0xA3, "length": 1},  # Communication baud rate
    {"begin": 0xA4, "length": 1},  # Charging constant current level override
    {"begin": 0xA5, "length": 1},  # Computed charging constant current level
    {
        "begin": 0xD0,  # General Alarm Status Register
        "length": 1,
        "flags": 0x1000,  # table
    },
    {
        "begin": 0xD1,  # PFC Alarm Status Register
        "length": 1,
        "flags": 0x1000,  # table
    },
    {
        "begin": 0xD2,  # LLC Alarm Status Register
        "length": 1,
        "flags": 0x1000,  # table
    },
    {
        "begin": 0xD3,  # Current Feed Alarm Status Register
        "length": 1,
        "flags": 0x1000,  # table
    },
    {
        "begin": 0xD4,  # Auxiliary Alarm Status Register
        "length": 1,
        "flags": 0x1000,  # table
    },
    {
        "begin": 0xD5,  # Battery Charger Alarm Status Register
        "length": 1,
        "flags": 0x1000,  # table
    },
    {
        "begin": 0xD7,  # Temperature Alarm Status Register
        "length": 1,
        "flags": 0x1000,  # table
    },
    {
        "begin": 0xD8,  # Fan Alarm Status Register
        "length": 1,
        "flags": 0x1000,  # table
    },
    {
        "begin": 0xD9,  # Communication Alarm Status Register
        "length": 1,
        "flags": 0x1000,  # table
    },
    {"begin": 0x106, "length": 1},  # BBU Specification Info
    {"begin": 0x107, "length": 1},  # BBU Manufacturer Date
    {"begin": 0x108, "length": 1},  # BBU Serial Number
    {"begin": 0x109, "length": 2},  # BBU Device Chemistry
    {"begin": 0x10B, "length": 2},  # BBU Manufacturer Data
    {"begin": 0x10D, "length": 8, "flags": 0x8000},  # BBU Manufacturer Name     # ascii
    {"begin": 0x115, "length": 8, "flags": 0x8000},  # BBU Device Name           # ascii
    {"begin": 0x11D, "length": 4},  # FB Battery Status
    {"begin": 0x121, "length": 1},  # SoH results
    {"begin": 0x122, "length": 1},  # Fan RPM Override
    {"begin": 0x123, "length": 1},  # Rack_monitor_BBU_control_enable
    {"begin": 0x124, "length": 1},  # LED Override
    {"begin": 0x125, "length": 1},  # PSU input frequency AC
    {"begin": 0x126, "length": 1},  # PSU power factor
    {"begin": 0x127, "length": 1},  # PSU iTHD
    {"begin": 0x128, "length": 2},  # PSU CC charge failure timeout
    {"begin": 0x12A, "length": 2},  # Time stamping for blackbox
    {"begin": 0x12C, "length": 2},  # Variable Charger Override timeout
    {"begin": 0x12E, "length": 1},  # Configurable BBU backup time(90s-1200s)
    {"begin": 0x12F, "length": 1},  # Configurable PLS timing(1s-300s)
    {"begin": 0x130, "length": 1},  # Forced discharge timeout
    {"begin": 0x131, "length": 2},  # Read SOH timestamp
]

pid_file_handle = None
pid_file_path = "/var/lock/rackmon-config.pid"


def file_is_locked(pathname):
    global pid_file_handle

    # Note: the file handle is not closed in the program, and it is by
    # design because we need to hold the file lock as long as the process
    # is running. The file will be automatically closed (and lock released)
    # when the process is terminated.
    pid_file_handle = open(pathname, "w")
    try:
        fcntl.lockf(pid_file_handle, fcntl.LOCK_EX | fcntl.LOCK_NB)
        return False
    except IOError:
        return True


def main():
    rack_version=os.getenv('RACKMOND_RACK_VERSION')
    if rack_version == "3":
        configure_rackmond(reglist_V3, verify_configure=True)
    elif rack_version == "2":
        configure_rackmond(reglist, verify_configure=True)
    else:
        print("[WARNING]:RACKMOND_RACK_VERSION unset, set to 2 by default")
        configure_rackmond(reglist, verify_configure=True)


if __name__ == "__main__":
    if file_is_locked(pid_file_path):
        print("another instance is running. Exiting!")
        sys.exit(0)

    main()
