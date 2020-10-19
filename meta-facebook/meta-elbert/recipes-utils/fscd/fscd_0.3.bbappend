# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://setup-fan.sh \
            file://get_fan_speed.sh \
            file://set_fan_speed.sh \
            file://fsc_board.py \
            file://fsc-config.json \
            file://zone1.fsc \
           "

FSC_BIN_FILES += "get_fan_speed.sh \
                  set_fan_speed.sh "

FSC_CONFIG += "fsc-config.json \
              "

FSC_ZONE_CONFIG +="zone1.fsc"

FSC_INIT_FILE += "setup-fan.sh"
