::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::  build_cpss.bat
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::  DEVELOPER IMAGE BUILD WRAPPER
::  $Revision: 44 $
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: ARGUMENTS    : [Can be defined in any order]
:: CPU          : ARM5181 ARM5281 ARM78200 ARM78200_BE ARM78200RD ARM78200RD_BE ARMADAXP ARMADAXP_BE
::                  PPC603 PPC85XX PPC85XX_LION_RD XCAT XCAT_BE VC VC8 VC10 VC10_64 BC
:: PP_TYPE      : DX_ALL EX EXMXPM EX_DX_ALL
:: TOOLKIT      : DIAB WB26 WB26_DIAB WB33 WB33_DIAB
:: CROSSBAR     : FOX DUNE
:: PRODUCT      : CPSS_ENABLER
:: UT           : UTF_YES (default) or UTF_NO
:: LUA          : NOLUA (the same as LUA_NO) -  by default LUA code is included, NOLUA removes LUA support
:: GALTIS       : NOGALTIS (the same as GALTIS_NO) -  by default GALTIS code is included, NOGALTIS removes GALTIS support
:: BUS_OPTION	: type of management bus - PCI(PEX) SMI PCI_SMI PCI_SMI_I2C
:: Builld optons: DEV ENVBASE CUST
:: GM_MODE      : GM_LION GM_XCAT2 GM_LION2 GM_PUMA3 GM_BOBCAT2 GM_BOBCAT2_B0 GM_BOBK_CAELUM
:: Additional
::  options     : SUB20
::  options     : NO_COPY_BIN - in developer mode don't copy(or check) binary files from f:\Objects\cpss\bin to c:\temp\cpss_bin
::                For example:build_cpss.bat DX_ALL VC10 NO_COPY_BIN 
::
::  OPTIONAL ARGUMENTS in customer mode:
::
::      variables                                               default value
::------------------------------------------------------------------------
::  COMPILATION_ROOT                %CD%\compilation_root
::  CPSS_PATH                       %CD%
::  USER_BASE                       %CD%
::  BSP_CONFIG_DIR                  %CD%\config
::
::   WIND_BASE         default value depends  on CPU type
::                     for PPC603     --    c:\Tornado
::                     for ARM5181, ARM5281, XCAT  --c:\TornadoARM
::
::      EXAMPLES:
::          set COMPILATION_ROOT=c:\compilation_root
::          set WIND_BASE=c:\localTornado
::
::          build_cpss.bat CPU PP_TYPE
::              in this case objects, BSP and images will go to
::              directory c:\compilation_root
::              Tornado toolkit located in c:\localTornado
::         
::       -------------------------------------------------
::       NO GALTIS CUSTOMER MODE:
::           build_cpss.bat DX_ALL VC10 CUST NOGALTIS
::
::
::  CHECKED CASES:
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::    ##  ::   run command line                        :: Toolkit      :: Compiler
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::      DX
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::    1   ::  build_cpss.bat PPC603  DX_ALL            :: Tornado      ::  GNU
::    2   ::  build_cpss.bat ARM5181 DX_ALL            :: Tornado      ::  GNU
::    3   ::  build_cpss.bat ARM5281 DX_ALL            :: Tornado      ::  GNU
::    4   ::  build_cpss.bat ARM5281 DX_ALL  WB26      :: Workbench2.6 ::  GNU
::    5   ::  build_cpss.bat PPC85XX DX_ALL  WB26      :: Workbench2.6 ::  GNU
::    6   ::  build_cpss.bat XCAT    DX_ALL            :: Tornado      ::  GNU
::    7   ::  build_cpss.bat XCAT_BE DX_ALL            :: Tornado      ::  GNU
::    8   ::  build_cpss.bat ARM_VE  DX_ALL            :: Tornado      ::  GNU
::    9   ::  build_cpss.bat ARM5281 DX_ALL  WB26_DIAB :: Workbench2.6 ::  DIAB
::    10  ::  build_cpss.bat PPC85XX DX_ALL  WB26_DIAB :: Workbench2.6 ::  DIAB
::    11  ::  build_cpss.bat BC      DX_ALL            ::     BC       ::  BC
::    12  ::  build_cpss.bat VC      DX_ALL            ::     VC       ::  VC
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::      EXDX
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::    13  ::  build_cpss.bat PPC603  EX_DX_ALL         :: Tornado      ::  GNU
::    14  ::  build_cpss.bat VC      EX_DX_ALL         ::     VC       ::  VC
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::      EXDXPM
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::    15  ::  build_cpss.bat ARM5281 EXMXPM  WB26_DIAB :: Workbench2.6 ::  DIAB
::    16  ::  build_cpss.bat PPC85XX EXMXPM  WB26_DIAB :: Workbench2.6 ::  DIAB
::    17  ::  build_cpss.bat ARM5281 EXMXPM            :: Tornado      ::  GNU
::    18  ::  build_cpss.bat ARM5281 EXMXPM  WB26      :: Workbench2.6 ::  GNU
::    19  ::  build_cpss.bat PPC85XX EXMXPM  WB26      :: Workbench2.6 ::  GNU
::    20  ::  build_cpss.bat ARM5281 EXMXPM  DUNE      :: Tornado      ::  GNU
::    21  ::  build_cpss.bat VC      EXMXPM_DX_CH      ::     VC       ::  VC
::    22  ::  build_cpss.bat VC      EXMXPM  DUNE      ::     VC       ::  VC
::    23  ::  build_cpss.bat VC      EXMXPM            ::     VC       ::  VC
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::      EX
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::    24  ::  build_cpss.bat PPC603  EX_ALL       FOX      :: Tornado      ::  GNU
::    25  ::  build_cpss.bat VC      EX_ALL                ::     VC       ::  VC
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
@ ECHO OFF

