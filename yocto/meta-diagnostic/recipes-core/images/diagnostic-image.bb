SUMMARY = "Minimal QEMU image with the UDS Diagnostic ECU Simulator"
LICENSE = "MIT"

inherit core-image

IMAGE_FEATURES += "ssh-server-dropbear debug-tweaks"

IMAGE_INSTALL += " \
    diagnostic \
"

QB_SLIRP_OPT = "-netdev user,id=net0,hostfwd=tcp:127.0.0.1:2222-:22,hostfwd=tcp:127.0.0.1:2323-:23,hostfwd=tcp:127.0.0.1:5000-:5000,tftp=${DEPLOY_DIR_IMAGE}"
