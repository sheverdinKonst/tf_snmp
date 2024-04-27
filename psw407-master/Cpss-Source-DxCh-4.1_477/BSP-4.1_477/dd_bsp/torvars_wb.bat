set WIND_PREFERRED_PACKAGES=vxworks-6.5
set WIND_HOME=C:/Workbench_2.6
set WIND_DIR=C:\Workbench_2.6
set WIND_BASE=%WIND_HOME%/vxworks-6.5
set WIND_USR=%WIND_HOME%/vxworks-6.5/target/usr
set WIND_DIAB_PATH=%WIND_HOME%/diab/5.5.1.0
set WRSD_LICENSE_FILE=%WIND_HOME%/license
set WIND_HOST_TYPE=x86-win32
set WIND_GNU_PATH=%WIND_HOME%/gnu/3.4.4-vxworks-6.5
set WIND_DOCS=%WIND_HOME%/docs
set WIND_TOOLS=%WIND_HOME%/workbench-2.6
set LD_LIBRARY_PATH=%WIND_HOME%/vxworks-6.5/host/x86-win32/lib;%WIND_HOME%/workbench-2.6/wrwb/platform/eclipse/x86-win32/bin;%WIND_HOME%/workbench-2.6/x86-win32/lib;%WIND_HOME%/workbench-2.6/foundation/4.0.11/x86-win32/lib
set WIND_PLATFORM=vxworks-6.5
set COMP_IPNET2=ip_net2-6.5
set WIND_COMPONENTS_LIBPATHS=%WIND_HOME%/components/obj/vxworks-6.5/krnl/lib
set WIND_COMPONENTS_INCLUDES=%WIND_HOME%/components/ip_net2-6.5/ipcom/include;%WIND_HOME%/components/ip_net2-6.5/ipcom/config;%WIND_HOME%/components/ip_net2-6.5/ipcom/port/vxworks/config;
rem set WIND_COMPONENTS=%WIND_HOME%/components

if "%PATH_SET%"=="1" goto end
set PATH_SET=1
set PATH=%WIND_DIR%\workbench-2.6\foundation\4.0.11\x86-win32\bin;%WIND_DIR%\vxworks-6.5\host\x86-win32\bin;%WIND_DIR%;%WIND_DIR%\workbench-2.6\x86-win32\bin;%WIND_DIR%\gnu\3.4.4-vxworks-6.5\x86-win32\bin;%WIND_DIR%\diab\5.5.1.0\WIN32\bin;%PATH%
:end

cls