set STARTTIME=%TIME%
echo          ---------------
echo Start    : %STARTTIME%
echo          ---------------

SET CPSS_PATH=%CD%
IF EXIST Build.sh SET CPSS_PATH=%CD%
@echo CPSS_PATH : %CPSS_PATH%
set cpss_tool=%CPSS_PATH%\tools
set TOOL_COMMON=%cpss_tool%\common

rem SET COMMON_BIN=F:\Objects\cpss\bin
rem SET PATH=c:\Program Files (x86)\Git\bin;C:\Python27;%COMMON_BIN%;%COMMON_BIN%\lib;%PATH%.


SET RELEASE_DEP=/cpss
rem for CPSS_PATH\tools\linuxScripts\fix_depfiles.py 
SET COMPILATION_ROOT=
SET USER_BASE=%CD%
SET BSP_CONFIG_DIR=%CD%\config

::::::::::::::::::::::::::
:: RESET LOCAL VARIABLES
::::::::::::::::::::::::::
SET TOOLKIT=
SET TOOLKIT_OPTION=
SET CPU=
SET PP_TYPE=
SET CROSSBAR=

SET UT=UTF_YES
SET UT_OPTION=
SET SCRIPT_MODE=DEV
SET CROSSBAR_OPTION=
SET GM_MODE=
SET BUS_OPTION=
SET ADD_OPT_MODE=
SET NO_COPY_BIN=
SET COPY_BIN=

:::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: DEFINE LISTS OF LEGAL COMMAND LINE PARAMETER VALUES
:::::::::::::::::::::::::::::::::::::::::::::::::::::::
SET CPU_LIST=ARM5181,ARM5281,ARM78200,ARM78200_BE,ARM78200RD,ARM78200RD_BE,ARM_EV,PPC603,PPC85XX,XCAT,XCAT_BE,VC,VC8,VC10,VC10_64,BC,PPC85XX_LION_RD,ARMADAXP,ARMADAXP_BE
SET PP_TYPE_LIST=DX_ALL,EX,EX_ALL,EX_TG,EX_RD,EXMXPM,EX_DX_ALL,EXMXPM_DX_CH
SET TOOLKIT_LIST=DIAB,WB26,WB26_DIAB,WB33,WB33_DIAB
SET CROSSBAR_LIST=FOX,DUNE
SET PRODUCT_LIST=CPSS_ENABLER
SET UT_LIST=UTF_NO
SET LUA_LIST=NOLUA,LUA_NO
SET GALTIS_LIST=NOGALTIS,GALTIS_NO
SET GM_LIST=GM_LION,GM_XCAT2,GM_LION2,GM_PUMA3,GM_BOBCAT2,GM_BOBCAT2_B0,GM_BOBK_CAELUM
SET ADDITIONAL_OPTION_LIST=SUB20
SET SCRIPT_MODE_OPTION_LIST=DEV,CUST,ENVBASE,UNZIP
:: PCI covers PEX
SET BUS_LIST=PCI,SMI,PCI_SMI,PCI_SMI_I2C
SET COPY_BIN_LIST=NO_COPY_BIN

