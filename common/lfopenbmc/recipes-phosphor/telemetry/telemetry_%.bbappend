FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-sensor-read-Metric.Value-objects-as-well-as-Sensor.V.patch \
"
