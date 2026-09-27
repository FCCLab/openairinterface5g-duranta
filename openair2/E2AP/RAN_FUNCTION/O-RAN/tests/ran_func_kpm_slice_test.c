/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 *
 * Per-slice Style 1 DRB.PdcpSduVolume* and DRB.UEThp* through get_kpm_meas_value().
 * PDCP byte counters and active time are stubbed. The RRC UE tree is real.
 * UEThp is sum(PDCP SDU bytes) * 8 / max(active time ms).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "collection/tree.h"
#include "common/ran_context.h"
#include "openair2/E2AP/RAN_FUNCTION/O-RAN/ran_func_kpm_subs.h"
#include "openair2/F1AP/f1ap_ids.h"
#include "openair2/RRC/NR/rrc_gNB_UE_context.h"

RAN_CONTEXT_t RC;

static int failures;

static void expect_i64(const char *what, int64_t got, int64_t want)
{
  if (got != want) {
    fprintf(stderr, "FAIL %s: got %ld want %ld\n", what, (long)got, (long)want);
    failures++;
  }
}

static void expect_f(const char *what, double got, double want)
{
  const double diff = got - want;
  if (diff > 0.01 || diff < -0.01) {
    fprintf(stderr, "FAIL %s: got %f want %f\n", what, got, want);
    failures++;
  }
}

int rrc_gNB_compare_ue_rnti_id(rrc_gNB_ue_context_t *c1, rrc_gNB_ue_context_t *c2)
{
  if (c1->ue_context.rrc_ue_id > c2->ue_context.rrc_ue_id)
    return 1;
  if (c1->ue_context.rrc_ue_id < c2->ue_context.rrc_ue_id)
    return -1;
  return 0;
}

RB_GENERATE(rrc_nr_ue_tree_s, rrc_gNB_ue_context_s, entries, rrc_gNB_compare_ue_rnti_id);

rrc_gNB_ue_context_t *rrc_gNB_get_ue_context_by_rnti(gNB_RRC_INST *rrc, sctp_assoc_t assoc_id, rnti_t rnti)
{
  (void)assoc_id;
  rrc_gNB_ue_context_t *ctx = NULL;
  RB_FOREACH(ctx, rrc_nr_ue_tree_s, &rrc->rrc_ue_head) {
    if (ctx->ue_context.rnti == rnti)
      return ctx;
  }
  return NULL;
}

void exit_function(const char *file, const char *function, const int line, const char *s, const int assert_code)
{
  fprintf(stderr, "exit_function %s at %s:%d %s (%d)\n", s, file, line, function, assert_code);
  abort();
}

typedef struct {
  uint32_t rrc_ue_id;
  bool present;
  bool ok;
  uint32_t rx_bytes;
  uint32_t tx_bytes;
  uint64_t dl_active_ms;
  uint64_t ul_active_ms;
} pdcp_stub_t;

static pdcp_stub_t pdcp_stubs[8];

bool nr_pdcp_get_statistics(ue_id_t ue_id, int srb_flag, int rb_id, nr_pdcp_statistics_t *out)
{
  (void)srb_flag;
  (void)rb_id;
  memset(out, 0, sizeof(*out));
  for (size_t i = 0; i < sizeof(pdcp_stubs) / sizeof(pdcp_stubs[0]); i++) {
    if (!pdcp_stubs[i].present || pdcp_stubs[i].rrc_ue_id != ue_id)
      continue;
    if (!pdcp_stubs[i].ok)
      return false;
    out->rxsdu_bytes = pdcp_stubs[i].rx_bytes;
    out->txsdu_bytes = pdcp_stubs[i].tx_bytes;
    return true;
  }
  return false;
}

bool nr_pdcp_consume_sdu_activity(ue_id_t ue_id, int srb_flag, int rb_id, bool downlink,
                                  uint32_t *bytes, uint64_t *active_ms)
{
  (void)srb_flag;
  (void)rb_id;
  for (size_t i = 0; i < sizeof(pdcp_stubs) / sizeof(pdcp_stubs[0]); i++) {
    if (!pdcp_stubs[i].present || pdcp_stubs[i].rrc_ue_id != ue_id)
      continue;
    if (!pdcp_stubs[i].ok)
      return false;
    if (downlink) {
      *bytes = pdcp_stubs[i].rx_bytes;
      *active_ms = pdcp_stubs[i].dl_active_ms;
      pdcp_stubs[i].dl_active_ms = 0;
    } else {
      *bytes = pdcp_stubs[i].tx_bytes;
      *active_ms = pdcp_stubs[i].ul_active_ms;
      pdcp_stubs[i].ul_active_ms = 0;
    }
    return true;
  }
  return false;
}

