# Verified Boot Status Utility

vboot-util can be used to verify verified/ dual-flash boot status.Running the utility will display various flags that may help in debugging verified boot issues.

**Note: When the system is successfully boots into the BMC image with a dual boot configuration the following flags are expected:**
- **hardware_enforce flag: 0x01**
- **software_enforce flag: 0x01**
- **recovery_boot flag: 0x0.**

## Tool Output Key

+-------------------+-----------------------------------------------------+
| **Field**         | **Description**                                     |
+===================+=====================================================+
| Certificates time | Time when the subordinate certificates were signed. |
+-------------------+-----------------------------------------------------+
| Certificates      | Time when the subordinate certificates of a         |
| fallback          | previously successfully flash image was signed.     |
|                   | Verified boot considers it an error for this to not |
|                   | match ‘Certificates time’.                          |
+-------------------+-----------------------------------------------------+
| U-Boot time       | Time when the U-boot of the BMC was signed.         |
+-------------------+-----------------------------------------------------+
| U-Boot fallback   | Time when the U-boot of the last successfully       |
|                   | flashed BMC image was signed.                       |
+-------------------+-----------------------------------------------------+
| Kernel time       | Reserved for future use (not set)                   |
+-------------------+-----------------------------------------------------+
| Kernel fallback   | Reserved for future use (not set)                   |
+-------------------+-----------------------------------------------------+
| recovery_boot     | BMC has booted into recovery image (CS0) due to     |
|                   | validation failure of the image in CS1.             |
+-------------------+-----------------------------------------------------+
| Status type/code  | If non-zero indicates the verified boot error type  |
|                   | and code.                                           |
+-------------------+-----------------------------------------------------+
| Status Text       | Human readable text describing the error type and   |
| (Below Status)    | code of verified boot. If successful, should be     |
|                   | “OpenBMC was verified correctly”.                   |
+-------------------+-----------------------------------------------------+
