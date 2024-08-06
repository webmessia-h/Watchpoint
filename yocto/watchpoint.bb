# watchpoint.bb
SUMMARY = "Watchpoint Kernel Module"
DESCRIPTION = "A kernel module that sets a watchpoint on a specified memory address."
LICENSE = "GPLv2"
SRC_URI = "file://watchpoint.c"
S = "${WORKDIR}"

inherit module

EXTRA_OEMAKE = ""
EXTRA_OEMAKE_class-target = 'KDIR="${STAGING_KERNEL_DIR}"'

do_compile() {
    unset CFLAGS CPPFLAGS CXXFLAGS LDFLAGS
    oe_runmake
}

do_install() {
    install -d ${D}${base_libdir}/modules/${KERNEL_VERSION}/extra
    install -m 0644 ${S}/watchpoint.ko ${D}${base_libdir}/modules/${KERNEL_VERSION}/extra/
}

FILES_${PN} = "${base_libdir}/modules/${KERNEL_VERSION}/extra/watchpoint.ko"
