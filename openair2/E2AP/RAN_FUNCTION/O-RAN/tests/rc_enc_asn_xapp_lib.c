/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 *
 * Canonical O-RAN E2SM-RC RRM Policy Ratio Format-1 message builders.
 * Wire layout matches O-RAN.WG3.E2SM-RC (S-NSSAI 1+3 octet, integer PRB %).
 */

#include "rc_enc_asn_xapp_lib.h"

#include "openair2/E2AP/flexric/src/sm/rc_sm/ie/ir/ran_param_struct.h"
#include "openair2/E2AP/flexric/src/sm/rc_sm/ie/ir/ran_param_list.h"

#include <assert.h>
#include <stdlib.h>

byte_array_t get_sd_big_endian_ba(uint32_t val)
{
    byte_array_t ba = {.len = 3};
    ba.buf = calloc(3, sizeof(uint8_t));
    assert(ba.buf != NULL && "Memory exhausted");

    for (size_t i = 0; i < ba.len; i++) {
        int shift = (int)((3 - i - 1) * 8);
        ba.buf[i] = (uint8_t)((val >> shift) & 0xFF);
    }

    return ba;
}

seq_ran_param_t get_sst_seq_ran_param(uint8_t sst_input)
{
    seq_ran_param_t sst_seq_rp = {};
    sst_seq_rp.ran_param_id = SST;
    sst_seq_rp.ran_param_val.type = ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE;

    uint8_t *sst_buf = calloc(1, sizeof(uint8_t));
    assert(sst_buf != NULL && "Memory exhausted");
    sst_buf[0] = sst_input;

    sst_seq_rp.ran_param_val.flag_false = calloc(1, sizeof(ran_parameter_value_t));
    assert(sst_seq_rp.ran_param_val.flag_false != NULL && "Memory exhausted");
    sst_seq_rp.ran_param_val.flag_false->octet_str_ran.len = 1;
    sst_seq_rp.ran_param_val.flag_false->octet_str_ran.buf = sst_buf;

    return sst_seq_rp;
}

seq_ran_param_t get_sd_seq_ran_param(uint32_t sd_input)
{
    seq_ran_param_t sd_seq_rp = {};
    sd_seq_rp.ran_param_id = SD;
    sd_seq_rp.ran_param_val.type = ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE;

    sd_seq_rp.ran_param_val.flag_false = calloc(1, sizeof(ran_parameter_value_t));
    assert(sd_seq_rp.ran_param_val.flag_false != NULL && "Memory exhausted");
    sd_seq_rp.ran_param_val.flag_false->octet_str_ran = get_sd_big_endian_ba(sd_input);

    return sd_seq_rp;
}

seq_ran_param_t get_plmn_identity_seq_ran_param(void)
{
    /* 3GPP TS 38.413 PLMN Identity: 3 octets. Default test PLMN 001/01. */
    seq_ran_param_t plmn_seq_rp = {};
    plmn_seq_rp.ran_param_id = PLMN_IDENTITY;
    plmn_seq_rp.ran_param_val.type = ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE;

    uint8_t *plmn_buf = calloc(3, sizeof(uint8_t));
    assert(plmn_buf != NULL && "Memory exhausted");
    plmn_buf[0] = 0x00;
    plmn_buf[1] = 0xf1;
    plmn_buf[2] = 0x10;

    plmn_seq_rp.ran_param_val.flag_false = calloc(1, sizeof(ran_parameter_value_t));
    assert(plmn_seq_rp.ran_param_val.flag_false != NULL && "Memory exhausted");
    plmn_seq_rp.ran_param_val.flag_false->octet_str_ran.len = 3;
    plmn_seq_rp.ran_param_val.flag_false->octet_str_ran.buf = plmn_buf;

    return plmn_seq_rp;
}

seq_ran_param_t get_min_prb_ratio_seq_ran_param(int64_t min_prb_input)
{
    seq_ran_param_t min_prb_ratio_seq_rp = {};
    min_prb_ratio_seq_rp.ran_param_id = MIN_PRB_POLICY_RATIO;
    min_prb_ratio_seq_rp.ran_param_val.type = ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE;

    min_prb_ratio_seq_rp.ran_param_val.flag_false = calloc(1, sizeof(ran_parameter_value_t));
    assert(min_prb_ratio_seq_rp.ran_param_val.flag_false != NULL && "Memory exhausted");
    min_prb_ratio_seq_rp.ran_param_val.flag_false->int_ran = min_prb_input;

    return min_prb_ratio_seq_rp;
}

