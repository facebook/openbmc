The BMC NIC interface must support usage on all available channels for all
NICs.  In the event the BMC loses network access on a NIC, the BMC will
attempt to restore the network access by using one of the other available
channels on the NIC if possible.  If none of the channels work, the BMC
shall attempt to use an alternate NIC path to access the network if available.
