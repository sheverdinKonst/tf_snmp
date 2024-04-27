/*
 * (c), Copyright 2009-2014, Marvell International Ltd.  (Marvell)
 *
 * This code contains confidential information of Marvell.
 * No rights are granted herein under any patent, mask work right or copyright
 * of Marvell or any third party. Marvell reserves the right at its sole
 * discretion to request that this code be immediately returned to Marvell.
 * This code is provided "as is". Marvell makes no warranties, expressed,
 * implied or otherwise, regarding its accuracy, completeness or performance.
 */
/**
 * @brief APIs for reading out the TM blocks error related information
 *
 * @file tm_errors.c
 *
 * $Revision: 2.0 $
 */

#include "tm_errors.h"

#include <errno.h>
#include "tm_errcodes.h"
#include "set_hw_registers.h"


/**
 */
int tm_qmr_get_errors(tm_handle hndl, struct tm_error_info *info)
{
    int rc;
    TM_CTL(ctl, hndl);
    rc = get_hw_qmr_errors(ctl, info);
    if (rc)
	{
        rc = TM_HW_QMR_GET_ERRORS_FAILED;
    }
    return rc;
}


/**
 */
int tm_bap_get_errors(tm_handle hndl, struct tm_error_info *info)
{
    int rc;
    TM_CTL(ctl, hndl);
    rc = get_hw_bap_errors(ctl, info);
    if (rc)
	{
        rc = TM_HW_BAP_GET_ERRORS_FAILED;
    }
    return rc;
}


/**
 */
int tm_rcb_get_errors(tm_handle hndl, struct tm_error_info *info)
{
    int rc;
    TM_CTL(ctl, hndl);
    rc = get_hw_rcb_errors(ctl, info);
    if (rc)
	{
        rc = TM_HW_RCB_GET_ERRORS_FAILED;
    }
    return rc;
}


/**
 */
int tm_sched_get_errors(tm_handle hndl, struct tm_error_info *info)
{
    int rc;
    TM_CTL(ctl, hndl);
    rc = get_hw_sched_errors(ctl, info);
    if (rc)
	{
        rc = TM_HW_SCHED_GET_ERRORS_FAILED;
    }
    return rc;
}


/**
 */
int tm_drop_get_errors(tm_handle hndl, struct tm_error_info *info)
{
    int rc;
    TM_CTL(ctl, hndl);

    rc = get_hw_drop_errors(ctl, info);
    if (rc)
	{
        rc = TM_HW_DROP_GET_ERRORS_FAILED;
    }
    return rc;
}