set PRODUCT_DEFAULT=CPSS_ENABLER

::::::::::::::::::::::::::::::::::::::::::::::
:: set local variable RUN_COMPILE_CMD allowing
:: to run CMD for CPSS compiling
::::::::::::::::::::::::::::::::::::::::::::::


:::::::::::::::::::::::::::::::::::::
:: check command line "help" option
:::::::::::::::::::::::::::::::::::::
IF "%1" == "help" ( 
   CALL:HELP_FUNCTION
   GOTO END
)


SET CPSS_TOOLS_PATH=%CPSS_PATH%\tools
SET FILE_ZSH_EXE=%CPSS_TOOLS_PATH%\bin\zsh.exe
SET FILE_SH_EXE=%CPSS_PATH%\tools\bin\sh.exe

::::::::::::::::::::::::::::::::
:: check comand line arguments
::::::::::::::::::::::::::::::::

FOR %%i IN (%1 %2 %3 %4 %5 %6 %7) DO (
    SET FOUND=
    FOR %%j IN (%CPU_LIST%) DO (
        IF %%i == %%j ( 
           SET CPU=%%j
           SET FOUND=TRUE
        )
    )
    FOR %%j IN (%PP_TYPE_LIST%) DO (
        IF %%i == %%j (
           SET PP_TYPE=%%j
           SET FOUND=TRUE
        )
    )
    FOR %%j IN (%TOOLKIT_LIST%) DO (
        IF %%i == %%j ( 
           SET TOOLKIT=%%j
           SET FOUND=TRUE
        )
    )
    FOR %%j IN (%CROSSBAR_LIST%) DO (
        IF %%i == %%j ( 
           SET CROSSBAR=%%j
           SET FOUND=TRUE
        )
    )
    FOR %%j IN (%PRODUCT_LIST%) DO (
        IF %%i == %%j (
           SET PRODUCT=%%j
           SET FOUND=TRUE
        )
    )
    FOR %%j IN (%UT_LIST%) DO (
        IF %%i == %%j (
           SET UT=%%j
           SET FOUND=TRUE
        )
    )
    FOR %%j IN (%LUA_LIST%) DO (
        IF %%i == %%j (
           SET LUA_MODE=%%j
           SET FOUND=TRUE
        )
    )
    FOR %%j IN (%GALTIS_LIST%) DO (
        IF %%i == %%j (
           SET GALTIS_MODE=%%j
           SET FOUND=TRUE
        )
    )
    FOR %%j IN (%GM_LIST%) DO (
        IF %%i == %%j (
           SET GM_MODE=%%j
           SET FOUND=TRUE
        )
    )

    FOR %%j IN (%SCRIPT_MODE_OPTION_LIST%) DO (
        IF %%i == %%j (
           SET SCRIPT_MODE=%%j
           SET FOUND=TRUE
        )
    )

    FOR %%j IN (%ADDITIONAL_OPTION_LIST%) DO (
        IF %%i == %%j (
           SET ADD_OPT_MODE=%%j
           SET FOUND=TRUE
        )
    )

    FOR %%j IN (%BUS_LIST%) DO (
        IF %%i == %%j (
           SET BUS=%%j
           SET FOUND=TRUE
        )    
    )

    FOR %%j IN (%COPY_BIN_LIST%) DO (
        IF %%i == %%j (
           SET NO_COPY_BIN=%%j
           SET FOUND=TRUE
        )    
    )

    IF NOT DEFINED FOUND (
       echo WRONG TOKEN IN COMMAND LINE - %%i
       GOTO:eof
    )    
)

IF "%NO_COPY_BIN%" == "" (
    SET COPY_BIN=YES
) ELSE (
    IF "%NO_COPY_BIN%" == "NO_COPY_BIN" (
        SET COPY_BIN=NO
    ) ELSE (
        SET COPY_BIN=YES
    )
)
IF NOT DEFINED COPY_BIN SET COPY_BIN=YES

