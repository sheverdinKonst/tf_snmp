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
 * @brief TM Blocks Error Reading interface.
 *
 * @file tm_errors.h
 *
 * $Revision: 2.0 $
 */

#ifndef TM_ERRORS_H
#define TM_ERRORS_H

#include "tm_defs.h"

/** Get QMR errors
 * @param[in]		hndl	TM lib handle
 * @param[out] 	    info    QMR error information
 *
 * @return an integer return code.
 * @retval zero on success. 
 * @retval -EINVAL if hndl is NULL.
 * @retval -EBADF if hndl is invalid.
 * @retval TM_HW_QMR_GET_ERRORS_FAILED if reading from HW fails
*/
int tm_qmr_get_errors(tm_handle hndl, struct tm_error_info *info);


/** Get BAP errors
 * @param[in]		hndl	TM lib handle
 * @param[out] 	    info    BAP error information (4 sets)
 *
 * @return an integer return code.
 * @retval zero on success. 
 * @retval -EINVAL if hndl is NULL.
 * @retval -EBADF if hndl is invalid.
 * @retval TM_HW_BAP_GET_ERRORS_FAILED if reading from HW fails
*/
int tm_bap_get_errors(tm_handle hndl, struct tm_error_info *info);


/** Get RCB errors
 * @param[in]		hndl	TM lib handle
 * @param[out] 	    info    RCB error information 
 *
 * @return an integer return code.
 * @retval zero on success. 
 * @retval -EINVAL if hndl is NULL.
 * @retval -EBADF if hndl is invalid.
 * @retval TM_HW_BAP_GET_ERRORS_FAILED if reading from HW fails
*/
int tm_rcb_get_errors(tm_handle hndl, struct tm_error_info *info);


/** Get Scheduler errors
 * @param[in]		hndl	TM lib handle
 * @param[out] 	    info    Scheduler error information 
 *
 * @return an integer return code.
 * @retval zero on success. 
 * @retval -EINVAL if hndl is NULL.
 * @retval -EBADF if hndl is invalid.
 * @retval TM_HW_SCHED_GET_ERRORS_FAILED if reading from HW fails
*/
int tm_sched_get_errors(tm_handle hndl, struct tm_error_info *info);


/** Get Drop Unit errors
 * @param[in]		hndl	TM lib handle
 * @param[out] 	    info    Drop Unit error information 
 *
 * @return an integer return code.
 * @retval zero on success. 
 * @retval -EINVAL if hndl is NULL.
 * @retval -EBADF if hndl is invalid.
 * @retval TM_HW_DROP_GET_ERRORS_FAILED if reading from HW fails
*/
int tm_drop_get_errors(tm_handle hndl, struct tm_error_info *info);


#endif   /* TM_ERRORS_H */