seq_ran_param_t get_max_prb_ratio_seq_ran_param(int64_t max_prb_input)
{
    seq_ran_param_t max_prb_ratio_seq_rp = {};
    max_prb_ratio_seq_rp.ran_param_id = MAX_PRB_POLICY_RATIO;
    max_prb_ratio_seq_rp.ran_param_val.type = ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE;

    max_prb_ratio_seq_rp.ran_param_val.flag_false = calloc(1, sizeof(ran_parameter_value_t));
    assert(max_prb_ratio_seq_rp.ran_param_val.flag_false != NULL && "Memory exhausted");
    max_prb_ratio_seq_rp.ran_param_val.flag_false->int_ran = max_prb_input;

    return max_prb_ratio_seq_rp;
}

seq_ran_param_t get_dedicated_prb_ratio_seq_ran_param(int64_t dedicated_prb_input)
{
    seq_ran_param_t dedicated_prb_ratio_seq_rp = {};
    dedicated_prb_ratio_seq_rp.ran_param_id = DEDICATED_PRB_POLICY_RATIO;
    dedicated_prb_ratio_seq_rp.ran_param_val.type = ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE;

    dedicated_prb_ratio_seq_rp.ran_param_val.flag_false = calloc(1, sizeof(ran_parameter_value_t));
    assert(dedicated_prb_ratio_seq_rp.ran_param_val.flag_false != NULL && "Memory exhausted");
    dedicated_prb_ratio_seq_rp.ran_param_val.flag_false->int_ran = dedicated_prb_input;

    return dedicated_prb_ratio_seq_rp;
}

seq_ran_param_t get_snssai_seq_ran_param(seq_ran_param_t sst_seq_rp, seq_ran_param_t sd_seq_rp)
{
    seq_ran_param_t snssai_seq_rp = {};
    snssai_seq_rp.ran_param_id = S_NSSAI;
    snssai_seq_rp.ran_param_val.type = STRUCTURE_RAN_PARAMETER_VAL_TYPE;

    snssai_seq_rp.ran_param_val.strct = calloc(1, sizeof(ran_param_struct_t));
    assert(snssai_seq_rp.ran_param_val.strct != NULL && "Memory exhausted");
    snssai_seq_rp.ran_param_val.strct->sz_ran_param_struct = 2;
    snssai_seq_rp.ran_param_val.strct->ran_param_struct = calloc(2, sizeof(seq_ran_param_t));
    assert(snssai_seq_rp.ran_param_val.strct->ran_param_struct != NULL && "Memory exhausted");
    snssai_seq_rp.ran_param_val.strct->ran_param_struct[0] = sst_seq_rp;
    snssai_seq_rp.ran_param_val.strct->ran_param_struct[1] = sd_seq_rp;

    return snssai_seq_rp;
}

seq_ran_param_t get_rrm_policy_member_seq_ran_param(seq_ran_param_t snssai_seq_ran_param,
                                                     seq_ran_param_t plmn_identity_seq_ran_param)
{
    seq_ran_param_t rrm_policy_member_seq_rp = {};
    rrm_policy_member_seq_rp.ran_param_id = RRM_POLICY_MEMBER;
    rrm_policy_member_seq_rp.ran_param_val.type = STRUCTURE_RAN_PARAMETER_VAL_TYPE;

    rrm_policy_member_seq_rp.ran_param_val.strct = calloc(1, sizeof(ran_param_struct_t));
    assert(rrm_policy_member_seq_rp.ran_param_val.strct != NULL && "Memory exhausted");
    rrm_policy_member_seq_rp.ran_param_val.strct->sz_ran_param_struct = 2;
    rrm_policy_member_seq_rp.ran_param_val.strct->ran_param_struct = calloc(2, sizeof(seq_ran_param_t));
    assert(rrm_policy_member_seq_rp.ran_param_val.strct->ran_param_struct != NULL && "Memory exhausted");
    /* O-RAN E2SM-RC: PLMN Identity then S-NSSAI under RRM Policy Member. */
    rrm_policy_member_seq_rp.ran_param_val.strct->ran_param_struct[0] = plmn_identity_seq_ran_param;
    rrm_policy_member_seq_rp.ran_param_val.strct->ran_param_struct[1] = snssai_seq_ran_param;

    return rrm_policy_member_seq_rp;
}

