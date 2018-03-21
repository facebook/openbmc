# ttyS0 is UART5, connected through SCMCPLD, to micro-server console, if BMC_UART_SEL5(GPIOS5) is 1
# ttyS1 is UART1, connected to the front panel Console port
# ttyS2 is UART2, connected through SCMCPLD, to debug board, if BMC_UART_SEL5(GPIOS5) is 1
# ttyS3 is UART3, connected through SYSCPLD
# ttyS4 is UART4, connected through SYSCPLD
SERIAL_CONSOLES = "57600;ttyS2 9600;ttyS1"
