from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
if __name__ == "__main__":
    wdfile = open('/dev/watchdog', 'w')
    wdfile.write('x')  # start watchdog
    wdfile.flush()
    while True:
        wdfile.write('V')  # kick watchdog to reset timer
        wdfile.flush()