seq_ran_param_t get_rrm_policy_member_list_seq_ran_param(seq_ran_param_t rrm_policy_member)
{
    seq_ran_param_t rrm_policy_member_list_seq_rp = {};
    rrm_policy_member_list_seq_rp.ran_param_id = RRM_POLICY_MEMBER_LIST;
    rrm_policy_member_list_seq_rp.ran_param_val.type = LIST_RAN_PARAMETER_VAL_TYPE;
    rrm_policy_member_list_seq_rp.ran_param_val.lst = calloc(1, sizeof(ran_param_list_t));
    assert(rrm_policy_member_list_seq_rp.ran_param_val.lst != NULL && "Memory exhausted");
    rrm_policy_member_list_seq_rp.ran_param_val.lst->sz_lst_ran_param = 1;
    rrm_policy_member_list_seq_rp.ran_param_val.lst->lst_ran_param = calloc(1, sizeof(lst_ran_param_t));
    assert(rrm_policy_member_list_seq_rp.ran_param_val.lst->lst_ran_param != NULL && "Memory exhausted");
    rrm_policy_member_list_seq_rp.ran_param_val.lst->lst_ran_param[0].ran_param_struct.sz_ran_param_struct = 1;
    rrm_policy_member_list_seq_rp.ran_param_val.lst->lst_ran_param[0].ran_param_struct.ran_param_struct =
        calloc(1, sizeof(seq_ran_param_t));
    assert(rrm_policy_member_list_seq_rp.ran_param_val.lst->lst_ran_param[0].ran_param_struct.ran_param_struct != NULL
           && "Memory exhausted");
    rrm_policy_member_list_seq_rp.ran_param_val.lst->lst_ran_param[0].ran_param_struct.ran_param_struct[0] =
        rrm_policy_member;

    return rrm_policy_member_list_seq_rp;
}

seq_ran_param_t get_rrm_policy_seq_ran_param(seq_ran_param_t rrm_policy_member_list)
{
    seq_ran_param_t rrm_policy_seq_rp = {};
    rrm_policy_seq_rp.ran_param_id = RRM_POLICY;
    rrm_policy_seq_rp.ran_param_val.type = STRUCTURE_RAN_PARAMETER_VAL_TYPE;

    rrm_policy_seq_rp.ran_param_val.strct = calloc(1, sizeof(ran_param_struct_t));
    assert(rrm_policy_seq_rp.ran_param_val.strct != NULL && "Memory exhausted");
    rrm_policy_seq_rp.ran_param_val.strct->sz_ran_param_struct = 1;
    rrm_policy_seq_rp.ran_param_val.strct->ran_param_struct = calloc(1, sizeof(seq_ran_param_t));
    assert(rrm_policy_seq_rp.ran_param_val.strct->ran_param_struct != NULL && "Memory exhausted");
    rrm_policy_seq_rp.ran_param_val.strct->ran_param_struct[0] = rrm_policy_member_list;
    return rrm_policy_seq_rp;
}

seq_ran_param_t get_rrm_policy_ratio_group_seq_ran_param(seq_ran_param_t rrm_policy_seq_rp,
                                                         seq_ran_param_t min_prb_ratio_seq_rp,
                                                         seq_ran_param_t max_prb_ratio_seq_rp,
                                                         seq_ran_param_t dedicated_prb_ratio_seq_rp)
{
    seq_ran_param_t rrm_policy_ratio_group_seq_rp = {};
    rrm_policy_ratio_group_seq_rp.ran_param_id = RRM_POLICY_RATIO_GROUP;
    rrm_policy_ratio_group_seq_rp.ran_param_val.type = STRUCTURE_RAN_PARAMETER_VAL_TYPE;

    rrm_policy_ratio_group_seq_rp.ran_param_val.strct = calloc(1, sizeof(ran_param_struct_t));
    assert(rrm_policy_ratio_group_seq_rp.ran_param_val.strct != NULL && "Memory exhausted");
    rrm_policy_ratio_group_seq_rp.ran_param_val.strct->sz_ran_param_struct = 4;
    rrm_policy_ratio_group_seq_rp.ran_param_val.strct->ran_param_struct = calloc(4, sizeof(seq_ran_param_t));
    assert(rrm_policy_ratio_group_seq_rp.ran_param_val.strct->ran_param_struct != NULL && "Memory exhausted");
    rrm_policy_ratio_group_seq_rp.ran_param_val.strct->ran_param_struct[0] = rrm_policy_seq_rp;
    rrm_policy_ratio_group_seq_rp.ran_param_val.strct->ran_param_struct[1] = min_prb_ratio_seq_rp;
    rrm_policy_ratio_group_seq_rp.ran_param_val.strct->ran_param_struct[2] = max_prb_ratio_seq_rp;
    rrm_policy_ratio_group_seq_rp.ran_param_val.strct->ran_param_struct[3] = dedicated_prb_ratio_seq_rp;
    return rrm_policy_ratio_group_seq_rp;
}

