# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

LOCAL_URI += " \
    file://mTerm_server.service \
    "

MTERM_SYSTEMD_SERVICES = "mTerm_server.service"