typedef struct {
  rnti_t rnti;
  bool present;
  bool ok;
  uint32_t txpdu_bytes;
  uint32_t rxpdu_bytes;
} rlc_stub_t;

static rlc_stub_t rlc_stubs[8];

bool nr_rlc_get_statistics(int ue_id, int srb_flag, int rb_id, nr_rlc_statistics_t *out)
{
  (void)srb_flag;
  (void)rb_id;
  memset(out, 0, sizeof(*out));
  for (size_t i = 0; i < sizeof(rlc_stubs) / sizeof(rlc_stubs[0]); i++) {
    if (!rlc_stubs[i].present || rlc_stubs[i].rnti != (rnti_t)ue_id)
      continue;
    if (!rlc_stubs[i].ok)
      return false;
    out->txpdu_bytes = rlc_stubs[i].txpdu_bytes;
    out->rxpdu_bytes = rlc_stubs[i].rxpdu_bytes;
    return true;
  }
  return false;
}

void nr_rlc_activate_avg_time_to_tx(const int ue_id, const logical_chan_id_t channel_id, const bool is_on)
{
  (void)ue_id;
  (void)channel_id;
  (void)is_on;
}

bool du_exists_f1_ue_data(uint32_t ue_id)
{
  (void)ue_id;
  return false;
}

f1_ue_data_t du_get_f1_ue_data(uint32_t ue_id)
{
  (void)ue_id;
  f1_ue_data_t data = {0};
  return data;
}

static pdcp_stub_t *pdcp_for(uint32_t rrc_ue_id)
{
  for (size_t i = 0; i < sizeof(pdcp_stubs) / sizeof(pdcp_stubs[0]); i++) {
    if (!pdcp_stubs[i].present) {
      pdcp_stubs[i].present = true;
      pdcp_stubs[i].ok = true;
      pdcp_stubs[i].rrc_ue_id = rrc_ue_id;
      return &pdcp_stubs[i];
    }
    if (pdcp_stubs[i].rrc_ue_id == rrc_ue_id)
      return &pdcp_stubs[i];
  }
  abort();
}

static label_info_lst_t slice_label(s_nssai_e2sm_t *slice, uint32_t *sd, uint8_t sst, uint32_t sd_value)
{
  *sd = sd_value;
  memset(slice, 0, sizeof(*slice));
  slice->sST = sst;
  slice->sD = sd;
  label_info_lst_t label = {0};
  label.sliceID = slice;
  return label;
}

static meas_record_lst_t measure(const char *name, label_info_lst_t label, uint32_t gran_ms, cudu_ue_info_pair_t ue)
{
  e2_node_level_stats_t stats[2] = {0};
  return get_kpm_meas_value((char *)name, label, gran_ms, ue, 0, stats);
}

static void add_rrc_ue(gNB_RRC_INST *rrc, uint32_t rrc_ue_id, rnti_t rnti, uint8_t sst, uint32_t sd)
{
  rrc_gNB_ue_context_t *ctx = calloc(1, sizeof(*ctx));
  ctx->ue_context.rrc_ue_id = rrc_ue_id;
  ctx->ue_context.rnti = rnti;
  seq_arr_init(&ctx->ue_context.pduSessions, sizeof(rrc_pdu_session_param_t));
  rrc_pdu_session_param_t pdu = {0};
  pdu.param.nssai.sst = sst;
  pdu.param.nssai.sd = sd;
  seq_arr_push_back(&ctx->ue_context.pduSessions, &pdu, sizeof(pdu));
  RB_INSERT(rrc_nr_ue_tree_s, &rrc->rrc_ue_head, ctx);
}

/* 125000000 bytes * 8 / 1000000 = 1000 Mbit */
enum { kPdcpBytes = 125000000, kGranMs = 1000 };