IF NOT "%BUS%" == "" (
    SET BUS_OPTION=-b%BUS%
)

IF "%CPU%" == "" (echo CPU required
    goto:EOF)

IF "%PP_TYPE%" == "EX" (
    SET PP_TYPE=EX_ALL
)

IF "%PP_TYPE%" == "" (echo PP_TYPE required
    goto:EOF)

:::::::::::::::::::::::::::::::
:: set UT option
:::::::::::::::::::::::::::::::
IF "%UT%" == "UTF_YES" (SET UT_OPTION=-uUTF_YES)
IF "%UT%" == "UTF_NO" (SET UT_OPTION=-uUTF_NO)

::::::::::::::::::::::::::::::::::::::::::::
:: UNZIP MODE
::::::::::::::::::::::::::::::::::::::::::::

IF "%SCRIPT_MODE%" == "CUST" (
    
    SET LIB_BASE=%CD%
    SET HOST_OPTION=win32
    
    GOTO DO_CUST
)
IF "%SCRIPT_MODE%" == "UNZIP" (
    
    SET LIB_BASE=%CD%
    SET HOST_OPTION=win32
    
    GOTO DO_UNZIP
)

SET PATH=c:\Program Files (x86)\Git\bin;C:\Python27;%PATH%.
IF NOT DEFINED COMMON_F_BIN SET COMMON_F_BIN=F:\Objects\cpss\bin

SET COMMON_F_BIN=F:\Objects\cpss\bin
SET COMMON_BIN=c:\temp\cpss_bin


ECHO COPY_BIN  = %COPY_BIN%

IF %COPY_BIN% == YES (
    @echo "CALL  %CPSS_PATH%\tools\genScripts\service\cpss_bin_copy.bat %COMMON_F_BIN% %COMMON_BIN% %CPSS_PATH%"
    CALL  %CPSS_PATH%\tools\genScripts\service\cpss_bin_copy.bat %COMMON_F_BIN% %COMMON_BIN% %CPSS_PATH%
)

IF NOT DEFINED CPSS_TOOLS_PATH SET CPSS_TOOLS_PATH=%COMMON_BIN%
IF NOT DEFINED LIB_BASE        SET LIB_BASE=%COMMON_BIN%\lib

ECHO "COMMON_BIN : " %COMMON_BIN%
ECHO "CPSS_TOOLS_PATH : " %CPSS_TOOLS_PATH%
ECHO "LIB_BASE : " %LIB_BASE%

SET PATH=%COMMON_BIN%;%LIB_BASE%;%PATH%.
SET CC=


rem ECHO FILE_SH_EXE  = %FILE_SH_EXE%
IF NOT EXIST %FILE_SH_EXE% (
    copy %COMMON_BIN%\sh.exe %FILE_SH_EXE% 
)
IF NOT EXIST %FILE_ZSH_EXE% (
    copy %COMMON_BIN%\zsh.exe %FILE_ZSH_EXE% 
)

IF NOT DEFINED NO_COPY_OBJECTS SET NO_COPY_OBJECTS=TRUE
@echo NO_COPY_OBJECTS : %NO_COPY_OBJECTS%

CALL get_git_param.bat %CPSS_PATH%
rem CALL %cs_config_path% 

SET CPSS_TOOLS_PATH=%COMMON_BIN%
SET LIB_BASE=%COMMON_BIN%\lib

SET HOST_OPTION=win32_marvell_cpss_dev
GOTO RUN_BUILD

:DO_UNZIP
::::::::::::::::::::::::::::::::::::::::::
:: check if required zips exist and unzip
::::::::::::::::::::::::::::::::::::::::::
SET CHECK_FILES=ALL
IF "%PP_TYPE%" == "EX" (
    IF NOT EXIST Cpss-FA-Xbar*.zip (
        echo " >> Cpss-FA-Xbar*.zip required"
        SET CHECK_FILES=NOTALL
        )
)
IF NOT EXIST EnablerSuite*.zip (
        ECHO " >> EnablerSuite*.zip is required"
        SET CHECK_FILES=NOTALL
)
IF NOT EXIST ExtUtils*.zip (
        ECHO " >> ExtUtils*.zip is required"
        SET CHECK_FILES=NOTALL
)