seq_ran_param_t *get_rrm_policy_ratio_list_seq_ran_param(seq_ran_param_t rrm_policy_ratio_group_seq_rp)
{
    seq_ran_param_t *rrm_policy_ratio_list_seq_rp = calloc(1, sizeof(seq_ran_param_t));
    assert(rrm_policy_ratio_list_seq_rp != NULL && "Memory exhausted");
    rrm_policy_ratio_list_seq_rp->ran_param_id = RRM_POLICY_RATIO_LIST;
    rrm_policy_ratio_list_seq_rp->ran_param_val.type = LIST_RAN_PARAMETER_VAL_TYPE;

    rrm_policy_ratio_list_seq_rp->ran_param_val.lst = calloc(1, sizeof(ran_param_list_t));
    assert(rrm_policy_ratio_list_seq_rp->ran_param_val.lst != NULL && "Memory exhausted");
    rrm_policy_ratio_list_seq_rp->ran_param_val.lst->sz_lst_ran_param = 1;
    rrm_policy_ratio_list_seq_rp->ran_param_val.lst->lst_ran_param = calloc(1, sizeof(lst_ran_param_t));
    assert(rrm_policy_ratio_list_seq_rp->ran_param_val.lst->lst_ran_param != NULL && "Memory exhausted");
    rrm_policy_ratio_list_seq_rp->ran_param_val.lst->lst_ran_param->ran_param_struct.sz_ran_param_struct = 1;
    rrm_policy_ratio_list_seq_rp->ran_param_val.lst->lst_ran_param->ran_param_struct.ran_param_struct =
        calloc(1, sizeof(seq_ran_param_t));
    assert(rrm_policy_ratio_list_seq_rp->ran_param_val.lst->lst_ran_param->ran_param_struct.ran_param_struct != NULL
           && "Memory exhausted");
    rrm_policy_ratio_list_seq_rp->ran_param_val.lst->lst_ran_param->ran_param_struct.ran_param_struct[0] =
        rrm_policy_ratio_group_seq_rp;
    return rrm_policy_ratio_list_seq_rp;
}

e2sm_rc_ctrl_msg_frmt_1_t get_ctrl_msg_frmt_1(int64_t dedicated_prb_input,
                                               int64_t minimum_prb_input,
                                               int64_t maximum_prb_input,
                                               uint8_t sst_input,
                                               uint32_t sd_input)
{
    seq_ran_param_t sst_seq_rp = get_sst_seq_ran_param(sst_input);
    seq_ran_param_t sd_seq_rp = get_sd_seq_ran_param(sd_input);
    seq_ran_param_t snssai_seq_rp = get_snssai_seq_ran_param(sst_seq_rp, sd_seq_rp);

    seq_ran_param_t min_prb_ratio_seq_rp = get_min_prb_ratio_seq_ran_param(minimum_prb_input);
    seq_ran_param_t max_prb_ratio_seq_rp = get_max_prb_ratio_seq_ran_param(maximum_prb_input);
    seq_ran_param_t dedicated_prb_ratio_seq_rp = get_dedicated_prb_ratio_seq_ran_param(dedicated_prb_input);

    seq_ran_param_t plmn_identity_seq_rp = get_plmn_identity_seq_ran_param();
    seq_ran_param_t rrm_policy_member_seq_rp =
        get_rrm_policy_member_seq_ran_param(snssai_seq_rp, plmn_identity_seq_rp);
    seq_ran_param_t rrm_policy_member_list_seq_rp =
        get_rrm_policy_member_list_seq_ran_param(rrm_policy_member_seq_rp);
    seq_ran_param_t rrm_policy_seq_rp = get_rrm_policy_seq_ran_param(rrm_policy_member_list_seq_rp);

    seq_ran_param_t rrm_policy_ratio_group_seq_rp = get_rrm_policy_ratio_group_seq_ran_param(
        rrm_policy_seq_rp, min_prb_ratio_seq_rp, max_prb_ratio_seq_rp, dedicated_prb_ratio_seq_rp);

    seq_ran_param_t *rrm_policy_ratio_list_seq_rp =
        get_rrm_policy_ratio_list_seq_ran_param(rrm_policy_ratio_group_seq_rp);

    e2sm_rc_ctrl_msg_frmt_1_t msg = {0};
    msg.sz_ran_param = 1;
    msg.ran_param = rrm_policy_ratio_list_seq_rp;
    return msg;
}
