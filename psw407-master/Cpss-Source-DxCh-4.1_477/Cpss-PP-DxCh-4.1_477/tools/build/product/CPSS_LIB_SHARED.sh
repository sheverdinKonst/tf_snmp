#

. $tool_build/product/cpss_common.inc
export MAKE_TARGET_LINK=cpss_lib
export SHARED_MEMORY=1
export INCLUDE_SHMT=1
PROJECT_DEFS="$PROJECT_DEFS SHARED_MEMORY"

DONT_LINK=YES
export LINUX_BUILD_KERNEL=NO
