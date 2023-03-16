The BMC shall have a watchdog timer to ensure the BMC is still executing.  
The implemenation shall leverage the existing WDT support.

Implementation Notes:
The BMC shall configure and periodically update the internal WDT. If this
daemon fails to update WDT within a given time, the WDT should trigger a
BMC reset.
