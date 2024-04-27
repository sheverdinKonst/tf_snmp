/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* cpssExtServices.c
*
* DESCRIPTION:
*       External Driver wrapper. definitions for bind OS , external driver
*       dependent services and trace services to CPSS .
*
*
* FILE REVISION NUMBER:
*       $Revision:  24 $
*******************************************************************************/

/************* Includes *******************************************************/

#include <cpss/extServices/cpssExtServices.h>
#include <cpss/extServices/private/prvCpssBindFunc.h>
#include <cpss/generic/log/prvCpssLog.h>

/* macro to force casting between 2 functions prototypes */
#define FORCE_FUNC_CAST     (void *)

/* define a STUB function that "do nothing" but return "not implemented" */
/* the function parameters do not matter , since the function is "forced to
   do casting"
*/

#define STR_NOT_IMPLEMENTED_CNS " extServiceFuncNotImplementedCalled \n"
static GT_STATUS extServiceFuncNotImplementedCalled
(
    void
)
{
    if(cpssOsPrintf != (CPSS_OS_IO_PRINTF_FUNC)extServiceFuncNotImplementedCalled)
    {
        /* we already have "printf" from the application
          but current pointer of a function was not initialized yet */
        cpssOsPrintf(STR_NOT_IMPLEMENTED_CNS);
    }

    return GT_NOT_IMPLEMENTED;
}
/* macro to replace the content of pointer only if replaced with non-NULL pointer */
#define REPLACE_IF_NOT_NULL_MAC(_target,_source)   \
    _target = _source ? _source : _target
/************* Prototypes *****************************************************/

/* PRV_CPSS_BIND_FUNC_STC -
*    structure that hold the  functions needed be bound to cpss.
*/
typedef struct{
    CPSS_EXT_DRV_FUNC_BIND_STC   extDrv;
    CPSS_OS_FUNC_BIND_STC        os;
    CPSS_TRACE_FUNC_BIND_STC     trace;
}PRV_CPSS_BIND_FUNC_STC;

/* need to fill stub function */
static PRV_CPSS_BIND_FUNC_STC   prvCpssBindFuncs;