IF "%GALTIS_MODE%" == "GALTIS_NO" (
	SET GALTIS_MODE="NOGALTIS"
)
IF NOT "%GALTIS_MODE%" == "NOGALTIS" (
    IF NOT EXIST GaltisSuite*.zip (
        ECHO " >> GaltisSuite*.zip is required"
        SET CHECK_FILES=NOTALL
        )
)

IF NOT EXIST ReferenceCode*.zip (
        ECHO " >> ReferenceCode*.zip is required"
        SET CHECK_FILES=NOTALL
)
IF "%UT%" == "UTF_YES" (
    IF NOT EXIST UT*.zip (
        ECHO " >> UT*.zip is required"
        SET CHECK_FILES=NOTALL
    )
)
IF "%LUA_MODE%" == "LUA_NO" (
	SET LUA_MODE=NOLUA
)
IF NOT "%LUA_MODE%" == "NOLUA" (
    IF NOT EXIST Lua*.zip (
        ECHO " >> LuaSuite*.zip is required"
        SET CHECK_FILES=NOTALL
        )
)

IF %CPU% == VC      (goto CHECK_SIMULATION)
IF %CPU% == VC8     (goto CHECK_SIMULATION)
IF %CPU% == VC10    (goto CHECK_SIMULATION)
IF %CPU% == VC10_64 (goto CHECK_SIMULATION)
IF %CPU% == BC      (goto CHECK_SIMULATION)
goto:CHECK_BSP

:CHECK_SIMULATION
IF NOT EXIST Simulation*.zip (
        ECHO " >> Simulation*.zip is required"
        SET CHECK_FILES=NOTALL
)
IF "%PP_TYPE%" == "DX_ALL" (
    IF NOT "%GM_MODE%" == ""  (
        IF NOT EXIST GM_DX*.zip (
            ECHO " >> GM_DX*.zip is required"
            SET CHECK_FILES=NOTALL
        )
    )
)
IF "%PP_TYPE%" == "EXMXPM" (
    IF NOT "%GM_MODE%" == "" (
        IF NOT EXIST GM_EXMXPM*.zip (
            ECHO " >> GM_EXMXPM*.zip is required"
            SET CHECK_FILES=NOTALL
        )
    )
)

goto:END_OF_CHECK_UNZIP

:CHECK_BSP
IF NOT EXIST BSP*.zip (
        ECHO " >> BSP*.zip is required"
        SET CHECK_FILES=NOTALL
        )
)

:END_OF_CHECK_UNZIP

IF  NOT "%CHECK_FILES%" == "ALL" (
    ECHO " >> ERROR - Missing ZIP files in work directory"
    goto:EOF
) ELSE (
    ECHO " >> All required ZIP files exist in work directory")

::::::::::::::::::::::::::::::::::::::::::::
:: if all required zip files exist do unzip
::::::::::::::::::::::::::::::::::::::::::::

SET COMPILATION_ROOT=%CD%\compilation_root

::::::::::::::::::::::::::::::::::::::::::
:: check if required zips exist and unzip
::::::::::::::::::::::::::::::::::::::::::
IF "%PP_TYPE%" == "EX" (
    IF EXIST Cpss-FA-Xbar*.zip (
        echo ".\tools\bin\unzip -o Cpss-FA-Xbar*.zip"
        .\tools\bin\unzip -o Cpss-FA-Xbar*.zip
    ) ELSE (
        ECHO "Cpss-FA-Xbar*.zip is required"
        goto:EOF)
)


IF NOT EXIST EnablerSuite*.zip (
        ECHO EnablerSuite*.zip is required
        goto:EOF
        ) ELSE ( .\tools\bin\unzip -o EnablerSuite*.zip)
IF NOT EXIST ExtUtils*.zip (
        ECHO ExtUtils*.zip is required
        goto:EOF
        ) ELSE ( .\tools\bin\unzip -o ExtUtils*.zip)

IF NOT "%GALTIS_MODE%" == "NOGALTIS" (
    IF NOT EXIST GaltisSuite*.zip (
        ECHO GaltisSuite*.zip is required
        goto:EOF
        ) ELSE ( .\tools\bin\unzip -o GaltisSuite*.zip)
)

IF NOT EXIST ReferenceCode*.zip (
        ECHO ReferenceCode*.zip is required
        goto:EOF
        ) ELSE ( .\tools\bin\unzip -o -d cpssEnabler ReferenceCode*.zip)
