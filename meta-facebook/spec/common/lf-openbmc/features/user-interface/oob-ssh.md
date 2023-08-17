The SSH server should be OpenSSH (and not dropbear). Meta may have patches and
configuration for our internal environment in [our fork][fb-openbmc] which are
not contained in the open source tree.

The SSH interface should be used for debug effort by humans and will not
typically be used for system automation or manufacturing testing (both should
use Redfish instead). In some cases, we will explicitly document SSH commands to
be used due to lacking of Redfish support for a particular use-case.
