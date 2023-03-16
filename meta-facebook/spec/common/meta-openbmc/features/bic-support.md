Provide BMC interface to the BIC to: 
- Get the BIC Vendor and Product ID info for the BIC
- Return Platform Info
- Return Kernel Info (cycles, stacks, threads, uptime and version)

This information shall be provided via Redfish and also via the bic-util
command for legacy support.