IF "%UT%" == "UTF_YES" (
    IF NOT EXIST UT*.zip (
            ECHO UT*.zip is required
            goto:EOF
    ) ELSE (.\tools\bin\unzip -o UT*.zip)
)
IF NOT "%LUA_MODE%" == "NOLUA" (
    IF NOT EXIST LuaSuite*.zip (
            ECHO LuaSuite*.zip is required
            goto:EOF
            ) ELSE (.\tools\bin\unzip -o LuaSuite*.zip)
)
IF NOT EXIST GaltisSuite*.zip (
::        ECHO GaltisSuite*.zip is required
::        goto:EOF
        ) ELSE (.\tools\bin\unzip -o GaltisSuite*.zip)


IF %CPU% == VC      (goto UNZIP_SIMULATION)
IF %CPU% == VC8     (goto UNZIP_SIMULATION)
IF %CPU% == VC10    (goto UNZIP_SIMULATION)
IF %CPU% == VC10_64 (goto UNZIP_SIMULATION)
IF %CPU% == BC      (goto UNZIP_SIMULATION)
goto:UNZIP_BSP

:UNZIP_SIMULATION
IF NOT EXIST Simulation*.zip (
        ECHO Simulation*.zip is required
        goto:EOF
        ) ELSE (
        .\tools\bin\unzip -o Simulation*.zip)

IF "%PP_TYPE%" == "DX_ALL" (
    IF NOT "%GM_MODE%" == "" (
        IF NOT EXIST GM_DX*.zip (
            ECHO " >> GM_DX*.zip is required"
            goto:EOF
        ) ELSE (
            .\tools\bin\unzip -o GM_DX*.zip)
    )
)

IF "%PP_TYPE%" == "EXMXPM" (
    IF NOT EXIST GM_EXMXPM*.zip (
        ECHO " >> GM_EXMXPM*.zip is required"
        goto:EOF
    ) ELSE (
        .\tools\bin\unzip -o GM_EXMXPM*.zip)
)

goto:END_UNZIP

:UNZIP_BSP
IF NOT EXIST BSP*.zip (
        ECHO BSP*.zip is required
        goto:EOF
        ) ELSE (
        mkdir %CD%\config
        IF %CPU% == PPC85XX (.\tools\bin\unzip -o -d config BSP*.zip GDA8548_6.5*)
        IF %CPU% == PPC603 (.\tools\bin\unzip -o -d config BSP*.zip mv_pmc8245*)
        IF %CPU% == ARM5181 (.\tools\bin\unzip -o -d config BSP*.zip db_88f5181_prpmc*)
        IF %CPU% == ARM5281 (.\tools\bin\unzip -o -d config BSP*.zip db_88f5281_mng*)
        IF %CPU% == XCAT (.\tools\bin\unzip -o -d config BSP*.zip xcat_bsp*)
        IF %CPU% == XCAT_BE (.\tools\bin\unzip -o -d config BSP*.zip xcat_bsp*)
        IF %CPU% == ARM78200RD (.\tools\bin\unzip -o -d config BSP*.zip dd_rd_bsp*)
        IF %CPU% == ARM78200RD_BE (.\tools\bin\unzip -o -d config BSP*.zip dd_rd_bsp*)
        IF %CPU% == ARM78200 (.\tools\bin\unzip -o -d config BSP*.zip dd_bsp*)
        IF %CPU% == ARM78200_BE (.\tools\bin\unzip -o -d config BSP*.zip dd_bsp*)
)

:END_UNZIP
::::::::::::::::::::::::::::::::::::
:: set environment variables
::::::::::::::::::::::::::::::::::::
:DO_CUST
IF "%COMPILATION_ROOT%" == "" SET COMPILATION_ROOT=%CD%\compilation_root
SET CPSS_PATH=%CD%
SET USER_BASE=%CD%
SET BSP_CONFIG_DIR=%CD%\config

GOTO RUN_BUILD
:::::::::::::::::::::::::::::::::::::::::::::::::::::::


:RUN_BUILD

