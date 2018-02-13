from rackmond import configure_rackmond

reglist = [
    {"begin": 0x0, #MFR_MODEL
     "length": 8},
    {"begin": 0x10, #MFR_DATE
     "length": 8},
    {"begin": 0x20, #FB Part #
     "length": 8},
    {"begin": 0x30, #HW Revision
     "length": 4},
    {"begin": 0x38, #FW Revision
     "length": 4},
    {"begin": 0x40, #MFR Serial #
     "length": 16},
    {"begin": 0x60, #Workorder #
     "length": 4},
    {"begin": 0x68, #PSU Status
     "length": 1,
     "keep": 10,   # 10-sample ring buffer
     "flags": 1},
    {"begin": 0x69, #Battery Status
     "length": 1,
     "keep": 10,   # 10-sample ring buffer
     "flags": 1},
    {"begin": 0x6B, #BBU Battery Mode
     "length": 1,
     "keep": 10,   # 10-sample ring buffer
     "flags": 1},
    {"begin": 0x6C, #BBU Battery Status
     "length": 1,
     "keep": 10,   # 10-sample ring buffer
     "flags": 1},
    {"begin": 0x6D, #BBU Cell Voltage 1
     "length": 1,
     "keep": 10},
    {"begin": 0x6E, #BBU Cell Voltage 2
     "length": 1,
     "keep": 10},
    {"begin": 0x6F, #BBU Cell Voltage 3
     "length": 1,
     "keep": 10},
    {"begin": 0x70, #BBU Cell Voltage 4
     "length": 1,
     "keep": 10},
    {"begin": 0x71, #BBU Cell Voltage 5
     "length": 1,
     "keep": 10},
    {"begin": 0x72, #BBU Cell Voltage 6
     "length": 1,
     "keep": 10},
    {"begin": 0x73, #BBU Cell Voltage 7
     "length": 1,
     "keep": 10},
    {"begin": 0x74, #BBU Cell Voltage 8
     "length": 1,
     "keep": 10},
    {"begin": 0x75, #BBU Cell Voltage 9
     "length": 1,
     "keep": 10},
    {"begin": 0x76, #BBU Cell Voltage 10
     "length": 1,
     "keep": 10},
    {"begin": 0x77, #BBU Cell Voltage 11
     "length": 1,
     "keep": 10},
    {"begin": 0x78, #BBU Cell Voltage 12
     "length": 1,
     "keep": 10},
    {"begin": 0x79, #BBU Cell Voltage 13
     "length": 1,
     "keep": 10},
    {"begin": 0x7A, #BBU Temp 1
     "length": 1,
     "keep": 10},
    {"begin": 0x7B, #BBU Temp 2
     "length": 1,
     "keep": 10},
    {"begin": 0x7C, #BBU Temp 3
     "length": 1,
     "keep": 10},
    {"begin": 0x7D, #BBU Temp 4
     "length": 1,
     "keep": 10},
    {"begin": 0x7E, #BBU Relative State of Charge
     "length": 1,
     "keep": 10},
    {"begin": 0x7F, #BBU Absolute State of Charge
     "length": 1,
     "keep": 10},
    {"begin": 0x80, #Input VAC
     "length": 1,
     "keep": 10},
    {"begin": 0x81, #BBU Battery Voltage
     "length": 1,
     "keep": 10},
    {"begin": 0x82, #Input Current AC
     "length": 1,
     "keep": 10},
    {"begin": 0x83, #BBU Battery Current
     "length": 1,
     "keep": 10},
    {"begin": 0x84, #Battery Voltage
     "length": 1,
     "keep": 10},
    {"begin": 0x85, #BBU Average Current
     "length": 1,
     "keep": 10},
    {"begin": 0x86, #Battery Current Output
     "length": 1},
    {"begin": 0x87, #BBU Remaining Capacity
     "length": 1,
     "keep": 10},
    {"begin": 0x88, #Battery Current Input
     "length": 1},
    {"begin": 0x89, #BBU Full Charge Capacity
     "length": 1,
     "keep": 10},
    {"begin": 0x8A, #Output Voltage (main converter)
     "length": 1,
     "keep": 10},
    {"begin": 0x8B, #BBU Run Time to Empty
     "length": 1,
     "keep": 10},
    {"begin": 0x8C, #Output Current (main converter)
     "length": 1,
     "keep": 10},
    {"begin": 0x8D, #BBU Average Time to Empty
     "length": 1,
     "keep": 10},
    {"begin": 0x8E, #IT Load Voltage Output
     "length": 1},
    {"begin": 0x8F, #BBU Charging Current
     "length": 1},
    {"begin": 0x90, #IT Load Current Output
     "length": 1},
    {"begin": 0x91, #BBU Charging Voltage
     "length": 1,
     "keep": 10},
    {"begin": 0x92, #Bulk Cap Voltage
     "length": 1},
    {"begin": 0x93, #BBU Cycle Count
     "length": 1,
     "keep": 10},
    {"begin": 0x94, #Input Power
     "length": 1,
     "keep": 10},
    {"begin": 0x95, #BBU Design Capacity
     "length": 1},
    {"begin": 0x96, #Output Power
     "length": 1,
     "keep": 10},
    {"begin": 0x97, #BBU Design Voltage
     "length": 1},
    {"begin": 0x98, #RPM Fan 0
     "length": 1},
    {"begin": 0x99, #BBU At Rate
     "length": 1},
    {"begin": 0x9A, #RPM Fan 1
     "length": 1},
    {"begin": 0x9B, #BBU At Rate Time to Full
     "length": 1,
     "keep": 10},
    {"begin": 0x9C, #BBU At Rate Time to Empty
     "length": 1,
     "keep": 10},
    {"begin": 0x9D, #BBU At Rate OK
     "length": 1,
     "keep": 10},
    {"begin": 0x9E, #Temp 0
     "length": 1},
    {"begin": 0x9F, #BBU Temp
     "length": 1},
    {"begin": 0xA0, #Temp 1
     "length": 1},
    {"begin": 0xA1, #BBU Max Error
     "length": 1},
    {"begin": 0xD0, #General Alarm Status Register
     "length": 1},
    {"begin": 0xD1, #PFC Alarm Status Register
     "length": 1},
    {"begin": 0xD2, #LLC Alarm Status Register
     "length": 1},
    {"begin": 0xD3, #Current Feed Alarm Status Register
     "length": 1},
    {"begin": 0xD4, #Auxiliary Alarm Status Register
     "length": 1},
    {"begin": 0xD5, #Battery Charger Alarm Status Register
     "length": 1},
    {"begin": 0xD7, #Temperature Alarm Status Register
     "length": 1},
    {"begin": 0xD8, #Fan Alarm Status Register
     "length": 1},
    {"begin": 0xD9, #Communication Alarm Status Register
     "length": 1},
    {"begin": 0x106, #BBU Specification Info
     "length": 1},
    {"begin": 0x107, #BBU Manufacturer Date
     "length": 1},
    {"begin": 0x108, #BBU Serial Number
     "length": 1},
    {"begin": 0x109, #BBU Device Chemistry
     "length": 2},
    {"begin": 0x10B, #BBU Manufacturer Data
     "length": 2},
    {"begin": 0x10D, #BBU Manufacturer Name
     "length": 8},
    {"begin": 0x115, #BBU Device Name
     "length": 8},
    {"begin": 0x11D, #FB Battery Status
     "length": 4},
    {"begin": 0x121, #SoH results
     "length": 1},
]

def main():
    configure_rackmond(reglist, verify_configure=True)

if __name__ == "__main__":
    main()
