from openbmc_gpio_table import And, BitsEqual, BitsNotEqual, Function, Or

soc_gpio_table = {
    'A11': [
        Function('SCL5', BitsEqual(0x418, [16], 0x1)),
        Function('GPIOK0', None)
    ],
    'A12': [
        Function('NDSR1', BitsEqual(0x41c, [2], 0x1)),
        Function('GPIOM2', None)
    ],
    'A13': [
        Function('SDA10', BitsEqual(0x418, [27], 0x1)),
        Function('GPIOL3', None)
    ],
    'A14': [
        Function('SDA9', BitsEqual(0x418, [25], 0x1)),
        Function('GPIOL1', None)
    ],
    'A15': [
        Function('SIOSCI#', Or(BitsEqual(0x418, [7], 0x1), BitsEqual(0x510, [5], 0x1))),
        Function('GPIOI7', None)
    ],
    'A16': [
        Function('MTDI2', BitsEqual(0x418, [1], 0x1)),
        Function('RXD12_MT', BitsEqual(0x4b8, [1], 0x1)),
        Function('GPIOI1', None)
    ],
    'A17': [
        Function('SGPM1I', BitsEqual(0x414, [27], 0x1)),
        Function('GPIOH3', None)
    ],
    'A18': [
        Function('SGPM1CK', BitsEqual(0x414, [24], 0x1)),
        Function('GPIOH0', None)
    ],
    'A19': [
        Function('HVI3C5SDA', BitsEqual(0x418, [13], 0x1)),
        Function('SDA3', BitsEqual(0x4b8, [13], 0x1)),
        Function('GPIOJ5', None)
    ],
    'A2': [
        Function('RGMII1RXCTL', And(BitsEqual(0x400, [7], 0x1), BitsEqual(0x500, [6], 0x1))),
        Function('GPIO18A7', None)
    ],
    'A20': [
        Function('HVI3C3SDA', BitsEqual(0x418, [9], 0x1)),
        Function('SDA1', BitsEqual(0x4b8, [9], 0x1)),
        Function('GPIOJ1', None)
    ],
    'A21': [
        Function('TXD8', BitsEqual(0x414, [20], 0x1)),
        Function('SD2DAT2', And(BitsEqual(0x4b4, [20], 0x1), BitsEqual(0x450, [1], 0x1))),
        Function('SALT13_SD2', BitsEqual(0x694, [20], 0x1)),
        Function('GPIOG4', None)
    ],
    'A22': [
        Function('RXD7', BitsEqual(0x414, [19], 0x1)),
        Function('SD2DAT1', And(BitsEqual(0x4b4, [19], 0x1), BitsEqual(0x450, [1], 0x1))),
        Function('SALT12_SD2', BitsEqual(0x694, [19], 0x1)),
        Function('GPIOG3', None)
    ],
    'A23': [
        Function('SD1WP#', BitsEqual(0x414, [15], 0x1)),
        Function('PWM15_SD1', BitsEqual(0x4b4, [15], 0x1)),
        Function('GPIOF7', None)
    ],
    'A24': [
        Function('SD1CD#', BitsEqual(0x414, [14], 0x1)),
        Function('PWM14_SD1', BitsEqual(0x4b4, [14], 0x1)),
        Function('GPIOF6', None)
    ],
    'A25': [
        Function('SD1DAT3', BitsEqual(0x414, [13], 0x1)),
        Function('PWM13_SD1', BitsEqual(0x4b4, [13], 0x1)),
        Function('GPIOF5', None)
    ],
    'A3': [
        Function('RGMII1TXD1', And(BitsEqual(0x400, [3], 0x1), BitsEqual(0x500, [6], 0x1))),
        Function('RMII1TXD1', And(BitsEqual(0x400, [3], 0x1), BitsEqual(0x500, [6], 0x0))),
        Function('GPIO18A3', None)
    ],
    'AA11': [
        Function('SPI1MISO', BitsEqual(0x438, [13], 0x1)),
        Function('GPIOZ5', None)
    ],
    'AA12': [
        Function('SALT8', BitsEqual(0x438, [3], 0x1)),
        Function('WDTRST4#', BitsEqual(0x4d8, [3], 0x1)),
        Function('eMMC_Reset', BitsEqual(0x6e8, [3], 0x1)),
        Function('GPIOY3', None)
    ],
    'AA16': [
        Function('SALT14_ADC', And(BitsEqual(0x434, [5], 0x1), BitsEqual(0x694, [21], 0x0))),
        Function('GPIU5', BitsEqual(0x434, [5], 0x1)),
        Function('ADC13', None)
    ],
    'AA17': [
        Function('SALT10_ADC', And(BitsEqual(0x434, [1], 0x1), BitsEqual(0x694, [17], 0x0))),
        Function('GPIU1', BitsEqual(0x434, [1], 0x1)),
        Function('ADC9', None)
    ],
    'AA23': [
        Function('PWM10_THRU', BitsEqual(0x41c, [26], 0x1)),
        Function('THRUIN1', Or(BitsEqual(0x4bc, [26], 0x1), BitsEqual(0x510, [28], 0x1))),
        Function('GPIOP2', None)
    ],
    'AA24': [
        Function('PWM11_THRU', BitsEqual(0x41c, [27], 0x1)),
        Function('THRUOUT1', Or(BitsEqual(0x4bc, [27], 0x1), BitsEqual(0x510, [28], 0x1))),
        Function('GPIOP3', None)
    ],
    'AA25': [
        Function('TACH0', BitsEqual(0x430, [0], 0x1)),
        Function('GPIOQ0', None)
    ],
    'AA26': [
        Function('TACH7', BitsEqual(0x430, [7], 0x1)),
        Function('GPIOQ7', None)
    ],
    'AA4': [
        Function('EMMCCMD', BitsEqual(0x400, [25], 0x1)),
        Function('GPIO18D1', None)
    ],
    'AA5': [
        Function('EMMCDAT1', BitsEqual(0x400, [27], 0x1)),
        Function('GPIO18D3', None)
    ],
    'AA9': [
        Function('SPI2CS1#', BitsEqual(0x434, [25], 0x1)),
        Function('GPIOX1', None)
    ],
    'AB10': [
        Function('SPI2DQ3', BitsEqual(0x434, [31], 0x1)),
        Function('RXD12_SPI2', BitsEqual(0x4d4, [31], 0x1)),
        Function('GPIOX7', None)
    ],
    'AB11': [
        Function('SPI1CK', BitsEqual(0x438, [11], 0x1)),
        Function('GPIOZ3', None)
    ],
    'AB12': [
        Function('FWSPIWP#', Or(BitsEqual(0x438, [7], 0x1), BitsEqual(0x510, [22], 0x1))),
        Function('GPIOY7', None)
    ],
    'AB15': [
        Function('SIOS3#', Or(BitsEqual(0x434, [8], 0x1), BitsEqual(0x510, [5], 0x1))),
        Function('GPIOV0', None)
    ],
    'AB16': [
        Function('SALT9_ADC', And(BitsEqual(0x434, [0], 0x1), BitsEqual(0x694, [16], 0x0))),
        Function('GPIU0', BitsEqual(0x434, [0], 0x1)),
        Function('ADC8', None)
    ],
    'AB17': [
        Function('SALT11_ADC', And(BitsEqual(0x434, [2], 0x1), BitsEqual(0x694, [18], 0x0))),
        Function('GPIU2', BitsEqual(0x434, [2], 0x1)),
        Function('ADC10', None)
    ],
    'AB18': [
        Function('GPIT6', BitsEqual(0x430, [30], 0x1)),
        Function('ADC6', None)
    ],
    'AB19': [
        Function('GPIT5', BitsEqual(0x430, [29], 0x1)),
        Function('ADC5', None)
    ],
    'AB22': [
        Function('PWM8_THRU', BitsEqual(0x41c, [24], 0x1)),
        Function('THRUIN0', Or(BitsEqual(0x4bc, [24], 0x1), BitsEqual(0x510, [28], 0x1))),
        Function('GPIOP0', None)
    ],
    'AB23': [
        Function('PWM13_THRU', BitsEqual(0x41c, [29], 0x1)),
        Function('THRUOUT2', Or(BitsEqual(0x4bc, [29], 0x1), BitsEqual(0x510, [28], 0x1))),
        Function('GPIOP5', None)
    ],
    'AB24': [
        Function('PWM14_THRU', BitsEqual(0x41c, [30], 0x1)),
        Function('THRUIN3', BitsEqual(0x4bc, [30], 0x1)),
        Function('GPIOP6', None)
    ],
    'AB25': [
        Function('TACH1', BitsEqual(0x430, [1], 0x1)),
        Function('GPIOQ1', None)
    ],
    'AB26': [
        Function('TACH3', BitsEqual(0x430, [3], 0x1)),
        Function('GPIOQ3', None)
    ],
    'AB4': [
        Function('EMMCCLK', BitsEqual(0x400, [24], 0x1)),
        Function('GPIO18D0', None)
    ],
    'AB5': [
        Function('EMMCDAT3', BitsEqual(0x400, [29], 0x1)),
        Function('GPIO18D5', None)
    ],
    'AB6': [
        Function('EMMCCD#', BitsEqual(0x400, [30], 0x1)),
        Function('GPIO18D6', None)
    ],
    'AB7': [
        Function('ESPID0', And(BitsEqual(0x434, [16], 0x1), BitsEqual(0x510, [6], 0x0))),
        Function('LAD0', And(BitsEqual(0x434, [16], 0x1), BitsEqual(0x510, [6], 0x1))),
        Function('GPIOW0', None)
    ],
    'AB8': [
        Function('ESPID1', And(BitsEqual(0x434, [17], 0x1), BitsEqual(0x510, [6], 0x0))),
        Function('LAD1', And(BitsEqual(0x434, [17], 0x1), BitsEqual(0x510, [6], 0x1))),
        Function('GPIOW1', None)
    ],
    'AB9': [
        Function('SPI2MOSI', BitsEqual(0x434, [28], 0x1)),
        Function('GPIOX4', None)
    ],
    'AC10': [
        Function('SPI1CS1#', Or(BitsEqual(0x438, [8], 0x1), And(BitsEqual(0x510, [16], 0x1), BitsEqual(0x510, [18], 0x0)))),
        Function('GPIOZ0', None)
    ],
    'AC11': [
        Function('SPI1MOSI', BitsEqual(0x438, [12], 0x1)),
        Function('GPIOZ4', None)
    ],
    'AC12': [
        Function('FWSPIABR', Or(BitsEqual(0x438, [6], 0x1), BitsEqual(0x510, [22], 0x1))),
        Function('GPIOY6', None)
    ],
    'AC15': [
        Function('SIOONCTRL#', Or(BitsEqual(0x434, [11], 0x1), BitsEqual(0x510, [5], 0x1))),
        Function('GPIOV3', None)
    ],
    'AC16': [
        Function('SALT13_ADC', And(BitsEqual(0x434, [4], 0x1), BitsEqual(0x694, [20], 0x0))),
        Function('GPIU4', BitsEqual(0x434, [4], 0x1)),
        Function('ADC12', None)
    ],
    'AC17': [
        Function('SALT16_ADC', And(BitsEqual(0x434, [7], 0x1), BitsEqual(0x694, [23], 0x0))),
        Function('GPIU7', BitsEqual(0x434, [7], 0x1)),
        Function('ADC15', None)
    ],
    'AC18': [
        Function('GPIT1', BitsEqual(0x430, [25], 0x1)),
        Function('ADC1', None)
    ],
    'AC19': [
        Function('GPIT4', BitsEqual(0x430, [28], 0x1)),
        Function('ADC4', None)
    ],
    'AC22': [
        Function('PWM5', BitsEqual(0x41c, [21], 0x1)),
        Function('GPIOO5', None)
    ],
    'AC23': [
        Function('PWM7', BitsEqual(0x41c, [23], 0x1)),
        Function('GPIOO7', None)
    ],
    'AC24': [
        Function('PWM6', BitsEqual(0x41c, [22], 0x1)),
        Function('GPIOO6', None)
    ],
    'AC26': [
        Function('TACH5', BitsEqual(0x430, [5], 0x1)),
        Function('GPIOQ5', None)
    ],
    'AC4': [
        Function('EMMCDAT0', BitsEqual(0x400, [26], 0x1)),
        Function('GPIO18D2', None)
    ],
    'AC5': [
        Function('EMMCWP#', BitsEqual(0x400, [31], 0x1)),
        Function('GPIO18D7', None)
    ],
    'AC7': [
        Function('ESPID3', And(BitsEqual(0x434, [19], 0x1), BitsEqual(0x510, [6], 0x0))),
        Function('LAD3', And(BitsEqual(0x434, [19], 0x1), BitsEqual(0x510, [6], 0x1))),
        Function('GPIOW3', None)
    ],
    'AC8': [
        Function('ESPID2', And(BitsEqual(0x434, [18], 0x1), BitsEqual(0x510, [6], 0x0))),
        Function('LAD2', And(BitsEqual(0x434, [18], 0x1), BitsEqual(0x510, [6], 0x1))),
        Function('GPIOW2', None)
    ],
    'AC9': [
        Function('SPI2CS2#', BitsEqual(0x434, [26], 0x1)),
        Function('GPIOX2', None)
    ],
    'AD10': [
        Function('SPI1ABR', Or(BitsEqual(0x438, [9], 0x1), Or(BitsEqual(0x510, [17], 0x1), BitsEqual(0x510, [27], 0x1)))),
        Function('GPIOZ1', None)
    ],
    'AD11': [
        Function('SPI1DQ2', Or(BitsEqual(0x438, [14], 0x1), BitsEqual(0x510, [27], 0x1))),
        Function('TXD13_SPI1', And(BitsEqual(0x4d8, [14], 0x1), BitsEqual(0x4b8, [2], 0x0))),
        Function('GPIOZ6', None)
    ],
    'AD12': [
        Function('SALT6', BitsEqual(0x438, [1], 0x1)),
        Function('WDTRST2#', BitsEqual(0x4d8, [1], 0x1)),
        Function('GPIOY1', None)
    ],
    'AD14': [
        Function('SIOPWREQ#', Or(BitsEqual(0x434, [10], 0x1), BitsEqual(0x510, [5], 0x1))),
        Function('GPIOV2', None)
    ],
    'AD15': [
        Function('LPCPME', BitsEqual(0x434, [14], 0x1)),
        Function('GPIOV6', None)
    ],
    'AD16': [
        Function('SALT15_ADC', And(BitsEqual(0x434, [6], 0x1), BitsEqual(0x694, [22], 0x0))),
        Function('GPIU6', BitsEqual(0x434, [6], 0x1)),
        Function('ADC14', None)
    ],
    'AD19': [
        Function('GPIT3', BitsEqual(0x430, [27], 0x1)),
        Function('ADC3', None)
    ],
    'AD20': [
        Function('GPIT0', BitsEqual(0x430, [24], 0x1)),
        Function('ADC0', None)
    ],
    'AD22': [
        Function('PWM1', BitsEqual(0x41c, [17], 0x1)),
        Function('GPIOO1', None)
    ],
    'AD23': [
        Function('PWM2', BitsEqual(0x41c, [18], 0x1)),
        Function('GPIOO2', None)
    ],
    'AD24': [
        Function('PWM3', BitsEqual(0x41c, [19], 0x1)),
        Function('GPIOO3', None)
    ],
    'AD25': [
        Function('PWM4', BitsEqual(0x41c, [20], 0x1)),
        Function('GPIOO4', None)
    ],
    'AD26': [
        Function('PWM0', BitsEqual(0x41c, [16], 0x1)),
        Function('GPIOO0', None)
    ],
    'AD7': [
        Function('ESPIALT#', And(BitsEqual(0x434, [22], 0x1), BitsEqual(0x510, [6], 0x0))),
        Function('LSIRQ#', And(BitsEqual(0x434, [22], 0x1), BitsEqual(0x510, [6], 0x1))),
        Function('GPIOW6', None)
    ],
    'AD8': [
        Function('ESPIRST#', And(BitsEqual(0x434, [23], 0x1), BitsEqual(0x510, [6], 0x0))),
        Function('LPCRST#', And(BitsEqual(0x434, [23], 0x1), BitsEqual(0x510, [6], 0x1))),
        Function('GPIOW7', None)
    ],
    'AD9': [
        Function('SPI2MISO', BitsEqual(0x434, [29], 0x1)),
        Function('GPIOX5', None)
    ],
    'AE10': [
        Function('SPI1WP#', Or(BitsEqual(0x438, [10], 0x1), BitsEqual(0x510, [27], 0x1))),
        Function('GPIOZ2', None)
    ],
    'AE11': [
        Function('SALT7', BitsEqual(0x438, [2], 0x1)),
        Function('WDTRST3#', BitsEqual(0x4d8, [2], 0x1)),
        Function('GPIOY2', None)
    ],
    'AE12': [
        Function('FWSPIDQ2', Or(BitsEqual(0x438, [4], 0x1), BitsEqual(0x510, [22], 0x1))),
        Function('GPIOY4', None)
    ],
    'AE14': [
        Function('LPCPD#', BitsEqual(0x434, [13], 0x1)),
        Function('GPIOV5', None)
    ],
    'AE15': [
        Function('SIOPWRGD', Or(BitsEqual(0x434, [12], 0x1), BitsEqual(0x510, [5], 0x1))),
        Function('GPIOV4', None)
    ],
    'AE16': [
        Function('SALT12_ADC', And(BitsEqual(0x434, [3], 0x1), BitsEqual(0x694, [19], 0x0))),
        Function('GPIU3', BitsEqual(0x434, [3], 0x1)),
        Function('ADC11', None)
    ],
    'AE18': [
        Function('GPIT7', BitsEqual(0x430, [31], 0x1)),
        Function('ADC7', None)
    ],
    'AE19': [
        Function('GPIT2', BitsEqual(0x430, [26], 0x1)),
        Function('ADC2', None)
    ],
    'AE7': [
        Function('ESPICK', And(BitsEqual(0x434, [20], 0x1), BitsEqual(0x510, [6], 0x0))),
        Function('LCLK', And(BitsEqual(0x434, [20], 0x1), BitsEqual(0x510, [6], 0x1))),
        Function('GPIOW4', None)
    ],
    'AE8': [
        Function('SPI2CS0#', BitsEqual(0x434, [24], 0x1)),
        Function('GPIOX0', None)
    ],
    'AF10': [
        Function('SPI1DQ3', Or(BitsEqual(0x438, [15], 0x1), BitsEqual(0x510, [27], 0x1))),
        Function('RXD13_SPI1', And(BitsEqual(0x4d8, [15], 0x1), BitsEqual(0x4b8, [3], 0x0))),
        Function('GPIOZ7', None)
    ],
    'AF11': [
        Function('SALT5', BitsEqual(0x438, [0], 0x1)),
        Function('WDTRST1#', BitsEqual(0x4d8, [0], 0x1)),
        Function('GPIOY0', None)
    ],
    'AF12': [
        Function('FWSPIDQ3', Or(BitsEqual(0x438, [5], 0x1), BitsEqual(0x510, [22], 0x1))),
        Function('GPIOY5', None)
    ],
    'AF14': [
        Function('SIOS5#', Or(BitsEqual(0x434, [9], 0x1), BitsEqual(0x510, [5], 0x1))),
        Function('GPIOV1', None)
    ],
    'AF15': [
        Function('LPCSMI#', BitsEqual(0x434, [15], 0x1)),
        Function('GPIOV7', None)
    ],
    'AF7': [
        Function('ESPICS#', And(BitsEqual(0x434, [21], 0x1), BitsEqual(0x510, [6], 0x0))),
        Function('LFRAME#', And(BitsEqual(0x434, [21], 0x1), BitsEqual(0x510, [6], 0x1))),
        Function('GPIOW5', None)
    ],
    'AF8': [
        Function('SPI2CK', BitsEqual(0x434, [27], 0x1)),
        Function('GPIOX3', None)
    ],
    'AF9': [
        Function('SPI2DQ2', BitsEqual(0x434, [30], 0x1)),
        Function('TXD12_SPI2', BitsEqual(0x4d4, [30], 0x1)),
        Function('GPIOX6', None)
    ],
    'B1': [
        Function('RGMII1RXD1', And(BitsEqual(0x400, [9], 0x1), BitsEqual(0x500, [6], 0x1))),
        Function('RMII1RXD1', And(BitsEqual(0x400, [9], 0x1), BitsEqual(0x500, [6], 0x0))),
        Function('GPIO18B1', None)
    ],
    'B12': [
        Function('NDTR1', BitsEqual(0x41c, [4], 0x1)),
        Function('GPIOM4', None)
    ],
    'B13': [
        Function('NDCD1', BitsEqual(0x41c, [1], 0x1)),
        Function('GPIOM1', None)
    ],
    'B14': [
        Function('VGAHS', BitsEqual(0x418, [30], 0x1)),
        Function('GPIOL6', None)
    ],
    'B16': [
        Function('SIOPBI#', Or(BitsEqual(0x418, [6], 0x1), BitsEqual(0x510, [5], 0x1))),
        Function('GPIOI6', None)
    ],
    'B17': [
        Function('SGPS1LD', BitsEqual(0x414, [29], 0x1)),
        Function('SDA15', BitsEqual(0x4b4, [29], 0x1)),
        Function('GPIOH5', None)
    ],
    'B18': [
        Function('SGPM1LD', BitsEqual(0x414, [25], 0x1)),
        Function('GPIOH1', None)
    ],
    'B2': [
        Function('RGMII1RXD0', And(BitsEqual(0x400, [8], 0x1), BitsEqual(0x500, [6], 0x1))),
        Function('RMII1RXD0', And(BitsEqual(0x400, [8], 0x1), BitsEqual(0x500, [6], 0x0))),
        Function('GPIO18B0', None)
    ],
    'B20': [
        Function('HVI3C3SCL', BitsEqual(0x418, [8], 0x1)),
        Function('SCL1', BitsEqual(0x4b8, [8], 0x1)),
        Function('GPIOJ0', None)
    ],
    'B21': [
        Function('RXD9', BitsEqual(0x414, [23], 0x1)),
        Function('SD2WP#', And(BitsEqual(0x4b4, [23], 0x1), BitsEqual(0x450, [1], 0x1))),
        Function('SALT16_SD2', BitsEqual(0x694, [23], 0x1)),
        Function('GPIOG7', None)
    ],
    'B22': [
        Function('RXD6', BitsEqual(0x414, [17], 0x1)),
        Function('SD2CMD', And(BitsEqual(0x4b4, [17], 0x1), BitsEqual(0x450, [1], 0x1))),
        Function('SALT10_SD2', BitsEqual(0x694, [17], 0x1)),
        Function('GPIOG1', None)
    ],
    'B24': [
        Function('NRTS4', And(BitsEqual(0x414, [7], 0x1), BitsEqual(0x470, [23], 0x0))),
        Function('RGMII4RXD3', And(BitsEqual(0x4b4, [7], 0x1), And(BitsEqual(0x510, [1], 0x1), BitsEqual(0x470, [23], 0x0)))),
        Function('RGMII4RXER', And(BitsEqual(0x4b4, [7], 0x1), And(BitsEqual(0x510, [1], 0x0), BitsEqual(0x470, [23], 0x0)))),
        Function('GPIOE7', None)
    ],
    'B25': [
        Function('NDTR4', And(BitsEqual(0x414, [6], 0x1), BitsEqual(0x470, [22], 0x0))),
        Function('RGMII4RXD2', And(BitsEqual(0x4b4, [6], 0x1), And(BitsEqual(0x510, [1], 0x1), BitsEqual(0x470, [22], 0x0)))),
        Function('RGMII4CRSDV', And(BitsEqual(0x4b4, [6], 0x1), And(BitsEqual(0x510, [1], 0x0), BitsEqual(0x470, [22], 0x0)))),
        Function('GPIOE6', None)
    ],
    'B26': [
        Function('NRI4', And(BitsEqual(0x414, [5], 0x1), BitsEqual(0x470, [21], 0x0))),
        Function('RGMII4RXD1', And(BitsEqual(0x4b4, [5], 0x1), And(BitsEqual(0x510, [1], 0x1), BitsEqual(0x470, [21], 0x0)))),
        Function('RMII4RXD1', And(BitsEqual(0x4b4, [5], 0x1), And(BitsEqual(0x510, [1], 0x0), BitsEqual(0x470, [21], 0x0)))),
        Function('GPIOE5', None)
    ],
    'B3': [
        Function('RGMII1RXCK', And(BitsEqual(0x400, [6], 0x1), BitsEqual(0x500, [6], 0x1))),
        Function('RMII1RCLKI', And(BitsEqual(0x400, [6], 0x1), BitsEqual(0x500, [6], 0x0))),
        Function('GPIO18A6', None)
    ],
    'C1': [
        Function('RGMII2TXD0', And(BitsEqual(0x400, [14], 0x1), BitsEqual(0x500, [7], 0x1))),
        Function('RMII2TXD0', And(BitsEqual(0x400, [14], 0x1), BitsEqual(0x500, [7], 0x0))),
        Function('GPIO18B6', None)
    ],
    'C11': [
        Function('SDA5', BitsEqual(0x418, [17], 0x1)),
        Function('GPIOK1', None)
    ],
    'C12': [
        Function('NRTS1', BitsEqual(0x41c, [5], 0x1)),
        Function('GPIOM5', None)
    ],
    'C13': [
        Function('TXD1', BitsEqual(0x41c, [6], 0x1)),
        Function('GPIOM6', None)
    ],
    'C14': [
        Function('VGAVS', BitsEqual(0x418, [31], 0x1)),
        Function('GPIOL7', None)
    ],
    'C15': [
        Function('TXD3', BitsEqual(0x418, [28], 0x1)),
        Function('GPIOL4', None)
    ],
    'C16': [
        Function('MTDO2', BitsEqual(0x418, [4], 0x1)),
        Function('GPIOI4', None)
    ],
    'C17': [
        Function('SGPS1I0', BitsEqual(0x414, [30], 0x1)),
        Function('SCL16', BitsEqual(0x4b4, [30], 0x1)),
        Function('GPIOH6', None)
    ],
    'C18': [
        Function('SGPM1O', BitsEqual(0x414, [26], 0x1)),
        Function('GPIOH2', None)
    ],
    'C19': [
        Function('HVI3C5SCL', BitsEqual(0x418, [12], 0x1)),
        Function('SCL3', BitsEqual(0x4b8, [12], 0x1)),
        Function('GPIOJ4', None)
    ],
    'C2': [
        Function('RGMII2TXCTL', And(BitsEqual(0x400, [13], 0x1), BitsEqual(0x500, [7], 0x1))),
        Function('RMII2TXEN', And(BitsEqual(0x400, [13], 0x1), BitsEqual(0x500, [7], 0x0))),
        Function('GPIO18B5', None)
    ],
    'C20': [
        Function('HVI3C6SCL', BitsEqual(0x418, [14], 0x1)),
        Function('SCL4', BitsEqual(0x4b8, [14], 0x1)),
        Function('GPIOJ6', None)
    ],
    'C21': [
        Function('TXD7', BitsEqual(0x414, [18], 0x1)),
        Function('SD2DAT0', And(BitsEqual(0x4b4, [18], 0x1), BitsEqual(0x450, [1], 0x1))),
        Function('SALT11_SD2', BitsEqual(0x694, [18], 0x1)),
        Function('GPIOG2', None)
    ],
    'C22': [
        Function('SD1DAT2', BitsEqual(0x414, [12], 0x1)),
        Function('PWM12_SD1', BitsEqual(0x4b4, [12], 0x1)),
        Function('GPIOF4', None)
    ],
    'C23': [
        Function('SD1DAT1', BitsEqual(0x414, [11], 0x1)),
        Function('PWM11_SD1', BitsEqual(0x4b4, [11], 0x1)),
        Function('GPIOF3', None)
    ],
    'C24': [
        Function('NDSR4', And(BitsEqual(0x414, [4], 0x1), BitsEqual(0x470, [20], 0x0))),
        Function('RGMII4RXD0', And(BitsEqual(0x4b4, [4], 0x1), And(BitsEqual(0x510, [1], 0x1), BitsEqual(0x470, [20], 0x0)))),
        Function('RMII4RXD0', And(BitsEqual(0x4b4, [4], 0x1), And(BitsEqual(0x510, [1], 0x0), BitsEqual(0x470, [20], 0x0)))),
        Function('GPIOE4', None)
    ],
    'C25': [
        Function('NCTS4', And(BitsEqual(0x414, [2], 0x1), BitsEqual(0x470, [18], 0x0))),
        Function('RGMII4RXCK', And(BitsEqual(0x4b4, [2], 0x1), And(BitsEqual(0x510, [1], 0x1), BitsEqual(0x470, [18], 0x0)))),
        Function('RMII4RCLKI', And(BitsEqual(0x4b4, [2], 0x1), And(BitsEqual(0x510, [1], 0x0), BitsEqual(0x470, [18], 0x0)))),
        Function('GPIOE2', None)
    ],
    'C26': [
        Function('NDCD4', And(BitsEqual(0x414, [3], 0x1), BitsEqual(0x470, [19], 0x0))),
        Function('RGMII4RXCTL', And(BitsEqual(0x4b4, [3], 0x1), And(BitsEqual(0x510, [1], 0x1), BitsEqual(0x470, [19], 0x0)))),
        Function('GPIOE3', None)
    ],
    'C4': [
        Function('RGMII1RXD2', And(BitsEqual(0x400, [10], 0x1), BitsEqual(0x500, [6], 0x1))),
        Function('RMII1CRSDV', And(BitsEqual(0x400, [10], 0x1), BitsEqual(0x500, [6], 0x0))),
        Function('GPIO18B2', None)
    ],
    'C5': [
        Function('RGMII1TXD2', And(BitsEqual(0x400, [4], 0x1), BitsEqual(0x500, [6], 0x1))),
        Function('GPIO18A4', None)
    ],
    'C6': [
        Function('RGMII1TXCK', And(BitsEqual(0x400, [0], 0x1), BitsEqual(0x500, [6], 0x1))),
        Function('RMII1RCLKO', And(BitsEqual(0x400, [0], 0x1), BitsEqual(0x500, [6], 0x0))),
        Function('GPIO18A0', None)
    ],
    'D1': [
        Function('RGMII2RXD0', And(BitsEqual(0x400, [20], 0x1), BitsEqual(0x500, [7], 0x1))),
        Function('RMII2RXD0', And(BitsEqual(0x400, [20], 0x1), BitsEqual(0x500, [7], 0x0))),
        Function('GPIO18C4', None)
    ],
    'D11': [
        Function('SCL7', BitsEqual(0x418, [20], 0x1)),
        Function('GPIOK4', None)
    ],
    'D12': [
        Function('SCL6', BitsEqual(0x418, [18], 0x1)),
        Function('GPIOK2', None)
    ],
    'D13': [
        Function('RXD1', BitsEqual(0x41c, [7], 0x1)),
        Function('GPIOM7', None)
    ],
    'D14': [
        Function('NCTS1', BitsEqual(0x41c, [0], 0x1)),
        Function('GPIOM0', None)
    ],
    'D15': [
        Function('SCL9', BitsEqual(0x418, [24], 0x1)),
        Function('GPIOL0', None)
    ],
    'D16': [
        Function('MTMS2', BitsEqual(0x418, [3], 0x1)),
        Function('RXD13_MT', BitsEqual(0x4b8, [3], 0x1)),
        Function('GPIOI3', None)
    ],
    'D17': [
        Function('MTRSTN2', BitsEqual(0x418, [0], 0x1)),
        Function('TXD12_MT', BitsEqual(0x4b8, [0], 0x1)),
        Function('GPIOI0', None)
    ],
    'D18': [
        Function('SGPS1CK', BitsEqual(0x414, [28], 0x1)),
        Function('SCL15', BitsEqual(0x4b4, [28], 0x1)),
        Function('GPIOH4', None)
    ],
    'D19': [
        Function('HVI3C6SDA', BitsEqual(0x418, [15], 0x1)),
        Function('SDA4', BitsEqual(0x4b8, [15], 0x1)),
        Function('GPIOJ7', None)
    ],
    'D2': [
        Function('RGMII2RXCK', And(BitsEqual(0x400, [18], 0x1), BitsEqual(0x500, [7], 0x1))),
        Function('RMII2RCLKI', And(BitsEqual(0x400, [18], 0x1), BitsEqual(0x500, [7], 0x0))),
        Function('GPIO18C2', None)
    ],
    'D20': [
        Function('HVI3C4SDA', BitsEqual(0x418, [11], 0x1)),
        Function('SDA2', BitsEqual(0x4b8, [11], 0x1)),
        Function('GPIOJ3', None)
    ],
    'D21': [
        Function('TXD9', BitsEqual(0x414, [22], 0x1)),
        Function('SD2CD#', And(BitsEqual(0x4b4, [22], 0x1), BitsEqual(0x450, [1], 0x1))),
        Function('SALT15_SD2', BitsEqual(0x694, [22], 0x1)),
        Function('GPIOG6', None)
    ],
    'D22': [
        Function('SD1CLK', BitsEqual(0x414, [8], 0x1)),
        Function('PWM8_SD1', BitsEqual(0x4b4, [8], 0x1)),
        Function('GPIOF0', None)
    ],
    'D23': [
        Function('SD1DAT0', BitsEqual(0x414, [10], 0x1)),
        Function('PWM10_SD1', BitsEqual(0x4b4, [10], 0x1)),
        Function('GPIOF2', None)
    ],
    'D24': [
        Function('NRTS3', And(BitsEqual(0x414, [1], 0x1), BitsEqual(0x470, [17], 0x0))),
        Function('RGMII4TXD3', And(BitsEqual(0x4b4, [1], 0x1), And(BitsEqual(0x510, [1], 0x1), BitsEqual(0x470, [17], 0x0)))),
        Function('GPIOE1', None)
    ],
    'D26': [
        Function('NDTR3', And(BitsEqual(0x414, [0], 0x1), BitsEqual(0x470, [16], 0x0))),
        Function('RGMII4TXD2', And(BitsEqual(0x4b4, [0], 0x1), And(BitsEqual(0x510, [1], 0x1), BitsEqual(0x470, [16], 0x0)))),
        Function('GPIOE0', None)
    ],
    'D3': [
        Function('RGMII2TXD1', And(BitsEqual(0x400, [15], 0x1), BitsEqual(0x500, [7], 0x1))),
        Function('RMII2TXD1', And(BitsEqual(0x400, [15], 0x1), BitsEqual(0x500, [7], 0x0))),
        Function('GPIO18B7', None)
    ],
    'D4': [
        Function('RGMII2TXCK', And(BitsEqual(0x400, [12], 0x1), BitsEqual(0x500, [7], 0x1))),
        Function('RMII2RCLKO', And(BitsEqual(0x400, [12], 0x1), BitsEqual(0x500, [7], 0x0))),
        Function('GPIO18B4', None)
    ],
    'D5': [
        Function('RGMII1TXD0', And(BitsEqual(0x400, [2], 0x1), BitsEqual(0x500, [6], 0x1))),
        Function('RMII1TXD0', And(BitsEqual(0x400, [2], 0x1), BitsEqual(0x500, [6], 0x0))),
        Function('GPIO18A2', None)
    ],
    'D6': [
        Function('RGMII1TXCTL', And(BitsEqual(0x400, [1], 0x1), BitsEqual(0x500, [6], 0x1))),
        Function('RMII1TXEN', And(BitsEqual(0x400, [1], 0x1), BitsEqual(0x500, [6], 0x0))),
        Function('GPIO18A1', None)
    ],
    'E1': [
        Function('RGMII2RXD3', And(BitsEqual(0x400, [23], 0x1), BitsEqual(0x500, [7], 0x1))),
        Function('RGMII2RXER', And(BitsEqual(0x400, [23], 0x1), BitsEqual(0x500, [7], 0x0))),
        Function('GPIO18C7', None)
    ],
    'E11': [
        Function('SDA7', BitsEqual(0x418, [21], 0x1)),
        Function('GPIOK5', None)
    ],
    'E12': [
        Function('SDA8', BitsEqual(0x418, [23], 0x1)),
        Function('GPIOK7', None)
    ],
    'E13': [
        Function('SDA6', BitsEqual(0x418, [19], 0x1)),
        Function('GPIOK3', None)
    ],
    'E14': [
        Function('NRI1', BitsEqual(0x41c, [3], 0x1)),
        Function('GPIOM3', None)
    ],
    'E15': [
        Function('SCL10', BitsEqual(0x418, [26], 0x1)),
        Function('GPIOL2', None)
    ],
    'E16': [
        Function('SIOPBO#', Or(BitsEqual(0x418, [5], 0x1), BitsEqual(0x510, [5], 0x1))),
        Function('GPIOI5', None)
    ],
    'E17': [
        Function('MTCK2', BitsEqual(0x418, [2], 0x1)),
        Function('TXD13_MT', BitsEqual(0x4b8, [2], 0x1)),
        Function('GPIOI2', None)
    ],
    'E18': [
        Function('SGPS1I1', BitsEqual(0x414, [31], 0x1)),
        Function('SDA16', BitsEqual(0x4b4, [31], 0x1)),
        Function('GPIOH7', None)
    ],
    'E19': [
        Function('HVI3C4SCL', BitsEqual(0x418, [10], 0x1)),
        Function('SCL2', BitsEqual(0x4b8, [10], 0x1)),
        Function('GPIOJ2', None)
    ],
    'E2': [
        Function('RGMII2RXD2', And(BitsEqual(0x400, [22], 0x1), BitsEqual(0x500, [7], 0x1))),
        Function('RGMII2CRSDV', And(BitsEqual(0x400, [22], 0x1), BitsEqual(0x500, [7], 0x0))),
        Function('GPIO18C6', None)
    ],
    'E20': [
        Function('RXD8', BitsEqual(0x414, [21], 0x1)),
        Function('SD2DAT3', And(BitsEqual(0x4b4, [21], 0x1), BitsEqual(0x450, [1], 0x1))),
        Function('SALT14_SD2', BitsEqual(0x694, [21], 0x1)),
        Function('GPIOG5', None)
    ],
    'E21': [
        Function('TXD6', BitsEqual(0x414, [16], 0x1)),
        Function('SD2CLK', And(BitsEqual(0x4b4, [16], 0x1), BitsEqual(0x450, [1], 0x1))),
        Function('SALT9_SD2', BitsEqual(0x694, [16], 0x1)),
        Function('GPIOG0', None)
    ],
    'E22': [
        Function('SD1CMD', BitsEqual(0x414, [9], 0x1)),
        Function('PWM9_SD1', BitsEqual(0x4b4, [9], 0x1)),
        Function('GPIOF1', None)
    ],
    'E23': [
        Function('NDCD3', BitsEqual(0x410, [29], 0x1)),
        Function('RGMII4TXCTL', And(BitsEqual(0x4b0, [29], 0x1), BitsEqual(0x510, [1], 0x1))),
        Function('RMII4TXEN', And(BitsEqual(0x4b0, [29], 0x1), BitsEqual(0x510, [1], 0x0))),
        Function('GPIOD5', None)
    ],
    'E24': [
        Function('NDSR3', BitsEqual(0x410, [30], 0x1)),
        Function('RGMII4TXD0', And(BitsEqual(0x4b0, [30], 0x1), BitsEqual(0x510, [1], 0x1))),
        Function('RMII4TXD0', And(BitsEqual(0x4b0, [30], 0x1), BitsEqual(0x510, [1], 0x0))),
        Function('GPIOD6', None)
    ],
    'E25': [
        Function('NRI3', BitsEqual(0x410, [31], 0x1)),
        Function('RGMII4TXD1', And(BitsEqual(0x4b0, [31], 0x1), BitsEqual(0x510, [1], 0x1))),
        Function('RMII4TXD1', And(BitsEqual(0x4b0, [31], 0x1), BitsEqual(0x510, [1], 0x0))),
        Function('GPIOD7', None)
    ],
    'E26': [
        Function('RGMII3RXD3', And(BitsEqual(0x410, [27], 0x1), BitsEqual(0x510, [0], 0x1))),
        Function('RMII3RXER', And(BitsEqual(0x410, [27], 0x1), BitsEqual(0x510, [0], 0x0))),
        Function('GPIOD3', None)
    ],
    'E3': [
        Function('RGMII2RXCTL', And(BitsEqual(0x400, [19], 0x1), BitsEqual(0x500, [7], 0x1))),
        Function('GPIO18C3', None)
    ],
    'E4': [
        Function('RGMII2TXD2', And(BitsEqual(0x400, [16], 0x1), BitsEqual(0x500, [7], 0x1))),
        Function('GPIO18C0', None)
    ],
    'E5': [
        Function('RGMII1RXD3', And(BitsEqual(0x400, [11], 0x1), BitsEqual(0x500, [6], 0x1))),
        Function('RMII1RXER', And(BitsEqual(0x400, [11], 0x1), BitsEqual(0x500, [6], 0x0))),
        Function('GPIO18B3', None)
    ],
    'E6': [
        Function('RGMII1TXD3', And(BitsEqual(0x400, [5], 0x1), BitsEqual(0x500, [6], 0x1))),
        Function('GPIO18A5', None)
    ],
    'F13': [
        Function('SCL8', BitsEqual(0x418, [22], 0x1)),
        Function('GPIOK6', None)
    ],
    'F15': [
        Function('RXD3', BitsEqual(0x418, [29], 0x1)),
        Function('GPIOL5', None)
    ],
    'F22': [
        Function('RGMII3TXD3', And(BitsEqual(0x410, [21], 0x1), BitsEqual(0x510, [0], 0x1))),
        Function('GPIOC5', None)
    ],
    'F23': [
        Function('RGMII3RXD0', And(BitsEqual(0x410, [24], 0x1), BitsEqual(0x510, [0], 0x1))),
        Function('RMII3RXD0', And(BitsEqual(0x410, [24], 0x1), BitsEqual(0x510, [0], 0x0))),
        Function('GPIOD0', None)
    ],
    'F24': [
        Function('NCTS3', BitsEqual(0x410, [28], 0x1)),
        Function('RGMII4TXCK', And(BitsEqual(0x4b0, [28], 0x1), BitsEqual(0x510, [1], 0x1))),
        Function('RMII4RCLKO', And(BitsEqual(0x4b0, [28], 0x1), BitsEqual(0x510, [1], 0x0))),
        Function('GPIOD4', None)
    ],
    'F25': [
        Function('RGMII3RXD2', And(BitsEqual(0x410, [26], 0x1), BitsEqual(0x510, [0], 0x1))),
        Function('RMII3CRSDV', And(BitsEqual(0x410, [26], 0x1), BitsEqual(0x510, [0], 0x0))),
        Function('GPIOD2', None)
    ],
    'F26': [
        Function('RGMII3RXD1', And(BitsEqual(0x410, [25], 0x1), BitsEqual(0x510, [0], 0x1))),
        Function('RMII3RXD1', And(BitsEqual(0x410, [25], 0x1), BitsEqual(0x510, [0], 0x0))),
        Function('GPIOD1', None)
    ],
    'F4': [
        Function('RGMII2RXD1', And(BitsEqual(0x400, [21], 0x1), BitsEqual(0x500, [7], 0x1))),
        Function('RMII2RXD1', And(BitsEqual(0x400, [21], 0x1), BitsEqual(0x500, [7], 0x0))),
        Function('GPIO18C5', None)
    ],
    'F5': [
        Function('RGMII2TXD3', And(BitsEqual(0x400, [17], 0x1), BitsEqual(0x500, [7], 0x1))),
        Function('GPIO18C1', None)
    ],
    'G22': [
        Function('RGMII3TXD2', And(BitsEqual(0x410, [20], 0x1), BitsEqual(0x510, [0], 0x1))),
        Function('GPIOC4', None)
    ],
    'G23': [
        Function('RGMII3RXCK', And(BitsEqual(0x410, [22], 0x1), BitsEqual(0x510, [0], 0x1))),
        Function('RMII3RCLKI', And(BitsEqual(0x410, [22], 0x1), BitsEqual(0x510, [0], 0x0))),
        Function('GPIOC6', None)
    ],
    'G24': [
        Function('RGMII3RXCTL', And(BitsEqual(0x410, [23], 0x1), BitsEqual(0x510, [0], 0x1))),
        Function('GPIOC7', None)
    ],
    'G26': [
        Function('MDIO2', BitsEqual(0x410, [13], 0x1)),
        Function('GPIOB5', None)
    ],
    'H22': [
        Function('RGMII3TXD0', And(BitsEqual(0x410, [18], 0x1), BitsEqual(0x510, [0], 0x1))),
        Function('RMII3TXD0', And(BitsEqual(0x410, [18], 0x1), BitsEqual(0x510, [0], 0x0))),
        Function('GPIOC2', None)
    ],
    'H23': [
        Function('RGMII3TXD1', And(BitsEqual(0x410, [19], 0x1), BitsEqual(0x510, [0], 0x1))),
        Function('RMII3TXD1', And(BitsEqual(0x410, [19], 0x1), BitsEqual(0x510, [0], 0x0))),
        Function('GPIOC3', None)
    ],
    'H24': [
        Function('RGMII3TXCK', And(BitsEqual(0x410, [16], 0x1), BitsEqual(0x510, [0], 0x1))),
        Function('RMII3RCLKO', And(BitsEqual(0x410, [16], 0x1), BitsEqual(0x510, [0], 0x0))),
        Function('GPIOC0', None)
    ],
    'H25': [
        Function('TXD4', BitsEqual(0x410, [14], 0x1)),
        Function('GPIOB6', None)
    ],
    'H26': [
        Function('SALT3', BitsEqual(0x410, [10], 0x1)),
        Function('GPIOB2', None)
    ],
    'J22': [
        Function('RGMII3TXCTL', And(BitsEqual(0x410, [17], 0x1), BitsEqual(0x510, [0], 0x1))),
        Function('RMII3TXEN', And(BitsEqual(0x410, [17], 0x1), BitsEqual(0x510, [0], 0x0))),
        Function('GPIOC1', None)
    ],
    'J23': [
        Function('MDC2', BitsEqual(0x410, [12], 0x1)),
        Function('GPIOB4', None)
    ],
    'J24': [
        Function('RXD4', BitsEqual(0x410, [15], 0x1)),
        Function('GPIOB7', None)
    ],
    'J25': [
        Function('SALT4', BitsEqual(0x410, [11], 0x1)),
        Function('GPIOB3', None)
    ],
    'J26': [
        Function('SALT1', BitsEqual(0x410, [8], 0x1)),
        Function('GPIOB0', None)
    ],
    'K23': [
        Function('SALT2', BitsEqual(0x410, [9], 0x1)),
        Function('GPIOB1', None)
    ],
    'K24': [
        Function('MDIO4', BitsEqual(0x410, [3], 0x1)),
        Function('SDA12', BitsEqual(0x4b0, [3], 0x1)),
        Function('GPIOA3', None)
    ],
    'K25': [
        Function('MACLINK4', BitsEqual(0x410, [7], 0x1)),
        Function('SDA14', BitsEqual(0x4b0, [7], 0x1)),
        Function('SGPS2I1', BitsEqual(0x690, [7], 0x1)),
        Function('GPIOA7', None)
    ],
    'K26': [
        Function('MACLINK1', BitsEqual(0x410, [4], 0x1)),
        Function('SCL13', BitsEqual(0x4b0, [4], 0x1)),
        Function('SGPS2CK', BitsEqual(0x690, [4], 0x1)),
        Function('GPIOA4', None)
    ],
    'L23': [
        Function('MACLINK3', BitsEqual(0x410, [6], 0x1)),
        Function('SCL14', BitsEqual(0x4b0, [6], 0x1)),
        Function('SGPS2I0', BitsEqual(0x690, [6], 0x1)),
        Function('GPIOA6', None)
    ],
    'L24': [
        Function('MACLINK2', BitsEqual(0x410, [5], 0x1)),
        Function('SDA13', BitsEqual(0x4b0, [5], 0x1)),
        Function('SGPS2LD', BitsEqual(0x690, [5], 0x1)),
        Function('GPIOA5', None)
    ],
    'L26': [
        Function('MDC4', BitsEqual(0x410, [2], 0x1)),
        Function('SCL12', BitsEqual(0x4b0, [2], 0x1)),
        Function('GPIOA2', None)
    ],
    'M23': [
        Function('NRTS2', BitsEqual(0x41c, [13], 0x1)),
        Function('GPION5', None)
    ],
    'M24': [
        Function('MDC3', BitsEqual(0x410, [0], 0x1)),
        Function('SCL11', BitsEqual(0x4b0, [0], 0x1)),
        Function('GPIOA0', None)
    ],
    'M25': [
        Function('MDIO3', BitsEqual(0x410, [1], 0x1)),
        Function('SDA11', BitsEqual(0x4b0, [1], 0x1)),
        Function('GPIOA1', None)
    ],
    'M26': [
        Function('RXD2', BitsEqual(0x41c, [15], 0x1)),
        Function('GPION7', None)
    ],
    'N23': [
        Function('NDCD2', BitsEqual(0x41c, [9], 0x1)),
        Function('GPION1', None)
    ],
    'N24': [
        Function('NRI2', BitsEqual(0x41c, [11], 0x1)),
        Function('GPION3', None)
    ],
    'N25': [
        Function('NDSR2', BitsEqual(0x41c, [10], 0x1)),
        Function('GPION2', None)
    ],
    'N26': [
        Function('TXD2', BitsEqual(0x41c, [14], 0x1)),
        Function('GPION6', None)
    ],
    'P23': [
        Function('TXD11', BitsEqual(0x430, [22], 0x1)),
        Function('GPIOS6', None)
    ],
    'P24': [
        Function('RXD10', BitsEqual(0x430, [21], 0x1)),
        Function('GPIOS5', None)
    ],
    'P25': [
        Function('NCTS2', BitsEqual(0x41c, [8], 0x1)),
        Function('GPION0', None)
    ],
    'P26': [
        Function('NDTR2', BitsEqual(0x41c, [12], 0x1)),
        Function('GPION4', None)
    ],
    'R23': [
        Function('MDC1', BitsEqual(0x430, [16], 0x1)),
        Function('GPIOS0', None)
    ],
    'R24': [
        Function('OSCCLK', BitsEqual(0x430, [19], 0x1)),
        Function('GPIOS3', None)
    ],
    'R26': [
        Function('TXD10', BitsEqual(0x430, [20], 0x1)),
        Function('GPIOS4', None)
    ],
    'T23': [
        Function('TACH13', BitsEqual(0x430, [13], 0x1)),
        Function('GPIOR5', None)
    ],
    'T24': [
        Function('RXD11', BitsEqual(0x430, [23], 0x1)),
        Function('GPIOS7', None)
    ],
    'T25': [
        Function('MDIO1', BitsEqual(0x430, [17], 0x1)),
        Function('GPIOS1', None)
    ],
    'T26': [
        Function('PEWAKE#', BitsEqual(0x430, [18], 0x1)),
        Function('GPIOS2', None)
    ],
    'U24': [
        Function('TACH9', BitsEqual(0x430, [9], 0x1)),
        Function('GPIOR1', None)
    ],
    'U25': [
        Function('TACH12', BitsEqual(0x430, [12], 0x1)),
        Function('GPIOR4', None)
    ],
    'U26': [
        Function('TACH15', BitsEqual(0x430, [15], 0x1)),
        Function('GPIOR7', None)
    ],
    'V24': [
        Function('TACH10', BitsEqual(0x430, [10], 0x1)),
        Function('GPIOR2', None)
    ],
    'V25': [
        Function('TACH8', BitsEqual(0x430, [8], 0x1)),
        Function('GPIOR0', None)
    ],
    'V26': [
        Function('TACH11', BitsEqual(0x430, [11], 0x1)),
        Function('GPIOR3', None)
    ],
    'W23': [
        Function('PWM12_THRU', BitsEqual(0x41c, [28], 0x1)),
        Function('THRUIN2', Or(BitsEqual(0x4bc, [28], 0x1), BitsEqual(0x510, [28], 0x1))),
        Function('GPIOP4', None)
    ],
    'W24': [
        Function('PWM9_THRU', BitsEqual(0x41c, [25], 0x1)),
        Function('THRUOUT0', Or(BitsEqual(0x4bc, [25], 0x1), BitsEqual(0x510, [28], 0x1))),
        Function('GPIOP1', None)
    ],
    'W26': [
        Function('TACH14', BitsEqual(0x430, [14], 0x1)),
        Function('GPIOR6', None)
    ],
    'Y1': [
        Function('FWSPI18CS#', BitsEqual(0x500, [3], 0x1)),
        Function('VBCS#', BitsEqual(0x500, [5], 0x1)),
        Function('EMMCDAT4', BitsEqual(0x404, [0], 0x1)),
        Function('GPIO18E0', None)
    ],
    'Y2': [
        Function('FWSPI18CK', BitsEqual(0x500, [3], 0x1)),
        Function('VBCK', BitsEqual(0x500, [5], 0x1)),
        Function('EMMCDAT5', BitsEqual(0x404, [1], 0x1)),
        Function('GPIO18E1', None)
    ],
    'Y23': [
        Function('PWM15_THRU', BitsEqual(0x41c, [31], 0x1)),
        Function('THRUOUT3', BitsEqual(0x4bc, [31], 0x1)),
        Function('HEARTBEAT', BitsEqual(0x69c, [31], 0x1)),
        Function('GPIOP7', None)
    ],
    'Y24': [
        Function('TACH2', BitsEqual(0x430, [2], 0x1)),
        Function('GPIOQ2', None)
    ],
    'Y25': [
        Function('TACH6', BitsEqual(0x430, [6], 0x1)),
        Function('GPIOQ6', None)
    ],
    'Y26': [
        Function('TACH4', BitsEqual(0x430, [4], 0x1)),
        Function('GPIOQ4', None)
    ],
    'Y3': [
        Function('FWSPI18MOSI', BitsEqual(0x500, [3], 0x1)),
        Function('VBMOSI', BitsEqual(0x500, [5], 0x1)),
        Function('EMMCDAT6', BitsEqual(0x404, [2], 0x1)),
        Function('GPIO18E2', None)
    ],
    'Y4': [
        Function('FWSPI18MISO', BitsEqual(0x500, [3], 0x1)),
        Function('VBMISO', BitsEqual(0x500, [5], 0x1)),
        Function('EMMCDAT7', BitsEqual(0x404, [3], 0x1)),
        Function('GPIO18E3', None)
    ],
    'Y5': [
        Function('EMMCDAT2', BitsEqual(0x400, [28], 0x1)),
        Function('GPIO18D4', None)
    ],
}
