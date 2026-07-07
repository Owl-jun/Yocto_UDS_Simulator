SUMMARY = "UDS Diagnostic ECU Simulator daemon"
DESCRIPTION = "TCP based UDS diagnostic daemon inspired by AUTOSAR DCM concepts."
LICENSE = "CLOSED"

inherit cmake systemd externalsrc

EXTERNALSRC = "/home/seokjunkang/dev/Yocto_UDS_Simulator"
EXTERNALSRC_BUILD = "${WORKDIR}/build"

EXTRA_OECMAKE += "-DSYSTEMD_SYSTEM_UNIT_DIR=${systemd_system_unitdir}"

SYSTEMD_SERVICE:${PN} = "diagnostic.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

FILES:${PN} += " \
    ${systemd_system_unitdir}/diagnostic.service \
    ${sysconfdir}/diagnostic.conf \
"

RDEPENDS:${PN} += "libstdc++"
