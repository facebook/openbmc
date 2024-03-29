# Version 1
```txt
+===================================+ 0x1001_6400
|             NOT USED              |
+===================================+ 0x1001_6000
|              rsv                  | reboot flag @0x1001_5C00, 0x1001_5C04, 0x1001_5C08
............Reboot Flag ............. 0x1001_5C00
|               vbs (max 1KB)       |
+-----------------------------------+ 0x1001_5800
|      SPL Heap (malloc_f) (12KB)   |
.                                   .
+-----------------------------------+ 0x1001_2800
|                GD                 |
+-----------------------------------+ 0x1001_2710
|               stack               |
.                                   .
.                                   .
|                                   |
+===================================+ 0x1000_0000

```

# Version 2

Reserved 2KB SRAM after vbs rsv, from 0x1001_5000, for TPM event log

```txt
+===================================+ 0x1001_6400
|             NOT USED              |
+===================================+ 0x1001_6000
|              rsv                  | reboot flag @0x1001_5C00, 0x1001_5C04, 0x1001_5C08
............Reboot Flag ............. 0x1001_5C00
|               vbs (max 1KB)       |
+-----------------------------------+ 0x1001_5800
|          TPM event log (2KB)      |
+-----------------------------------+ 0x1001_5000
|      SPL Heap (malloc_f) (12KB)   |
.                                   .
+-----------------------------------+ 0x1001_2000
|                GD                 |
+-----------------------------------+ 0x1001_1F10
|               stack               |
.                                   .
.                                   .
|                                   |
+===================================+ 0x1000_0000
```

# Version 3
Reserved 4KB after TPM event log, from 0x1001_4000 ~ 0x1001_5000 for vboot3.0 operation certificate.

```txt
+===================================+ 0x1001_6400
|             NOT USED              |
+===================================+ 0x1001_6000
|              rsv                  | reboot flag @0x1001_5C00, 0x1001_5C04, 0x1001_5C08
............Reboot Flag ............. 0x1001_5C00
|               vbs (max 1KB)       |
+-----------------------------------+ 0x1001_5800
|          TPM event log (2KB)      |
+-----------------------------------+ 0x1001_5000
|    vboot3.0 operation Certificate |
|            (4KB)                  |
+-----------------------------------+ 0x1001_4000
|      SPL Heap (malloc_f) (12KB)   |
.                                   .
+-----------------------------------+ 0x1001_1000
|                GD                 |
+-----------------------------------+ 0x1001_0F10
|               stack               |
.                                   .
.                                   .
|                                   |
+===================================+ 0x1000_0000
```