CPSS_EXT_DRV_MGMT_CACHE_FLUSH_FUNC      cpssExtDrvMgmtCacheFlush                      = (CPSS_EXT_DRV_MGMT_CACHE_FLUSH_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXT_DRV_MGMT_CACHE_INVALIDATE_FUNC cpssExtDrvMgmtCacheInvalidate                 = (CPSS_EXT_DRV_MGMT_CACHE_INVALIDATE_FUNC) extServiceFuncNotImplementedCalled;

CPSS_EXTDRV_DMA_WRITE_FUNC  cpssExtDrvDmaWrite                                        = (CPSS_EXTDRV_DMA_WRITE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_DMA_READ_FUNC   cpssExtDrvDmaRead                                         = (CPSS_EXTDRV_DMA_READ_FUNC) extServiceFuncNotImplementedCalled;

CPSS_EXTDRV_ETH_PORT_RX_INIT_FUNC       cpssExtDrvEthPortRxInit                       = (CPSS_EXTDRV_ETH_PORT_RX_INIT_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_ETH_RAW_SOCKET_MODE_SET_FUNC       cpssExtDrvEthRawSocketModeSet          = (CPSS_EXTDRV_ETH_RAW_SOCKET_MODE_SET_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_ETH_RAW_SOCKET_MODE_GET_FUNC       cpssExtDrvEthRawSocketModeGet          = (CPSS_EXTDRV_ETH_RAW_SOCKET_MODE_GET_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_LINUX_MODE_SET_FUNC       cpssExtDrvLinuxModeSet          = (CPSS_EXTDRV_LINUX_MODE_SET_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_LINUX_MODE_GET_FUNC       cpssExtDrvLinuxModeGet          = (CPSS_EXTDRV_LINUX_MODE_GET_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_HSU_MEM_BASE_ADDR_GET_FUNC         cpssExtDrvHsuMemBaseAddrGet            = (CPSS_EXTDRV_HSU_MEM_BASE_ADDR_GET_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_HSU_WARM_RESTART_FUNC              cpssExtDrvHsuWarmRestart               = (CPSS_EXTDRV_HSU_WARM_RESTART_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_HSU_INBOUND_SDMA_ENABLE_FUNC       cpssExtDrvInboundSdmaEnable            = (CPSS_EXTDRV_HSU_INBOUND_SDMA_ENABLE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_HSU_INBOUND_SDMA_DISABLE_FUNC      cpssExtDrvInboundSdmaDisable           = (CPSS_EXTDRV_HSU_INBOUND_SDMA_DISABLE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_HSU_INBOUND_SDMA_STATE_GET_FUNC    cpssExtDrvInboundSdmaStateGet          = (CPSS_EXTDRV_HSU_INBOUND_SDMA_STATE_GET_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_ETH_PORT_TX_INIT_FUNC       cpssExtDrvEthPortTxInit                       = (CPSS_EXTDRV_ETH_PORT_TX_INIT_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_ETH_PORT_ENABLE_FUNC        cpssExtDrvEthPortEnable                       = (CPSS_EXTDRV_ETH_PORT_ENABLE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_ETH_PORT_DISABLE_FUNC       cpssExtDrvEthPortDisable                      = (CPSS_EXTDRV_ETH_PORT_DISABLE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_ETH_PORT_TX_FUNC            cpssExtDrvEthPortTx                           = (CPSS_EXTDRV_ETH_PORT_TX_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_ETH_PORT_INPUT_HOOK_ADD_FUNC  cpssExtDrvEthInputHookAdd                   = (CPSS_EXTDRV_ETH_PORT_INPUT_HOOK_ADD_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_ETH_RAW_SOCKET_RX_HOOK_ADD_FUNC  cpssExtDrvEthRawSocketRxHookAdd                   = (CPSS_EXTDRV_ETH_RAW_SOCKET_RX_HOOK_ADD_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_ETH_PORT_TX_COMPLETE_HOOK_ADD_FUNC  cpssExtDrvEthTxCompleteHookAdd        = (CPSS_EXTDRV_ETH_PORT_TX_COMPLETE_HOOK_ADD_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_ETH_PORT_RX_PACKET_FREE_FUNC  cpssExtDrvEthRxPacketFree                   = (CPSS_EXTDRV_ETH_PORT_RX_PACKET_FREE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_ETH_PORT_TX_MODE_SET_FUNC     cpssExtDrvEthPortTxModeSet                  = (CPSS_EXTDRV_ETH_PORT_TX_MODE_SET_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_ETH_CPU_CODE_TO_QUEUE         cpssExtDrvEthCpuCodeToQueue                 = (CPSS_EXTDRV_ETH_CPU_CODE_TO_QUEUE) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_ETH_CPU_PREPEND_TWO_BYTES_FUNC cpssExtDrvEthPrePendTwoBytesHeaderSet      = (CPSS_EXTDRV_ETH_CPU_PREPEND_TWO_BYTES_FUNC) extServiceFuncNotImplementedCalled;

CPSS_EXTDRV_INT_CONNECT_FUNC        cpssExtDrvIntConnect                              = (CPSS_EXTDRV_INT_CONNECT_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_INT_ENABLE_FUNC         cpssExtDrvIntEnable                               = (CPSS_EXTDRV_INT_ENABLE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_INT_DISABLE_FUNC        cpssExtDrvIntDisable                              = (CPSS_EXTDRV_INT_DISABLE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_INT_LOCK_MODE_SET_FUNC  cpssExtDrvSetIntLockUnlock                        = (CPSS_EXTDRV_INT_LOCK_MODE_SET_FUNC) extServiceFuncNotImplementedCalled;

CPSS_EXTDRV_PCI_CONFIG_WRITE_REG_FUNC      cpssExtDrvPciConfigWriteReg                = (CPSS_EXTDRV_PCI_CONFIG_WRITE_REG_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_PCI_CONFIG_READ_REG_FUNC       cpssExtDrvPciConfigReadReg                 = (CPSS_EXTDRV_PCI_CONFIG_READ_REG_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_PCI_DEV_FIND_FUNC              cpssExtDrvPciFindDev                       = (CPSS_EXTDRV_PCI_DEV_FIND_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_PCI_INT_VEC_GET_FUNC           cpssExtDrvGetPciIntVec                     = (CPSS_EXTDRV_PCI_INT_VEC_GET_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_INT_MASK_GET_FUNC              cpssExtDrvGetIntMask                       = (CPSS_EXTDRV_INT_MASK_GET_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_PCI_COMBINED_ACCESS_ENABLE_FUNC cpssExtDrvEnableCombinedPciAccess         = (CPSS_EXTDRV_PCI_COMBINED_ACCESS_ENABLE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_PCI_DOUBLE_WRITE_FUNC           cpssExtDrvPciDoubleWrite                  = (CPSS_EXTDRV_PCI_DOUBLE_WRITE_FUNC)extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_PCI_DOUBLE_READ_FUNC            cpssExtDrvPciDoubleRead                   = (CPSS_EXTDRV_PCI_DOUBLE_READ_FUNC)extServiceFuncNotImplementedCalled;

CPSS_EXTDRV_HW_IF_SMI_INIT_DRIVER_FUNC             cpssExtDrvHwIfSmiInitDriver        = (CPSS_EXTDRV_HW_IF_SMI_INIT_DRIVER_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_HW_IF_SMI_WRITE_REG_FUNC               cpssExtDrvHwIfSmiWriteReg          = (CPSS_EXTDRV_HW_IF_SMI_WRITE_REG_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_HW_IF_SMI_READ_REG_FUNC                cpssExtDrvHwIfSmiReadReg           = (CPSS_EXTDRV_HW_IF_SMI_READ_REG_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_HW_IF_SMI_TASK_REG_RAM_READ_FUNC       cpssExtDrvHwIfSmiTskRegRamRead     = (CPSS_EXTDRV_HW_IF_SMI_TASK_REG_RAM_READ_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_HW_IF_SMI_TASK_REG_RAM_WRITE_FUNC      cpssExtDrvHwIfSmiTskRegRamWrite    = (CPSS_EXTDRV_HW_IF_SMI_TASK_REG_RAM_WRITE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_HW_IF_SMI_TASK_REG_VEC_READ_FUNC       cpssExtDrvHwIfSmiTskRegVecRead     = (CPSS_EXTDRV_HW_IF_SMI_TASK_REG_VEC_READ_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_HW_IF_SMI_TASK_REG_VEC_WRITE_FUNC      cpssExtDrvHwIfSmiTskRegVecWrite    = (CPSS_EXTDRV_HW_IF_SMI_TASK_REG_VEC_WRITE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_HW_IF_SMI_TASK_WRITE_REG_FUNC          cpssExtDrvHwIfSmiTaskWriteReg      = (CPSS_EXTDRV_HW_IF_SMI_TASK_WRITE_REG_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_HW_IF_SMI_TASK_READ_REG_FUNC           cpssExtDrvHwIfSmiTaskReadReg       = (CPSS_EXTDRV_HW_IF_SMI_TASK_READ_REG_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_HW_IF_SMI_INTERRUPT_WRITE_REG_FUNC     cpssExtDrvHwIfSmiInterruptWriteReg = (CPSS_EXTDRV_HW_IF_SMI_INTERRUPT_WRITE_REG_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_HW_IF_SMI_INTERRUPT_READ_REG_FUNC      cpssExtDrvHwIfSmiInterruptReadReg  = (CPSS_EXTDRV_HW_IF_SMI_INTERRUPT_READ_REG_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_HW_IF_SMI_DEV_VENDOR_ID_GET_FUNC       cpssExtDrvSmiDevVendorIdGet        = (CPSS_EXTDRV_HW_IF_SMI_DEV_VENDOR_ID_GET_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_HW_IF_SMI_TASK_WRITE_REG_FIELD_FUNC    cpssExtDrvHwIfSmiTaskWriteRegField = (CPSS_EXTDRV_HW_IF_SMI_TASK_WRITE_REG_FIELD_FUNC) extServiceFuncNotImplementedCalled;

CPSS_EXTDRV_HW_IF_TWSI_INIT_DRIVER_FUNC  cpssExtDrvHwIfTwsiInitDriver                 = (CPSS_EXTDRV_HW_IF_TWSI_INIT_DRIVER_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_HW_IF_TWSI_WRITE_REG_FUNC    cpssExtDrvHwIfTwsiWriteReg                   = (CPSS_EXTDRV_HW_IF_TWSI_WRITE_REG_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_HW_IF_TWSI_READ_REG_FUNC     cpssExtDrvHwIfTwsiReadReg                    = (CPSS_EXTDRV_HW_IF_TWSI_READ_REG_FUNC) extServiceFuncNotImplementedCalled;

CPSS_EXTDRV_I2C_MGMT_MASTER_INIT_FUNC      cpssExtDrvI2cMgmtMasterInit                = (CPSS_EXTDRV_I2C_MGMT_MASTER_INIT_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_MGMT_READ_REGISTER_FUNC        cpssExtDrvMgmtReadRegister                 = (CPSS_EXTDRV_MGMT_READ_REGISTER_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_MGMT_WRITE_REGISTER_FUNC       cpssExtDrvMgmtWriteRegister                = (CPSS_EXTDRV_MGMT_WRITE_REGISTER_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_MGMT_ISR_READ_REGISTER_FUNC    cpssExtDrvMgmtIsrReadRegister              = (CPSS_EXTDRV_MGMT_ISR_READ_REGISTER_FUNC) extServiceFuncNotImplementedCalled;
CPSS_EXTDRV_MGMT_ISR_WRITE_REGISTER_FUNC   cpssExtDrvMgmtIsrWriteRegister             = (CPSS_EXTDRV_MGMT_ISR_WRITE_REGISTER_FUNC) extServiceFuncNotImplementedCalled;

CPSS_EXTDRV_DRAGONITE_GET_BASE_ADDRESS_FUNC cpssExtDrvDragoniteShMemBaseAddrGet             = (CPSS_EXTDRV_DRAGONITE_GET_BASE_ADDRESS_FUNC) extServiceFuncNotImplementedCalled;

CPSS_OS_INET_NTOHL_FUNC  cpssOsNtohl                                                  = (CPSS_OS_INET_NTOHL_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_INET_HTONL_FUNC  cpssOsHtonl                                                  = (CPSS_OS_INET_HTONL_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_INET_NTOHS_FUNC  cpssOsNtohs                                                  = (CPSS_OS_INET_NTOHS_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_INET_HTONS_FUNC  cpssOsHtons                                                  = (CPSS_OS_INET_HTONS_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_INET_NTOA_FUNC   cpssOsInetNtoa                                               = (CPSS_OS_INET_NTOA_FUNC) extServiceFuncNotImplementedCalled;

CPSS_OS_INT_ENABLE_FUNC    cpssOsIntEnable                                            = (CPSS_OS_INT_ENABLE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_INT_DISABLE_FUNC   cpssOsIntDisable                                           = (CPSS_OS_INT_DISABLE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_INT_MODE_SET_FUNC  cpssOsSetIntLockUnlock                                     = (CPSS_OS_INT_MODE_SET_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_INT_CONNECT_FUNC   cpssOsInterruptConnect                                     = (CPSS_OS_INT_CONNECT_FUNC) extServiceFuncNotImplementedCalled;

CPSS_OS_IO_BIND_STDOUT_FUNC cpssOsBindStdOut                                          = (CPSS_OS_IO_BIND_STDOUT_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_IO_PRINTF_FUNC      cpssOsPrintf                                              = (CPSS_OS_IO_PRINTF_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_IO_VPRINTF_FUNC     cpssOsVprintf                                             = (CPSS_OS_IO_VPRINTF_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_IO_SPRINTF_FUNC     cpssOsSprintf                                             = (CPSS_OS_IO_SPRINTF_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_IO_VSPRINTF_FUNC    cpssOsVsprintf                                            = (CPSS_OS_IO_VSPRINTF_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_IO_PRINT_SYNC_FUNC  cpssOsPrintSync                                           = (CPSS_OS_IO_PRINT_SYNC_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_IO_GETS_FUNC        cpssOsGets                                                = (CPSS_OS_IO_GETS_FUNC) extServiceFuncNotImplementedCalled;

CPSS_OS_IO_FOPEN_FUNC       cpssOsFopen                                               = (CPSS_OS_IO_FOPEN_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_IO_FCLOSE_FUNC      cpssOsFclose                                              = (CPSS_OS_IO_FCLOSE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_IO_REWIND_FUNC      cpssOsRewind                                              = (CPSS_OS_IO_REWIND_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_IO_FPRINTF_FUNC     cpssOsFprintf                                             = (CPSS_OS_IO_FPRINTF_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_IO_FGETS_FUNC       cpssOsFgets                                               = (CPSS_OS_IO_FGETS_FUNC) extServiceFuncNotImplementedCalled;

CPSS_OS_BZERO_FUNC             cpssOsBzero                                            = (CPSS_OS_BZERO_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_MEM_SET_FUNC           cpssOsMemSet                                           = (CPSS_OS_MEM_SET_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_MEM_CPY_FUNC           cpssOsMemCpy                                           = (CPSS_OS_MEM_CPY_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_MEM_CMP_FUNC           cpssOsMemCmp                                           = (CPSS_OS_MEM_CMP_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_STATIC_MALLOC_FUNC     cpssOsStaticMalloc                                     = (CPSS_OS_STATIC_MALLOC_FUNC) extServiceFuncNotImplementedCalled;
#ifdef OS_MALLOC_MEMORY_LEAKAGE_DBG
    CPSS_OS_MALLOC_FUNC            cpssOsMalloc_MemoryLeakageDbg                      = (CPSS_OS_MALLOC_FUNC) extServiceFuncNotImplementedCalled;
    CPSS_OS_REALLOC_FUNC           cpssOsRealloc_MemoryLeakageDbg                     = (CPSS_OS_REALLOC_FUNC) extServiceFuncNotImplementedCalled;
    CPSS_OS_FREE_FUNC              cpssOsFree_MemoryLeakageDbg                        = (CPSS_OS_FREE_FUNC) extServiceFuncNotImplementedCalled;
#else  /*!OS_MALLOC_MEMORY_LEAKAGE_DBG*/
    CPSS_OS_MALLOC_FUNC            cpssOsMalloc                                       = (CPSS_OS_MALLOC_FUNC) extServiceFuncNotImplementedCalled;
    CPSS_OS_REALLOC_FUNC           cpssOsRealloc                                      = (CPSS_OS_REALLOC_FUNC) extServiceFuncNotImplementedCalled;
    CPSS_OS_FREE_FUNC              cpssOsFree                                         = (CPSS_OS_FREE_FUNC) extServiceFuncNotImplementedCalled;
#endif /*!OS_MALLOC_MEMORY_LEAKAGE_DBG*/

CPSS_OS_CACHE_DMA_MALLOC_FUNC  cpssOsCacheDmaMalloc                                   = (CPSS_OS_CACHE_DMA_MALLOC_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_CACHE_DMA_FREE_FUNC    cpssOsCacheDmaFree                                     = (CPSS_OS_CACHE_DMA_FREE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_PHY_TO_VIRT_FUNC       cpssOsPhy2Virt                                         = (CPSS_OS_PHY_TO_VIRT_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_VIRT_TO_PHY_FUNC       cpssOsVirt2Phy                                         = (CPSS_OS_VIRT_TO_PHY_FUNC) extServiceFuncNotImplementedCalled;

CPSS_OS_RAND_FUNC  cpssOsRand                                                         = (CPSS_OS_RAND_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_SRAND_FUNC cpssOsSrand                                                        = (CPSS_OS_SRAND_FUNC) extServiceFuncNotImplementedCalled;

CPSS_OS_MUTEX_CREATE_FUNC        cpssOsMutexCreate                                    = (CPSS_OS_MUTEX_CREATE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_MUTEX_DELETE_FUNC        cpssOsMutexDelete                                    = (CPSS_OS_MUTEX_DELETE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_MUTEX_LOCK_FUNC          cpssOsMutexLock                                      = (CPSS_OS_MUTEX_LOCK_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_MUTEX_UNLOCK_FUNC        cpssOsMutexUnlock                                    = (CPSS_OS_MUTEX_UNLOCK_FUNC) extServiceFuncNotImplementedCalled;

CPSS_OS_SIG_SEM_BIN_CREATE_FUNC  cpssOsSigSemBinCreate                                = (CPSS_OS_SIG_SEM_BIN_CREATE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_SIG_SEM_M_CREATE_FUNC    cpssOsSigSemMCreate                                  = (CPSS_OS_SIG_SEM_M_CREATE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_SIG_SEM_C_CREATE_FUNC    cpssOsSigSemCCreate                                  = (CPSS_OS_SIG_SEM_C_CREATE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_SIG_SEM_DELETE_FUNC      cpssOsSigSemDelete                                   = (CPSS_OS_SIG_SEM_DELETE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_SIG_SEM_WAIT_FUNC        cpssOsSigSemWait                                     = (CPSS_OS_SIG_SEM_WAIT_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_SIG_SEM_SIGNAL_FUNC      cpssOsSigSemSignal                                   = (CPSS_OS_SIG_SEM_SIGNAL_FUNC) extServiceFuncNotImplementedCalled;

CPSS_OS_STR_LEN_FUNC     cpssOsStrlen                                                 = (CPSS_OS_STR_LEN_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_STR_CPY_FUNC     cpssOsStrCpy                                                 = (CPSS_OS_STR_CPY_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_STR_N_CPY_FUNC   cpssOsStrNCpy                                                = (CPSS_OS_STR_N_CPY_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_STR_CHR_FUNC     cpssOsStrChr                                                 = (CPSS_OS_STR_CHR_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_STR_REV_CHR_FUNC cpssOsStrRChr                                                = (CPSS_OS_STR_REV_CHR_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_STR_CMP_FUNC     cpssOsStrCmp                                                 = (CPSS_OS_STR_CMP_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_STR_N_CMP_FUNC   cpssOsStrNCmp                                                = (CPSS_OS_STR_N_CMP_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_STR_CAT_FUNC     cpssOsStrCat                                                 = (CPSS_OS_STR_CAT_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_STR_N_CAT_FUNC   cpssOsStrNCat                                                = (CPSS_OS_STR_N_CAT_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_TO_UPPER_FUNC    cpssOsToUpper                                                = (CPSS_OS_TO_UPPER_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_STR_TO_32_FUNC   cpssOsStrTo32                                                = (CPSS_OS_STR_TO_32_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_STR_TO_U32_FUNC  cpssOsStrToU32                                               = (CPSS_OS_STR_TO_U32_FUNC) extServiceFuncNotImplementedCalled;

CPSS_OS_TIME_WK_AFTER_FUNC  cpssOsTimerWkAfter                                        = (CPSS_OS_TIME_WK_AFTER_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_TIME_TICK_GET_FUNC  cpssOsTickGet                                             = (CPSS_OS_TIME_TICK_GET_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_TIME_GET_FUNC       cpssOsTime                                                = (CPSS_OS_TIME_GET_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_TIME_RT_FUNC        cpssOsTimeRT                                              = (CPSS_OS_TIME_RT_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_GET_SYS_CLOCK_RATE_FUNC cpssOsGetSysClockRate                                 = (CPSS_OS_GET_SYS_CLOCK_RATE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_DELAY_FUNC          cpssOsDelay                                               = (CPSS_OS_DELAY_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_STRF_TIME_FUNC      cpssOsStrfTime                                            = (CPSS_OS_STRF_TIME_FUNC) extServiceFuncNotImplementedCalled;

CPSS_OS_TASK_CREATE_FUNC    cpssOsTaskCreate                                          = (CPSS_OS_TASK_CREATE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_TASK_DELETE_FUNC    cpssOsTaskDelete                                          = (CPSS_OS_TASK_DELETE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_TASK_GET_SELF_FUNC  cpssOsTaskGetSelf                                         = (CPSS_OS_TASK_GET_SELF_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_TASK_LOCK_FUNC      cpssOsTaskLock                                            = (CPSS_OS_TASK_LOCK_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_TASK_UNLOCK_FUNC    cpssOsTaskUnLock                                          = (CPSS_OS_TASK_UNLOCK_FUNC) extServiceFuncNotImplementedCalled;

CPSS_OS_STDLIB_QSORT_FUNC   cpssOsQsort                                               = (CPSS_OS_STDLIB_QSORT_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_STDLIB_BSEARCH_FUNC cpssOsBsearch                                             = (CPSS_OS_STDLIB_BSEARCH_FUNC) extServiceFuncNotImplementedCalled;

CPSS_OS_MSGQ_CREATE_FUNC    cpssOsMsgQCreate                                          = (CPSS_OS_MSGQ_CREATE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_MSGQ_DELETE_FUNC    cpssOsMsgQDelete                                          = (CPSS_OS_MSGQ_DELETE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_MSGQ_SEND_FUNC      cpssOsMsgQSend                                            = (CPSS_OS_MSGQ_SEND_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_MSGQ_RECV_FUNC      cpssOsMsgQRecv                                            = (CPSS_OS_MSGQ_RECV_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_MSGQ_NUM_MSGS_FUNC  cpssOsMsgQNumMsgs                                         = (CPSS_OS_MSGQ_NUM_MSGS_FUNC) extServiceFuncNotImplementedCalled;
CPSS_OS_LOG_FUNC            cpssOsLog                                                 =(CPSS_OS_LOG_FUNC) extServiceFuncNotImplementedCalled;  

CPSS_TRACE_HW_ACCESS_WRITE_FUNC  cpssTraceHwAccessWrite                               = (CPSS_TRACE_HW_ACCESS_WRITE_FUNC) extServiceFuncNotImplementedCalled;
CPSS_TRACE_HW_ACCESS_READ_FUNC   cpssTraceHwAccessRead                                = (CPSS_TRACE_HW_ACCESS_READ_FUNC) extServiceFuncNotImplementedCalled;
CPSS_TRACE_HW_ACCESS_DELAY_FUNC   cpssTraceHwAccessDelay                                = (CPSS_TRACE_HW_ACCESS_DELAY_FUNC) extServiceFuncNotImplementedCalled;

  

/*******************************************************************************
* cpssExtServicesBind
*
* DESCRIPTION:
*       bind the cpss with OS , external driver functions.
* INPUTS:
*       extDrvFuncBindInfoPtr - (pointer to) set of call back functions
*       osFuncBindPtr - (pointer to) set of call back functions
*       traceFuncBindPtr - (pointer to) set of call back functions
*
* OUTPUTS:
*       none
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       function must be called by application before phase 1 init
*
*******************************************************************************/
/* can't use here cpssOsMemCpy, because this pointer isn't initialized yet */
static GT_VOID * localMemCpy
(
    IN GT_VOID *      destination,
    IN const GT_VOID* source,
    IN GT_U32         size
)
{
    GT_U8* dst_ptr = (GT_U8 *)destination;
    GT_U8* src_ptr = (GT_U8 *)source;

    while(size--)
    {
        *dst_ptr++ = *src_ptr++;
    }

    return destination;
}

GT_STATUS   cpssExtServicesBind(
    IN CPSS_EXT_DRV_FUNC_BIND_STC   *extDrvFuncBindInfoPtr,
    IN CPSS_OS_FUNC_BIND_STC        *osFuncBindPtr,
    IN CPSS_TRACE_FUNC_BIND_STC     *traceFuncBindPtr
)
{
    if(extDrvFuncBindInfoPtr)
    {
        /* ext drv functions */
        localMemCpy(&prvCpssBindFuncs.extDrv, extDrvFuncBindInfoPtr,
                     sizeof(prvCpssBindFuncs.extDrv));
    }

    if(osFuncBindPtr)
    {
        /* OS functions */
        localMemCpy(&prvCpssBindFuncs.os, osFuncBindPtr,
                     sizeof(prvCpssBindFuncs.os));
    }

    if(traceFuncBindPtr)
    {
        /* HW access trace functions */
        localMemCpy(&prvCpssBindFuncs.trace, traceFuncBindPtr,
                     sizeof(prvCpssBindFuncs.trace));
    }

    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvMgmtCacheFlush      , prvCpssBindFuncs.extDrv.extDrvMgmtCacheBindInfo.extDrvMgmtCacheFlush);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvMgmtCacheInvalidate , prvCpssBindFuncs.extDrv.extDrvMgmtCacheBindInfo.extDrvMgmtCacheInvalidate);

    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvDmaWrite            , prvCpssBindFuncs.extDrv.extDrvDmaBindInfo.extDrvDmaWriteDriverFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvDmaRead             , prvCpssBindFuncs.extDrv.extDrvDmaBindInfo.extDrvDmaReadFunc);

    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvEthPortRxInit       , prvCpssBindFuncs.extDrv.extDrvEthPortBindInfo.extDrvEthPortRxInitFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvEthRawSocketModeSet       , prvCpssBindFuncs.extDrv.extDrvEthPortBindInfo.extDrvEthRawSocketModeSetFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvEthRawSocketModeGet       , prvCpssBindFuncs.extDrv.extDrvEthPortBindInfo.extDrvEthRawSocketModeGetFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvLinuxModeSet       , prvCpssBindFuncs.extDrv.extDrvEthPortBindInfo.extDrvLinuxModeSetFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvLinuxModeGet       , prvCpssBindFuncs.extDrv.extDrvEthPortBindInfo.extDrvLinuxModeGetFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvHsuMemBaseAddrGet         , prvCpssBindFuncs.extDrv.extDrvHsuDrvBindInfo.extDrvHsuMemBaseAddrGetFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvHsuWarmRestart           , prvCpssBindFuncs.extDrv.extDrvHsuDrvBindInfo.extDrvHsuWarmRestartFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvInboundSdmaEnable        , prvCpssBindFuncs.extDrv.extDrvHsuDrvBindInfo.extDrvHsuInboundSdmaEnableFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvInboundSdmaDisable       , prvCpssBindFuncs.extDrv.extDrvHsuDrvBindInfo.extDrvHsuInboundSdmaDisableFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvInboundSdmaStateGet      , prvCpssBindFuncs.extDrv.extDrvHsuDrvBindInfo.extDrvHsuInboundSdmaStateGetFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvEthPortTxInit            , prvCpssBindFuncs.extDrv.extDrvEthPortBindInfo.extDrvEthPortTxInitFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvEthPortEnable       , prvCpssBindFuncs.extDrv.extDrvEthPortBindInfo.extDrvEthPortEnableFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvEthPortDisable      , prvCpssBindFuncs.extDrv.extDrvEthPortBindInfo.extDrvEthPortDisableFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvEthPortTx           , prvCpssBindFuncs.extDrv.extDrvEthPortBindInfo.extDrvEthPortTxFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvEthInputHookAdd     , prvCpssBindFuncs.extDrv.extDrvEthPortBindInfo.extDrvEthPortInputHookAddFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvEthRawSocketRxHookAdd     , prvCpssBindFuncs.extDrv.extDrvEthPortBindInfo.extDrvEthRawSocketRxHookAddFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvEthTxCompleteHookAdd, prvCpssBindFuncs.extDrv.extDrvEthPortBindInfo.extDrvEthPortTxCompleteHookAddFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvEthRxPacketFree     , prvCpssBindFuncs.extDrv.extDrvEthPortBindInfo.extDrvEthPortRxPacketFreeFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvEthPortTxModeSet    , prvCpssBindFuncs.extDrv.extDrvEthPortBindInfo.extDrvEthPortTxModeSetFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvEthCpuCodeToQueue   , prvCpssBindFuncs.extDrv.extDrvEthPortBindInfo.extDrvEthCpuCodeToQueueFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvEthPrePendTwoBytesHeaderSet, prvCpssBindFuncs.extDrv.extDrvEthPortBindInfo.extDrvEthPrePendTwoBytesHeaderSetFunc);

    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvIntConnect          , prvCpssBindFuncs.extDrv.extDrvIntBindInfo.extDrvIntConnectFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvIntEnable           , prvCpssBindFuncs.extDrv.extDrvIntBindInfo.extDrvIntEnableFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvIntDisable          , prvCpssBindFuncs.extDrv.extDrvIntBindInfo.extDrvIntDisableFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvSetIntLockUnlock    , prvCpssBindFuncs.extDrv.extDrvIntBindInfo.extDrvIntLockModeSetFunc);

    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvPciConfigWriteReg   , prvCpssBindFuncs.extDrv.extDrvPciInfo.extDrvPciConfigWriteRegFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvPciConfigReadReg    , prvCpssBindFuncs.extDrv.extDrvPciInfo.extDrvPciConfigReadRegFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvPciFindDev          , prvCpssBindFuncs.extDrv.extDrvPciInfo.extDrvPciDevFindFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvGetPciIntVec        , prvCpssBindFuncs.extDrv.extDrvPciInfo.extDrvPciIntVecFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvGetIntMask          , prvCpssBindFuncs.extDrv.extDrvPciInfo.extDrvPciIntMaskFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvEnableCombinedPciAccess  , prvCpssBindFuncs.extDrv.extDrvPciInfo.extDrvPciCombinedAccessEnableFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvPciDoubleWrite      , prvCpssBindFuncs.extDrv.extDrvPciInfo.extDrvPciDoubleWriteFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvPciDoubleRead       , prvCpssBindFuncs.extDrv.extDrvPciInfo.extDrvPciDoubleReadFunc);

    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvHwIfSmiInitDriver        , prvCpssBindFuncs.extDrv.extDrvHwIfSmiBindInfo.extDrvHwIfSmiInitDriverFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvHwIfSmiWriteReg          , prvCpssBindFuncs.extDrv.extDrvHwIfSmiBindInfo.extDrvHwIfSmiWriteRegFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvHwIfSmiReadReg           , prvCpssBindFuncs.extDrv.extDrvHwIfSmiBindInfo.extDrvHwIfSmiReadRegFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvHwIfSmiTskRegRamRead     , prvCpssBindFuncs.extDrv.extDrvHwIfSmiBindInfo.extDrvHwIfSmiTaskRegRamReadFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvHwIfSmiTskRegRamWrite    , prvCpssBindFuncs.extDrv.extDrvHwIfSmiBindInfo.extDrvHwIfSmiTaskRegRamWriteFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvHwIfSmiTskRegVecRead     , prvCpssBindFuncs.extDrv.extDrvHwIfSmiBindInfo.extDrvHwIfSmiTaskRegVecReadFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvHwIfSmiTskRegVecWrite    , prvCpssBindFuncs.extDrv.extDrvHwIfSmiBindInfo.extDrvHwIfSmiTaskRegVecWriteFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvHwIfSmiTaskWriteReg      , prvCpssBindFuncs.extDrv.extDrvHwIfSmiBindInfo.extDrvHwIfSmiTaskWriteRegFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvHwIfSmiTaskReadReg       , prvCpssBindFuncs.extDrv.extDrvHwIfSmiBindInfo.extDrvHwIfSmiTaskReadRegFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvHwIfSmiInterruptWriteReg , prvCpssBindFuncs.extDrv.extDrvHwIfSmiBindInfo.extDrvHwIfSmiIntWriteRegFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvHwIfSmiInterruptReadReg  , prvCpssBindFuncs.extDrv.extDrvHwIfSmiBindInfo.extDrvHwIfSmiIntReadRegFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvSmiDevVendorIdGet        , prvCpssBindFuncs.extDrv.extDrvHwIfSmiBindInfo.extDrvHwIfSmiDevVendorIdGetFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvHwIfSmiTaskWriteRegField , prvCpssBindFuncs.extDrv.extDrvHwIfSmiBindInfo.extDrvHwIfSmiTaskWriteFieldFunc);

    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvHwIfTwsiInitDriver , prvCpssBindFuncs.extDrv.extDrvHwIfTwsiBindInfo.extDrvHwIfTwsiInitDriverFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvHwIfTwsiWriteReg   , prvCpssBindFuncs.extDrv.extDrvHwIfTwsiBindInfo.extDrvHwIfTwsiWriteRegFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvHwIfTwsiReadReg    , prvCpssBindFuncs.extDrv.extDrvHwIfTwsiBindInfo.extDrvHwIfTwsiReadRegFunc);

    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvI2cMgmtMasterInit    , prvCpssBindFuncs.extDrv.extDrvMgmtHwIfBindInfo.extDrvI2cMgmtMasterInitFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvMgmtReadRegister     , prvCpssBindFuncs.extDrv.extDrvMgmtHwIfBindInfo.extDrvMgmtReadRegisterFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvMgmtWriteRegister    , prvCpssBindFuncs.extDrv.extDrvMgmtHwIfBindInfo.extDrvMgmtWriteRegisterFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvMgmtIsrReadRegister  , prvCpssBindFuncs.extDrv.extDrvMgmtHwIfBindInfo.extDrvMgmtIsrReadRegisterFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvMgmtIsrWriteRegister , prvCpssBindFuncs.extDrv.extDrvMgmtHwIfBindInfo.extDrvMgmtIsrWriteRegisterFunc);

    REPLACE_IF_NOT_NULL_MAC(cpssExtDrvDragoniteShMemBaseAddrGet , prvCpssBindFuncs.extDrv.extDrvDragoniteInfo.extDrvDragoniteShMemBaseAddrGetFunc);

    REPLACE_IF_NOT_NULL_MAC(cpssOsNtohl       , prvCpssBindFuncs.os.osInetBindInfo.osInetNtohlFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsHtonl       , prvCpssBindFuncs.os.osInetBindInfo.osInetHtonlFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsNtohs       , prvCpssBindFuncs.os.osInetBindInfo.osInetNtohsFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsHtons       , prvCpssBindFuncs.os.osInetBindInfo.osInetHtonsFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsInetNtoa    , prvCpssBindFuncs.os.osInetBindInfo.osInetNtoaFunc);

#if !defined(_linux) && !defined(_FreeBSD)
    REPLACE_IF_NOT_NULL_MAC(cpssOsIntEnable        , prvCpssBindFuncs.os.osIntBindInfo.osIntEnableFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsIntDisable       , prvCpssBindFuncs.os.osIntBindInfo.osIntDisableFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsSetIntLockUnlock , prvCpssBindFuncs.os.osIntBindInfo.osIntModeSetFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsInterruptConnect , prvCpssBindFuncs.os.osIntBindInfo.osIntConnectFunc);
#endif

    REPLACE_IF_NOT_NULL_MAC(cpssOsBindStdOut , prvCpssBindFuncs.os.osIoBindInfo.osIoBindStdOutFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsPrintf     , prvCpssBindFuncs.os.osIoBindInfo.osIoPrintfFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsVprintf    , prvCpssBindFuncs.os.osIoBindInfo.osIoVprintfFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsSprintf    , prvCpssBindFuncs.os.osIoBindInfo.osIoSprintfFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsVsprintf   , prvCpssBindFuncs.os.osIoBindInfo.osIoVsprintfFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsPrintSync  , prvCpssBindFuncs.os.osIoBindInfo.osIoPrintSynchFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsGets       , prvCpssBindFuncs.os.osIoBindInfo.osIoGetsFunc);

    REPLACE_IF_NOT_NULL_MAC(cpssOsFopen         , prvCpssBindFuncs.os.osIoBindInfo.osIoFopenFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsFclose        , prvCpssBindFuncs.os.osIoBindInfo.osIoFcloseFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsRewind        , prvCpssBindFuncs.os.osIoBindInfo.osIoRewindFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsFprintf       , prvCpssBindFuncs.os.osIoBindInfo.osIoFprintfFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsFgets         , prvCpssBindFuncs.os.osIoBindInfo.osIoFgets);

    REPLACE_IF_NOT_NULL_MAC(cpssOsBzero         , prvCpssBindFuncs.os.osMemBindInfo.osMemBzeroFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsMemSet        , prvCpssBindFuncs.os.osMemBindInfo.osMemSetFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsMemCpy        , prvCpssBindFuncs.os.osMemBindInfo.osMemCpyFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsMemCmp        , prvCpssBindFuncs.os.osMemBindInfo.osMemCmpFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsStaticMalloc  , prvCpssBindFuncs.os.osMemBindInfo.osMemStaticMallocFunc);

#ifdef OS_MALLOC_MEMORY_LEAKAGE_DBG
    REPLACE_IF_NOT_NULL_MAC(cpssOsMalloc_MemoryLeakageDbg        , prvCpssBindFuncs.os.osMemBindInfo.osMemMallocFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsRealloc_MemoryLeakageDbg       , prvCpssBindFuncs.os.osMemBindInfo.osMemReallocFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsFree_MemoryLeakageDbg          , prvCpssBindFuncs.os.osMemBindInfo.osMemFreeFunc);
#else  /*!OS_MALLOC_MEMORY_LEAKAGE_DBG*/
    REPLACE_IF_NOT_NULL_MAC(cpssOsMalloc        , prvCpssBindFuncs.os.osMemBindInfo.osMemMallocFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsRealloc       , prvCpssBindFuncs.os.osMemBindInfo.osMemReallocFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsFree          , prvCpssBindFuncs.os.osMemBindInfo.osMemFreeFunc);
#endif /*!OS_MALLOC_MEMORY_LEAKAGE_DBG*/
    REPLACE_IF_NOT_NULL_MAC(cpssOsCacheDmaMalloc, prvCpssBindFuncs.os.osMemBindInfo.osMemCacheDmaMallocFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsCacheDmaFree  , prvCpssBindFuncs.os.osMemBindInfo.osMemCacheDmaFreeFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsPhy2Virt      , prvCpssBindFuncs.os.osMemBindInfo.osMemPhyToVirtFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsVirt2Phy      , prvCpssBindFuncs.os.osMemBindInfo.osMemVirtToPhyFunc);

    REPLACE_IF_NOT_NULL_MAC(cpssOsRand    , prvCpssBindFuncs.os.osRandBindInfo.osRandFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsSrand   , prvCpssBindFuncs.os.osRandBindInfo.osSrandFunc);

    REPLACE_IF_NOT_NULL_MAC(cpssOsMutexCreate     , prvCpssBindFuncs.os.osSemBindInfo.osMutexCreateFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsMutexDelete     , prvCpssBindFuncs.os.osSemBindInfo.osMutexDeleteFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsMutexLock       , prvCpssBindFuncs.os.osSemBindInfo.osMutexLockFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsMutexUnlock     , prvCpssBindFuncs.os.osSemBindInfo.osMutexUnlockFunc);

    REPLACE_IF_NOT_NULL_MAC(cpssOsSigSemBinCreate , prvCpssBindFuncs.os.osSemBindInfo.osSigSemBinCreateFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsSigSemMCreate   , prvCpssBindFuncs.os.osSemBindInfo.osSigSemMCreateFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsSigSemCCreate   , prvCpssBindFuncs.os.osSemBindInfo.osSigSemCCreateFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsSigSemDelete    , prvCpssBindFuncs.os.osSemBindInfo.osSigSemDeleteFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsSigSemWait      , prvCpssBindFuncs.os.osSemBindInfo.osSigSemWaitFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsSigSemSignal    , prvCpssBindFuncs.os.osSemBindInfo.osSigSemSignalFunc);

    REPLACE_IF_NOT_NULL_MAC(cpssOsStrlen   , prvCpssBindFuncs.os.osStrBindInfo.osStrlenFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsStrCpy   , prvCpssBindFuncs.os.osStrBindInfo.osStrCpyFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsStrNCpy  , prvCpssBindFuncs.os.osStrBindInfo.osStrNCpyFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsStrChr   , prvCpssBindFuncs.os.osStrBindInfo.osStrChrFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsStrRChr  , prvCpssBindFuncs.os.osStrBindInfo.osStrRevChrFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsStrCmp   , prvCpssBindFuncs.os.osStrBindInfo.osStrCmpFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsStrNCmp  , prvCpssBindFuncs.os.osStrBindInfo.osStrNCmpFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsStrCat   , prvCpssBindFuncs.os.osStrBindInfo.osStrCatFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsStrNCat  , prvCpssBindFuncs.os.osStrBindInfo.osStrStrNCatFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsToUpper  , prvCpssBindFuncs.os.osStrBindInfo.osStrChrToUpperFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsStrTo32  , prvCpssBindFuncs.os.osStrBindInfo.osStrTo32Func);
    REPLACE_IF_NOT_NULL_MAC(cpssOsStrToU32 , prvCpssBindFuncs.os.osStrBindInfo.osStrToU32Func);

    REPLACE_IF_NOT_NULL_MAC(cpssOsTimerWkAfter, prvCpssBindFuncs.os.osTimeBindInfo.osTimeWkAfterFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsTickGet     , prvCpssBindFuncs.os.osTimeBindInfo.osTimeTickGetFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsTime        , prvCpssBindFuncs.os.osTimeBindInfo.osTimeGetFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsTimeRT      , prvCpssBindFuncs.os.osTimeBindInfo.osTimeRTFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsGetSysClockRate , prvCpssBindFuncs.os.osTimeBindInfo.osGetSysClockRateFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsDelay       , prvCpssBindFuncs.os.osTimeBindInfo.osDelayFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsStrfTime    , prvCpssBindFuncs.os.osTimeBindInfo.osStrftimeFunc);

    REPLACE_IF_NOT_NULL_MAC(cpssOsTaskCreate  , prvCpssBindFuncs.os.osTaskBindInfo.osTaskCreateFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsTaskDelete  , prvCpssBindFuncs.os.osTaskBindInfo.osTaskDeleteFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsTaskGetSelf , prvCpssBindFuncs.os.osTaskBindInfo.osTaskGetSelfFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsTaskLock    , prvCpssBindFuncs.os.osTaskBindInfo.osTaskLockFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsTaskUnLock  , prvCpssBindFuncs.os.osTaskBindInfo.osTaskUnLockFunc);

    REPLACE_IF_NOT_NULL_MAC(cpssOsQsort       , prvCpssBindFuncs.os.osStdLibBindInfo.osQsortFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsBsearch     , prvCpssBindFuncs.os.osStdLibBindInfo.osBsearchFunc);

    REPLACE_IF_NOT_NULL_MAC(cpssOsMsgQCreate  , prvCpssBindFuncs.os.osMsgQBindInfo.osMsgQCreateFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsMsgQDelete  , prvCpssBindFuncs.os.osMsgQBindInfo.osMsgQDeleteFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsMsgQSend    , prvCpssBindFuncs.os.osMsgQBindInfo.osMsgQSendFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsMsgQRecv    , prvCpssBindFuncs.os.osMsgQBindInfo.osMsgQRecvFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssOsMsgQNumMsgs , prvCpssBindFuncs.os.osMsgQBindInfo.osMsgQNumMsgsFunc);

    REPLACE_IF_NOT_NULL_MAC(cpssOsLog         , prvCpssBindFuncs.os.osLogBindInfo.osLogFunc);

    REPLACE_IF_NOT_NULL_MAC(cpssTraceHwAccessWrite  , prvCpssBindFuncs.trace.traceHwBindInfo.traceHwAccessWriteFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssTraceHwAccessRead   , prvCpssBindFuncs.trace.traceHwBindInfo.traceHwAccessReadFunc);
    REPLACE_IF_NOT_NULL_MAC(cpssTraceHwAccessDelay   , prvCpssBindFuncs.trace.traceHwBindInfo.traceHwAccessDelayFunc);

#if defined(CPSS_LOG_ENABLE)
    prvCpssLogInit();
#endif
    return GT_OK;
}

