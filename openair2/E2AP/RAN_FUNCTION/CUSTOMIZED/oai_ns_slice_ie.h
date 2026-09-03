/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 *
 * Accessors for NS slice IEs used by ran_func_slice.c.
 *
 * Prefer FlexRIC tip types (slice_data_ie.h with NS_SET_POLICY). When the
 * submodule already defines ns_slice_* / SLICE_CTRL_SM_V0_NS_SET_POLICY,
 * use those layouts directly — no duplicate typedefs.
 */

#ifndef OAI_NS_SLICE_IE_H
#define OAI_NS_SLICE_IE_H

#include <stddef.h>
#include <stdint.h>

#include "openair2/E2AP/flexric/src/sm/slice_sm/ie/slice_data_ie.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Tip FlexRIC uses 0xffffff; keep unsigned literal for OAI call sites. */
#undef NS_SLICE_DEFAULT_SD
#define NS_SLICE_DEFAULT_SD 0xffffffu

/* Native tip layouts (ns_policy / ns_set_policy already in FlexRIC IEs). */
typedef slice_ind_msg_t oai_slice_ind_msg_layout_t;

static inline oai_slice_ind_msg_layout_t *oai_slice_ind_msg(slice_ind_msg_t *msg)
{
  return msg;
}

static inline const oai_slice_ind_msg_layout_t *oai_slice_ind_msg_const(const slice_ind_msg_t *msg)
{
  return msg;
}

static inline ns_slice_policy_list_t *oai_slice_ctrl_ns_policy(slice_ctrl_msg_t *msg)
{
  return &msg->u.ns_set_policy;
}

static inline const ns_slice_policy_list_t *oai_slice_ctrl_ns_policy_const(const slice_ctrl_msg_t *msg)
{
  return &msg->u.ns_set_policy;
}

#ifdef __cplusplus
}
#endif

#endif /* OAI_NS_SLICE_IE_H */
