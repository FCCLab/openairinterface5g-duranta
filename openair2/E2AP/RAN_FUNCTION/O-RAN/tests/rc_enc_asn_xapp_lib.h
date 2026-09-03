/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 *
 * Canonical O-RAN E2SM-RC RRM Policy Ratio control-message builders
 * (S-NSSAI SST/SD + Min/Max/Dedicated PRB ratios). Used by ran_func_rc_test
 * and as the reference encoder for OAI NS / PRB control via E2SM-RC.
 */

#ifndef RC_ENC_ASN_XAPP_LIB_H
#define RC_ENC_ASN_XAPP_LIB_H

#include "openair2/E2AP/flexric/src/sm/rc_sm/ie/rc_data_ie.h"

typedef enum rc_ctrl_msg_ran_param_id_e {
    RRM_POLICY_RATIO_LIST = 1,
    RRM_POLICY_RATIO_GROUP = 2,
    RRM_POLICY = 3,
    RRM_POLICY_MEMBER_LIST = 5,
    RRM_POLICY_MEMBER = 6,
    PLMN_IDENTITY = 7,
    S_NSSAI = 8,
    SST = 9,
    SD = 10,
    MIN_PRB_POLICY_RATIO = 11,
    MAX_PRB_POLICY_RATIO = 12,
    DEDICATED_PRB_POLICY_RATIO = 13
} rc_ctrl_msg_ran_param_id_t;

e2sm_rc_ctrl_msg_frmt_1_t get_ctrl_msg_frmt_1(
    int64_t dedicated_prb_input,
    int64_t minimum_prb_input,
    int64_t maximum_prb_input,
    uint8_t sst_input,
    uint32_t sd_input
);

#endif
