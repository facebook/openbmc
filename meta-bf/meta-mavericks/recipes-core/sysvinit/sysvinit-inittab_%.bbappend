# ttyS0 is UART5, connected through SYSCPLD, to debug board, if GPIOE0 is 1
# ttyS1 is UART1, connected through SYSCPLD, to micro-server console, if GPIOE0 is 1
# ttyS2 is UART3, connected to the front panel Console port
# ttyS3 is UART4, connected through SYSCPLD, to rackmon RS485
SERIAL_CONSOLES = "57600;ttyS0 9600;ttyS2"