ECHO "CPSS_TOOLS_PATH : %CPSS_TOOLS_PATH%"
ECHO "RELEASE_DEP : %RELEASE_DEP%"
ECHO "LIB_BASE : %LIB_BASE%"
ECHO "CPSS_PATH : %CPSS_PATH%"
ECHO "USER_BASE : %USER_BASE%"
ECHO "BSP_CONFIG_DIR : %BSP_CONFIG_DIR%"
ECHO "COMPILATION_ROOT : %COMPILATION_ROOT%"

:::::::::::::::::::::::::::::::
:: define optional parameters
:::::::::::::::::::::::::::::::
IF "%TOOLKIT%" == "DIAB" SET TOOLKIT_OPTION=-Ttornado_diab
IF "%TOOLKIT%" == "WB26" SET TOOLKIT_OPTION=-Tworkbench26
IF "%TOOLKIT%" == "WB26_DIAB" SET TOOLKIT_OPTION=-Tworkbench26_diab
IF "%TOOLKIT%" == "WB33" SET TOOLKIT_OPTION=-Tworkbench33
IF "%TOOLKIT%" == "WB33_DIAB" SET TOOLKIT_OPTION=-Tworkbench33_diab

IF "%SCRIPT_MODE%" == "ENVBASE" SET SCRIPT_MODE_OPTION=-Denvbase
IF "%SCRIPT_MODE%" == "DEV" SET SCRIPT_MODE_OPTION=-Ddev
IF "%SCRIPT_MODE%" == "CUST" SET SCRIPT_MODE_OPTION=-Dcust

IF "%CROSSBAR%" == "FOX"  SET CROSSBAR_OPTION=-xFOX
IF "%CROSSBAR%" == "DUNE"  SET CROSSBAR_OPTION=-xDUNE
IF "%PP_TYPE%" == "DX_ALL"  SET LUA_OPTION=-l
IF "%PP_TYPE%" == "EXMXPM"  SET LUA_OPTION=-l
SET GM_OPTION=

IF "%LUA_MODE%" == "NOLUA"  SET LUA_OPTION=
IF "%GALTIS_MODE%" == "NOGALTIS"  SET GALTIS_OPTION=-Gno_galtis
IF "%GM_MODE%" == "GM_LION"  SET GM_OPTION=-glion
IF "%GM_MODE%" == "GM_XCAT2"  SET GM_OPTION=-gxcat2
IF "%GM_MODE%" == "GM_LION2"  SET GM_OPTION=-glion2
IF "%GM_MODE%" == "GM_PUMA3"  SET GM_OPTION=-gpuma3
IF "%GM_MODE%" == "GM_BOBCAT2"  SET GM_OPTION=-gbobcat2
IF "%GM_MODE%" == "GM_BOBCAT2_B0"  SET GM_OPTION=-gbobcat2_b0
IF "%GM_MODE%" == "GM_BOBK_CAELUM"  SET GM_OPTION=-gbobk_caelum

IF "%ADD_OPT_MODE%" == "SUB20"  SET ADD_OPTION=-hsub20


SET CPU_OPTION=%CPU%
SET PP_OPTION=%PP_TYPE%

::::::::::::::::::::::::::::::::::::::::::::::::::
:: define parameters not defined in command line
::::::::::::::::::::::::::::::::::::::::::::::::::
IF "%CPU%" == "BC"      goto SIM_SETUP
IF "%CPU%" == "VC"      goto SIM_SETUP
IF "%CPU%" == "VC8"     goto SIM_SETUP
IF "%CPU%" == "VC10"    goto SIM_SETUP
IF "%CPU%" == "VC10_64" goto SIM_SETUP

:: SET HOST_OPTION=win32_marvell_cpss_dev
SET OS_OPTION=vxWorks
SET BUILD_OPTION=HW
SET PRODUCT_OPTION=CPSS_ENABLER
goto:BUILD_IMAGE

:SIM_SETUP
:: SET HOST_OPTION=win32_marvell_cpss_dev
SET OS_OPTION=win32
SET BUILD_OPTION=simulation

SET CPU_OPTION=i386
IF "%CPU%" == "VC10_64" SET CPU_OPTION=i386_64

IF "%CPU%" == "BC"      SET TOOLKIT_OPTION=-Tbc
IF "%CPU%" == "VC8"     SET TOOLKIT_OPTION=-Tvc8
IF "%CPU%" == "VC10"    SET TOOLKIT_OPTION=-Tvc10
IF "%CPU%" == "VC10_64" SET TOOLKIT_OPTION=-Tvc10
SET PRODUCT_OPTION=CPSS_ENABLER
goto:BUILD_IMAGE