static void test_pdcp_volume(void)
{
  cudu_ue_info_pair_t node = {0};
  label_info_lst_t none = {0};
  meas_record_lst_t rec;

  rec = measure("DRB.PdcpSduVolumeDL", none, kGranMs, node);
  expect_i64("PDCP DL node with no RRC", rec.int_val, 0);
  expect_i64("PDCP DL node value type", rec.value, INTEGER_MEAS_VALUE);

  /* Style 4, rrc_ue_id 1. Kept out of the node-level tree so its baseline stays independent. */
  cudu_ue_info_pair_t ue1 = {.rrc_ue_id = 1};
  pdcp_stub_t *p1 = pdcp_for(1);
  p1->rx_bytes = kPdcpBytes;
  p1->tx_bytes = kPdcpBytes / 2;
  rec = measure("DRB.PdcpSduVolumeDL", none, kGranMs, ue1);
  expect_i64("PDCP DL per UE first sample (Mbit)", rec.int_val, 1000);
  p1->rx_bytes = 2 * kPdcpBytes;
  rec = measure("DRB.PdcpSduVolumeDL", none, kGranMs, ue1);
  expect_i64("PDCP DL per UE delta (Mbit)", rec.int_val, 1000);
  rec = measure("DRB.PdcpSduVolumeUL", none, kGranMs, ue1);
  expect_i64("PDCP UL per UE uses txsdu not rxsdu (Mbit)", rec.int_val, 500);

  gNB_RRC_INST *rrc = calloc(1, sizeof(*rrc));
  RB_INIT(&rrc->rrc_ue_head);
  RC.nrrrc = calloc(1, sizeof(*RC.nrrrc));
  RC.nrrrc[0] = rrc;

  /* sd 1 and sd 2. A third UE on sd 1 has no PDCP entity and must be skipped. */
  add_rrc_ue(rrc, 2, 0x200, 1, 1);
  add_rrc_ue(rrc, 3, 0x300, 1, 2);
  add_rrc_ue(rrc, 4, 0x400, 1, 1);
  pdcp_stub_t *p2 = pdcp_for(2);
  pdcp_stub_t *p3 = pdcp_for(3);
  pdcp_stub_t *p4 = pdcp_for(4);
  p2->rx_bytes = 2 * kPdcpBytes;
  p2->tx_bytes = kPdcpBytes / 2;
  p3->rx_bytes = kPdcpBytes;
  p3->tx_bytes = kPdcpBytes / 5;
  p4->ok = false;
  p4->rx_bytes = 5u * kPdcpBytes;

  s_nssai_e2sm_t slice1, slice2, slice9;
  uint32_t sd1v = 0, sd2v = 0, sd9v = 0;
  label_info_lst_t sd1 = slice_label(&slice1, &sd1v, 1, 1);
  label_info_lst_t sd2 = slice_label(&slice2, &sd2v, 1, 2);
  label_info_lst_t other = slice_label(&slice9, &sd9v, 1, 9);
  rec = measure("DRB.PdcpSduVolumeDL", sd1, kGranMs, node);
  expect_i64("PDCP DL slice sd=1 sums only that slice (Mbit)", rec.int_val, 2000);

  rec = measure("DRB.PdcpSduVolumeDL", sd2, kGranMs, node);
  expect_i64("PDCP DL slice sd=2 (Mbit)", rec.int_val, 1000);

  rec = measure("DRB.PdcpSduVolumeDL", sd1, kGranMs, node);
  expect_i64("PDCP DL slice sd=1 second sample is the delta", rec.int_val, 0);

  p2->rx_bytes += kPdcpBytes;
  rec = measure("DRB.PdcpSduVolumeDL", sd1, kGranMs, node);
  expect_i64("PDCP DL slice sd=1 after more bytes (Mbit)", rec.int_val, 1000);

  rec = measure("DRB.PdcpSduVolumeUL", sd1, kGranMs, node);
  expect_i64("PDCP UL slice sd=1 (Mbit)", rec.int_val, 500);
  rec = measure("DRB.PdcpSduVolumeUL", sd2, kGranMs, node);
  expect_i64("PDCP UL slice sd=2 (Mbit)", rec.int_val, 200);

  rec = measure("DRB.PdcpSduVolumeDL", other, kGranMs, node);
  expect_i64("PDCP DL unknown slice", rec.int_val, 0);

  rec = measure("DRB.PdcpSduVolumeDL", none, kGranMs, node);
  expect_i64("PDCP DL all slices after baselines are consumed", rec.int_val, 0);
}

