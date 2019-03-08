# ttyS0 is UART1, connected to the front panel Console port
# ttyS1 is UART2, connected through SYSCPLD, to debug board, if UART_SEL is 0b11
# ttyS4 is UART5, connected through SYSCPLD, to micro-server console, if UART_SEL is 0b11

SERIAL_CONSOLES = "57600;ttyS1 9600;ttyS0"
