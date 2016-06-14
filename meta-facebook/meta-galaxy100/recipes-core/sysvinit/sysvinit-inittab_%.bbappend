# ttyS0 is UART5, connected to the front panel Console port
# ttyS1 is UART1, connected through SYSCPLD, to micro-server console, if GPIOE0 is 1
# ttyS2 is UART3, connected through SYSCPLD, to micro-server console, if press CTRL+f+b+u+1
SERIAL_CONSOLES = "9600;ttyS0 57600;ttyS2"
