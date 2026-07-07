SUMMARY = "Minimal QEMU image with the UDS Diagnostic ECU Simulator"
LICENSE = "MIT"

inherit core-image

IMAGE_FEATURES += "ssh-server-dropbear debug-tweaks"

IMAGE_INSTALL += " \
    diagnostic \
"