::::::::::::::::::::::::::::::::::::::::
:: RUN BUILD.SH WITH DEFINED PARAMETERS
::::::::::::::::::::::::::::::::::::::::
:BUILD_IMAGE

IF NOT DEFINED RUN_COMPILE_CMD SET RUN_COMPILE_CMD=YES

SET OPTIONS=%UT_OPTION% %TOOLKIT_OPTION% %SCRIPT_MODE_OPTION% %CROSSBAR_OPTION% %LUA_OPTION% %GALTIS_OPTION% %GM_OPTION% %BUS_OPTION% %ADD_OPTION%
SET ARGS=%HOST_OPTION% %CPU_OPTION% %OS_OPTION% %PP_OPTION% %BUILD_OPTION% %PRODUCT_OPTION%

@echo OPTIONS : %OPTIONS%
@echo ARGS : %ARGS%

@echo FILE_ZSH_EXE=%FILE_ZSH_EXE%
@echo %FILE_ZSH_EXE% %ZSH_DEBUG% Build.sh %OPTIONS% %ARGS% %COMPILATION_ROOT%

%FILE_ZSH_EXE% %ZSH_DEBUG% Build.sh %OPTIONS% %ARGS% %COMPILATION_ROOT%
rem F:\Objects\cpss\bin\zsh.exe %ZSH_DEBUG% Build.sh %OPTIONS% %ARGS% %COMPILATION_ROOT%

:::::::::::::::::::::::::::::::::::::::::
::  if script in mode UNZIP the compile
::  window should not be opened
:: RUN_COMPILE_CMD variable defines if the second
:: window "COMPILE" should be run
:::::::::::::::::::::::::::::::::::::::::
IF NOT "%SCRIPT_MODE%" == "UNZIP" (
    IF "%RUN_COMPILE_CMD%" == "YES" SET RUN_COMPILE_CMD=RUN
)

GOTO:END

:HELP_FUNCTION
ECHO ------------------------------------------------------------------ 
ECHO COMMAND LINE FORMAT:

ECHO "build_cpss.bat < Packet Processor > < CPU/Compiler > < Options >"
ECHO ------------------------------------------------------------------ 
ECHO CPU/Compiler:

ECHO "%CPU_LIST%"
ECHO ------------------------------------------------------------------ 
ECHO Packet Processor:

ECHO "%PP_TYPE_LIST%"
ECHO ------------------------------------------------------------------ 
ECHO OPTIONS:
ECHO   ****************  
ECHO   Toolkit options:
ECHO   ****************  
ECHO     DIAB          - use Tornado DIAB compiler 
ECHO     WB26          - use Workbanch 2.6
ECHO     WB26_DIAB     - use DIAB compiler from Workbanch 2.6
ECHO     WB33          - use Workbanch 3.3
ECHO     WB26_DIAB     - use DIAB compiler from Workbanch 3.3

ECHO   ***********  
ECHO   UT options:
ECHO   ***********  
ECHO     UT_NO         - UT code not included 

ECHO   ***********  
ECHO   LUA options:
ECHO   ***********  
ECHO     NOLUA         - Lua code not included 
ECHO   ***********  
ECHO   GALTIS options:
ECHO   ***********  
ECHO     NOGALTIS      - Galtis code not included 
ECHO   ***********  
ECHO   GM options:
ECHO   ***********  
ECHO     %GM_LIST% 
ECHO   ********************  
ECHO   SCRIPT MODE options:
ECHO   ********************  
ECHO     UNZIP      - specifies the following steps: 
ECHO                    ~ unzip CPSS zip files to working directory 
ECHO                    ~ compile unziped source files
ECHO                    ~ build CPSS appDemo
ECHO     CUST       - specifies the following steps: 
ECHO                    ~ compile unziped source files
ECHO                    ~ build CPSS appDemo
 
ECHO ------------------------------------------------------------------ 

GOTO:END
 
:END
rem    IF EXIST %FILE_SH_EXE% (
rem        del  /F /Q %FILE_SH_EXE% 
rem    )

SET ZSH_DEBUG=
SET NO_COPY_OBJECTS=

CALL %TOOL_COMMON%\duration.bat %STARTTIME%
