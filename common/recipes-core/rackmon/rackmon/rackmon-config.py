from rackmond import configure_rackmond


reglist = [
    {"begin": 0x0, "length": 8},  # MFR_MODEL
    {"begin": 0x10, "length": 8},  # MFR_DATE
    {"begin": 0x20, "length": 8},  # FB Part #
    {"begin": 0x30, "length": 4},  # HW Revision
    {"begin": 0x38, "length": 4},  # FW Revision
    {"begin": 0x40, "length": 16},  # MFR Serial #
    {"begin": 0x60, "length": 4},  # Workorder #
    {
        "begin": 0x68,  # PSU Status
        "length": 1,
        "keep": 10,  # 10-sample ring buffer
        "flags": 1,
    },
    {
        "begin": 0x69,  # Battery Status
        "length": 1,
        "keep": 10,  # 10-sample ring buffer
        "flags": 1,
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
    {"begin": 0x7F, "length": 1, "keep": 10},  # BBU Absolute State of Charge
    {"begin": 0x80, "length": 1, "keep": 10},  # Input VAC
    {"begin": 0x81, "length": 1, "keep": 10},  # BBU Battery Voltage
    {"begin": 0x82, "length": 1, "keep": 10},  # Input Current AC
    {"begin": 0x83, "length": 1, "keep": 10},  # BBU Battery Current
    {"begin": 0x84, "length": 1, "keep": 10},  # Battery Voltage
    {"begin": 0x85, "length": 1, "keep": 10},  # BBU Average Current
    {"begin": 0x86, "length": 1},  # Battery Current Output
    {"begin": 0x87, "length": 1, "keep": 10},  # BBU Remaining Capacity
    {"begin": 0x88, "length": 1},  # Battery Current Input
    {"begin": 0x89, "length": 1, "keep": 10},  # BBU Full Charge Capacity
    {"begin": 0x8A, "length": 1, "keep": 10},  # Output Voltage (main converter)
    {"begin": 0x8B, "length": 1, "keep": 10},  # BBU Run Time to Empty
    {"begin": 0x8C, "length": 1, "keep": 10},  # Output Current (main converter)
    {"begin": 0x8D, "length": 1, "keep": 10},  # BBU Average Time to Empty
    {"begin": 0x8E, "length": 1},  # IT Load Voltage Output
    {"begin": 0x8F, "length": 1},  # BBU Charging Current
    {"begin": 0x90, "length": 1},  # IT Load Current Output
    {"begin": 0x91, "length": 1, "keep": 10},  # BBU Charging Voltage
    {"begin": 0x92, "length": 1},  # Bulk Cap Voltage
    {"begin": 0x93, "length": 1, "keep": 10},  # BBU Cycle Count
    {"begin": 0x94, "length": 1, "keep": 10},  # Input Power
    {"begin": 0x95, "length": 1},  # BBU Design Capacity
    {"begin": 0x96, "length": 1, "keep": 10},  # Output Power
    {"begin": 0x97, "length": 1},  # BBU Design Voltage
    {"begin": 0x98, "length": 1},  # RPM Fan 0
    {"begin": 0x99, "length": 1},  # BBU At Rate
    {"begin": 0x9A, "length": 1},  # RPM Fan 1
    {"begin": 0x9B, "length": 1, "keep": 10},  # BBU At Rate Time to Full
    {"begin": 0x9C, "length": 1, "keep": 10},  # BBU At Rate Time to Empty
    {"begin": 0x9D, "length": 1, "keep": 10},  # BBU At Rate OK
    {"begin": 0x9E, "length": 1},  # Temp 0
    {"begin": 0x9F, "length": 1},  # BBU Temp
    {"begin": 0xA0, "length": 1},  # Temp 1
    {"begin": 0xA1, "length": 1},  # BBU Max Error
    {"begin": 0xD0, "length": 1},  # General Alarm Status Register
    {"begin": 0xD1, "length": 1},  # PFC Alarm Status Register
    {"begin": 0xD2, "length": 1},  # LLC Alarm Status Register
    {"begin": 0xD3, "length": 1},  # Current Feed Alarm Status Register
    {"begin": 0xD4, "length": 1},  # Auxiliary Alarm Status Register
    {"begin": 0xD5, "length": 1},  # Battery Charger Alarm Status Register
    {"begin": 0xD7, "length": 1},  # Temperature Alarm Status Register
    {"begin": 0xD8, "length": 1},  # Fan Alarm Status Register
    {"begin": 0xD9, "length": 1},  # Communication Alarm Status Register
    {"begin": 0x106, "length": 1},  # BBU Specification Info
    {"begin": 0x107, "length": 1},  # BBU Manufacturer Date
    {"begin": 0x108, "length": 1},  # BBU Serial Number
    {"begin": 0x109, "length": 2},  # BBU Device Chemistry
    {"begin": 0x10B, "length": 2},  # BBU Manufacturer Data
    {"begin": 0x10D, "length": 8},  # BBU Manufacturer Name
    {"begin": 0x115, "length": 8},  # BBU Device Name
    {"begin": 0x11D, "length": 4},  # FB Battery Status
    {"begin": 0x121, "length": 1},  # SoH results
]


def main():
    configure_rackmond(reglist, verify_configure=True)


if __name__ == "__main__":
    main()
