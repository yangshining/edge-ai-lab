
ENABLE_EXTRA_MULTI_USER ?= "False"

#same as rootfs_config： CONFIG_auto-login
ALLOW_AUTOLOGIN_AS_ROOT ?= "False"


# This configuration does not take effect when ENABLE_EXTRA_MULTI_USER is False
# DISABLE_ROOT = "True"
DISABLE_ROOT = "${@bb.utils.contains('ENABLE_EXTRA_MULTI_USER', 'True', 'True', "False", d)}"
# define users
ROOT_DEFAULT_PASSWORD ?= "Ph03nix5parr0w!"
DEFAULT_ADMIN_ACCOUNT ?= "mavenir"
DEFAULT_ADMIN_GROUP ?= "wheel"
DEFAULT_ADMIN_ACCOUNT_PASSWORD ?= "Ph03nix5parr0w!"

EXTRA_USERS_PARAMS_MODIFY = "${@bb.utils.contains('DISABLE_ROOT', 'True', "usermod -L root;", "usermod -P '${ROOT_DEFAULT_PASSWORD}' root;", d)}"

EXTRA_USERS_PARAMS_MODIFY += "useradd  ${DEFAULT_ADMIN_ACCOUNT};"
EXTRA_USERS_PARAMS_MODIFY += "groupadd  ${DEFAULT_ADMIN_GROUP};"
EXTRA_USERS_PARAMS_MODIFY += "usermod -P '${DEFAULT_ADMIN_ACCOUNT_PASSWORD}' ${DEFAULT_ADMIN_ACCOUNT};"
EXTRA_USERS_PARAMS_MODIFY += "usermod -aG ${DEFAULT_ADMIN_GROUP}  ${DEFAULT_ADMIN_ACCOUNT};"

# admin's password is admin
EXTRA_USERS_PARAMS_MODIFY += "useradd -p '\$6\$LvBgd4j4gxR6Q2xx\$CleN0jdUuJ3OqLVf/X6Je5qHboMcwaEJjoVEYzvfXeMsmSAHw6HACuGbUfTI/8DOiL0a3qAZ7q48BP/4VP/vK1' admin;usermod -a -G audio admin;usermod -a -G video admin; \
"

do_rootfs[depends] += "zsys-env:do_install"
EXTRA_USERS_PARAMS:fara3 = "${EXTRA_USERS_PARAMS_MODIFY}"
# python __anonymous() {
# if d.getVar('ENABLE_EXTRA_MULTI_USER') == "True":
# d.setVar("EXTRA_USERS_PARAMS", d.getVar('EXTRA_USERS_PARAMS_MODIFY'))
# }

# add debug-tweaks
EXTRA_IMAGE_FEATURES = "${@bb.utils.contains('ENABLE_EXTRA_MULTI_USER', 'True', '', "debug-tweaks", d)}"

# Allow autologin as root
ROOTFS_POSTPROCESS_COMMAND += '${@bb.utils.contains("ALLOW_AUTOLOGIN_AS_ROOT","True", "autologin_root ; ","", d)}'

autologin_root() {
        cat << EOF > ${IMAGE_ROOTFS}/bin/autologin
#!/bin/sh
exec /bin/login -f root
EOF
        chmod 0755 ${IMAGE_ROOTFS}/bin/autologin
}

# For sudo users
EXTRA_USERS_SUDOERS = '${@bb.utils.contains("ENABLE_EXTRA_MULTI_USER","True", "${DEFAULT_ADMIN_ACCOUNT} ALL=(ALL) ALL;","${DEFAULT_ADMIN_ACCOUNT} ALL=(ALL) ALL;", d)}'

PACKAGE_INSTALL:append:fara3 = " ${@['', 'sudo'][bool(d.getVar('EXTRA_USERS_SUDOERS'))]}"

ROOTFS_POSTPROCESS_COMMAND:append:fara3 = " set_sudoers;"

set_sudoers () {
	sudoers_settings="${EXTRA_USERS_SUDOERS}"
	export PSEUDO="${FAKEROOTENV} ${STAGING_DIR_NATIVE}${bindir}/pseudo"
	setting=`echo $sudoers_settings | cut -d ';' -f1`
	remaining=`echo $sudoers_settings | cut -d ';' -f2-`
	while test "x$setting" != "x"; do
		eval "$PSEUDO echo \"$setting\" >> \"${IMAGE_ROOTFS}\"/etc/sudoers.d/99-petalinux"
		setting=`echo $remaining | cut -d ';' -f1`
		remaining=`echo $remaining | cut -d ';' -f2-`
	done
	eval "$PSEUDO chmod 0440 \"${IMAGE_ROOTFS}\"/etc/sudoers.d/99-petalinux"
}

ROOTFS_POSTPROCESS_COMMAND:fara3 += "rootfs_fixup_sshd_config; "

rootfs_fixup_sshd_config () {
	sed -i -e 's:PermitEmptyPasswords yes:PermitEmptyPasswords no:' ${IMAGE_ROOTFS}/etc/ssh/sshd_config
}
