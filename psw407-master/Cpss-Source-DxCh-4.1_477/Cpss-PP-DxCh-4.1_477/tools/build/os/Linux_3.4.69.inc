#
# required environment:
#   LINUX_DIST_PATH
#   LSP_USER_BASE
#   LINUX_KERNEL_VERSION  (or DEFAULT_LINUX_KERNEL_VERSION from CPU defs)
#   LINUX_LSP_NAME        (or DEFAULT_LINUX_LSP_NAME from CPU defs)
#   LINUX_BUILD_KERNEL    set to YES to build kernel
#
#

os_pre_build()
{
    if [ "${LINUX_BUILD_KERNEL}" != YES ]
    then
        echo " >>> os_pre_build : LINUX_BUILD_KERNEL - $LINUX_BUILD_KERNEL : no build linux kernel <<<"
        return 0
    fi

    info_message -e "BEGIN buildLinux"

    # build linux kernel
    os_linux_patch_kernel || return 1
    os_linux_kernel_is_already_built && return 0

    rm -f ${DIST}/.config
    #now make it
    info_message  "make ${MAKE_FLAGS} -C ${DIST} /arch/${CPU_TOOL}/configs/${DEFCONFIG}"
    make ${MAKE_FLAGS} -C ${DIST} ${DEFCONFIG} 
    if [ $? -ne 0 ]
    then
        error_message -e "\tmake .config failed"
        return 1
    else
        info_message -e "\tmake .config...done"
    fi

    os_linux_make_rootfs || return 1

    chmod a+x ${DIST}/scripts/$LINUX_CRFS_SCRIPT_NAME


    info_message "${DIST}/scripts/${LINUX_CRFS_SCRIPT_NAME} ${DIST}/rootfs ${TOOLKIT_PATH} ${LINUX_CRFS_ARCH_ARG} ${LINUX_DIST_PATH}"
    ${DIST}/scripts/${LINUX_CRFS_SCRIPT_NAME} ${DIST}/rootfs ${TOOLKIT_PATH} ${LINUX_CRFS_ARCH_ARG} ${LINUX_DIST_PATH}
    if [ $? -ne 0 ]
    then
        error_message -e "\tROOTFS CREATION FAILED"
        return 1
    fi

    # patch init.sh to create environment variable with LSP revision
    init=${DIST}/rootfs/etc/init.sh
    grep -q '#LSP_VERS' $init || {
        sed -i '/test -e \/usr\/bin\/appDemo/ i \
#LSP_VERS \
dmesg -s 32768 |grep -i LSP > /dev/shm/LSP_VERS.TXT \
' $init
    }

    # now build kernel
    info_message "make ${MAKE_FLAGS} -C ${DIST} uImage"
    if [ "${FAST_KERNEL_COMP}" = "TRUE" ] ; then
        KCPPFLAGS="-DSHARED_MEMORY" make ${MAKE_FLAGS} -C ${DIST} uImage -j 20
    else 
        KCPPFLAGS="-DSHARED_MEMORY" make ${MAKE_FLAGS} -C ${DIST} uImage
    fi

    if [ $? -ne 0 ]
    then
        error_message -e "\tmake uImage failed"
        return 1
    else
        info_message -e "\tmake uImage...done"
    fi

    # save md5 of lsp to make sure compilation successful
    find ${LSP_USER_BASE}/${LINUX_LSP_NAME}/. -type f -print0 | sort -z |
        xargs -0 cat | md5sum | cut -d ' ' -f 1 > $DIST/.linux_config_done

    info_message -e "END   buildLinux"
    return 0
}

os_post_build()
{
if [ "${LINUX_BUILD_KERNEL}" != YES ]
then
   echo " >>> os_post_build : LINUX_BUILD_KERNEL - $LINUX_BUILD_KERNEL : no build linux kernel <<<"     
   if [ "${CPSS_LIB_ONLY}" = "YES" ]
   then
        echo " >>> os_post_build : CPSS_LIB_ONLY - $CPSS_LIB_ONLY : copy cpss lib to $IMAGE_PATH <<<" 
        echo "cp ${COMPILATION_ROOT}/cpssDrvLib.a  ${IMAGE_PATH} "
        #cp ${IMAGE_PATH} ${COMPILATION_ROOT}/cpssDrvLib.a 
        cp ${COMPILATION_ROOT}/cpssDrvLib.a  ${IMAGE_PATH} 
   fi
   return 0
fi

if [ "${APP_DEMO_ONLY}" = "YES" ]
then
    info_message "AppDemo created"	
else
    info_message "BEGIN buildImage"
    # create image

    linux_2_6_cr_jffs2_params

    if [ -f  ${UIMAGE_PATH}/ep3041a.dtb ]; then
      trace_message -e "cp ${UIMAGE_PATH}/ep3041a.dtb ${DIST}/rootfs"
	    cp ${UIMAGE_PATH}/ep3041a.dtb ${DIST}/rootfs
    fi

    trace_message -e "cp ${UIMAGE_PATH}/uImage ${DIST}/rootfs"
    if [ "${UIMAGE_NO_COPY}" = "TRUE" ]
    then
	    info_message -e "\tuImage not included in rootfs."
    else	
	    cp ${UIMAGE_PATH}/uImage ${DIST}/rootfs
    fi	

    if [ "$DRAGONITE_TYPE" = "A1" ]
    then
	dragoniteFwPath=${CPSS_PATH}/cpssEnabler/mainSysConfig/src/appDemo/dragonite/firmware
	if [ -e ${dragoniteFwPath}/drigonite_fw.bin ]
	then
	    chmod 777 ${DIST}/rootfs/etc/drigonite_fw.bin
	    info_message "Dragonite FW copy to rootfs:"
	    info_message "cp ${dragoniteFwPath}/drigonite_fw.bin ${DIST}/rootfs/etc"	    
	    cp ${dragoniteFwPath}/drigonite_fw.bin ${DIST}/rootfs/etc
	else
	    info_message -e "\t${dragoniteFwPath}/drigonite_fw.bin not found."
	fi
    fi   

    chmod a+x ${DIST}/scripts/${CR_JFFS2_FILE_NAME}

    info_message -e "\t"${DIST}"/scripts/"${CR_JFFS2_FILE_NAME} ${CR_JFFS2_PARAMS}
    ${DIST}/scripts/${CR_JFFS2_FILE_NAME} ${CR_JFFS2_PARAMS}

    if [ $? -ne 0 ] # test result
    then
        error_message -e "\tCreate jffs2_img failed"
        return 1
    fi

    info_message "END buildImage"
fi
    return 0
}