static void test_ue_thp(void)
{
  cudu_ue_info_pair_t node = {0};
  label_info_lst_t none = {0};
  meas_record_lst_t rec;

  gNB_RRC_INST **saved_rrc = RC.nrrrc;
  RC.nrrrc = NULL;
  rec = measure("DRB.UEThpDl", none, kGranMs, node);
  expect_f("UEThp DL node with no RRC", rec.real_val, 0.0);
  expect_i64("UEThp DL node value type", rec.value, REAL_MEAS_VALUE);
  RC.nrrrc = saved_rrc;

  /* Style 4, rrc_ue_id 1. 125000 bytes * 8 / 100 ms = 10000 kbps. */
  cudu_ue_info_pair_t ue1 = {.rrc_ue_id = 1};
  pdcp_stub_t *p1 = pdcp_for(1);
  p1->rx_bytes = 125000;
  p1->tx_bytes = 62500;
  p1->dl_active_ms = 100;
  p1->ul_active_ms = 100;
  rec = measure("DRB.UEThpDl", none, 1, ue1);
  expect_f("UEThp DL per UE first sample (kbps)", rec.real_val, 10000.0);
  p1->rx_bytes += 125000;
  p1->dl_active_ms = 100;
  rec = measure("DRB.UEThpDl", none, kGranMs, ue1);
  expect_f("UEThp DL per UE delta (kbps)", rec.real_val, 10000.0);
  p1->rx_bytes += 1000;
  rec = measure("DRB.UEThpDl", none, kGranMs, ue1);
  expect_f("UEThp DL per UE zero span uses 1 ms floor (kbps)", rec.real_val, 8000.0);
  rec = measure("DRB.UEThpUl", none, kGranMs, ue1);
  expect_f("UEThp UL per UE uses txsdu not rxsdu (kbps)", rec.real_val, 5000.0);

  /* UE 4 starts with no PDCP entity and must be skipped until it is marked present. */
  pdcp_stub_t *p2 = pdcp_for(2);
  pdcp_stub_t *p3 = pdcp_for(3);
  pdcp_stub_t *p4 = pdcp_for(4);
  p2->rx_bytes = 100000;
  p2->dl_active_ms = 100;
  p4->ok = false;
  p4->rx_bytes = 5u * 125000;
  p4->dl_active_ms = 40;

  s_nssai_e2sm_t slice1, slice2, slice9;
  uint32_t sd1v = 0, sd2v = 0, sd9v = 0;
  label_info_lst_t sd1 = slice_label(&slice1, &sd1v, 1, 1);
  label_info_lst_t sd2 = slice_label(&slice2, &sd2v, 1, 2);
  label_info_lst_t other = slice_label(&slice9, &sd9v, 1, 9);
  rec = measure("DRB.UEThpDl", sd1, 1, node);
  expect_f("UEThp DL slice sd=1 skips a UE with no PDCP (kbps)", rec.real_val, 8000.0);

  /* UE2 adds 100000 bytes over 100 ms. UE4 adds 50000 over 40 ms.
     sum/max = 150000*8/100 = 12000. Summing each UE's rate would be 18000. */
  p2->rx_bytes += 100000;
  p2->dl_active_ms = 100;
  p4->ok = true;
  p4->rx_bytes = 50000;
  p4->dl_active_ms = 40;
  p3->rx_bytes = 25000;
  p3->dl_active_ms = 50;
  rec = measure("DRB.UEThpDl", sd1, 1, node);
  expect_f("UEThp DL slice sd=1 is sum/max active time (kbps)", rec.real_val, 12000.0);

  rec = measure("DRB.UEThpDl", sd2, kGranMs, node);
  expect_f("UEThp DL slice sd=2 (kbps)", rec.real_val, 4000.0);

  rec = measure("DRB.UEThpDl", sd1, kGranMs, node);
  expect_f("UEThp DL slice sd=1 second sample is the delta", rec.real_val, 0.0);

  p2->rx_bytes += 125000;
  p2->dl_active_ms = 100;
  /* Idle UE's span must not stretch the denominator (that would make this 2500 kbps). */
  p4->dl_active_ms = 400;
  rec = measure("DRB.UEThpDl", sd1, kGranMs, node);
  expect_f("UEThp DL slice sd=1 after more bytes uses max active time (kbps)", rec.real_val, 10000.0);

  p2->tx_bytes = 62500;
  p2->ul_active_ms = 100;
  p3->tx_bytes = 25000;
  p3->ul_active_ms = 50;
  rec = measure("DRB.UEThpUl", sd1, kGranMs, node);
  expect_f("UEThp UL slice sd=1 (kbps)", rec.real_val, 5000.0);
  rec = measure("DRB.UEThpUl", sd2, kGranMs, node);
  expect_f("UEThp UL slice sd=2 (kbps)", rec.real_val, 4000.0);

  rec = measure("DRB.UEThpDl", other, kGranMs, node);
  expect_f("UEThp DL unknown slice", rec.real_val, 0.0);
}

int main(void)
{
  init_kpm_subs_data();
  test_pdcp_volume();
  test_ue_thp();
  if (failures != 0) {
    fprintf(stderr, "%d KPM slice test(s) failed\n", failures);
    return 1;
  }
  printf("ran_func_kpm_slice_test passed\n");
  return 0;
}
