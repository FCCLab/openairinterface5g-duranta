/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*!
 * \brief       Default pluggable policy functions for the DL scheduler pipeline.
 *
 * These are the built-in implementations behind the function pointers
 * (dl_ri_pmi_select, dl_tda_select, dl_beam_select, dl_mcs_select, dl_rb_alloc, dl_lcid_alloc)
 * wired up at MAC init time.  They can be replaced at runtime by external
 * scheduler plug-ins without touching the core scheduling loop in
 * gNB_scheduler_dlsch.c.
 *
 * nr_dl_proportional_fair (dl_rb_alloc default) honours nr_dl_sched_params_t::slice_rb_start/end
 * when SCHE_NS calls nr_dl_schedule() per slice:
 *   nr_slice_rb_bounds() maps absolute slice PRBs → BWP-relative search range
 *   get_rb_alloc_slice() places RBs only inside slice ∩ UE BWP
 */

#include "common/utils/nr/nr_common.h"
#include "gNB_scheduler_dlsch_default_policies.h"
/*MAC*/
#include "NR_MAC_COMMON/nr_mac.h"
#include "NR_MAC_gNB/nr_mac_gNB.h"
#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include "openair2/LAYER2/nr_rlc/nr_rlc_oai_api.h"

/*TAG*/
#include "NR_TAG-Id.h"

/*Softmodem params*/
#include "executables/softmodem-common.h"
#include "../../../nfapi/oai_integration/vendor_ext.h"

// Default RI/PMI selector: reads rank and PMI from CSI feedback for new-tx,
// or from HARQ process state for retx.
void nr_dl_ri_pmi_select_default(const nr_cell_sched_t *cell, nr_dl_candidate_t *candidates, int n_candidates)
{
  FOR_EACH_CANDIDATE(cand, candidates, n_candidates)
  {
    NR_UE_sched_ctrl_t *sched_ctrl = &cand->UE->UE_sched_ctrl;
    NR_UE_DL_BWP_t *dl_bwp = &cand->UE->current_DL_BWP;
    if (cand->is_retx) {
      cand->sched_pdsch.nrOfLayers = sched_ctrl->harq_processes[cand->retx_harq_pid].sched_pdsch.nrOfLayers;
      cand->sched_pdsch.pm_index =
          get_pm_index(cell, cand->UE, dl_bwp->dci_format, cand->sched_pdsch.nrOfLayers, cell->radio_config.pdsch_AntennaPorts.XP);
    } else {
      cand->sched_pdsch.nrOfLayers = cand->csi_ri + 1;
      cand->sched_pdsch.pm_index = cand->csi_pm_index;
    }
  }
}

// Default TDA selector: picks the slot-wide TDA index from get_dl_tda(),
// then resolves tda_info per candidate using each UE's own BWP / search
// space / coreset. Marks invalids with skipped=true.
int nr_dl_tda_select_default(const gNB_MAC_INST *mac, const nr_cell_sched_t *cell, nr_dl_candidate_t *candidates, int n_candidates, frame_t frame, slot_t slot)
{
  int tda = get_dl_tda(mac, cell, slot);
  AssertFatal(tda >= 0, "Unable to find PDSCH time domain allocation in list\n");
  const NR_ServingCellConfigCommon_t *scc = cell->common_channels.ServingCellConfigCommon;

  int n_valid = 0;
  FOR_EACH_CANDIDATE(cand, candidates, n_candidates)
  {
    if (cand->skipped)
      continue;
    NR_UE_info_t *UE = cand->UE;
    NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
    NR_UE_DL_BWP_t *dl_bwp = &UE->current_DL_BWP;
    int coresetid = sched_ctrl->coreset->controlResourceSetId;
    NR_tda_info_t tda_info = get_dl_tda_info(dl_bwp,
                                             sched_ctrl->search_space->searchSpaceType->present,
                                             tda,
                                             scc->dmrs_TypeA_Position,
                                             1,
                                             TYPE_C_RNTI_,
                                             coresetid,
                                             false);
    if (!tda_info.valid_tda) {
      cand->skipped = true;
      continue;
    }

    /* HARQ retx must keep the same TBS the UE stored from the initial TX.
     * Refitting rbSize across a different TDA (CSI-RS / mixed-slot rows) is
     * fragile under NS (moving PRB windows + DMRS) and produced UE-side
     * "NDI retx but TBS mismatch" → all-zero PDUs → SRB max RETX → RLF.
     * Only schedule retx when this slot's TDA matches the original; otherwise
     * defer to a later slot. */
    if (cand->is_retx) {
      const NR_sched_pdsch_t *orig = &sched_ctrl->harq_processes[cand->retx_harq_pid].sched_pdsch;
      bool tda_changed =
          tda != orig->time_domain_allocation
          || tda_info.startSymbolIndex != orig->tda_info.startSymbolIndex
          || tda_info.nrOfSymbols != orig->tda_info.nrOfSymbols;
      if (tda_changed) {
        cand->skipped = true;
        continue;
      }
      cand->sched_pdsch.time_domain_allocation = orig->time_domain_allocation;
      cand->sched_pdsch.tda_info = orig->tda_info;
      cand->alloc_slbitmap = SL_to_bitmap(orig->tda_info.startSymbolIndex, orig->tda_info.nrOfSymbols);
      cand->retx_rbSize = orig->rbSize;
      n_valid++;
      continue;
    }

    cand->sched_pdsch.time_domain_allocation = tda;
    cand->sched_pdsch.tda_info = tda_info;
    cand->alloc_slbitmap = SL_to_bitmap(tda_info.startSymbolIndex, tda_info.nrOfSymbols);
    n_valid++;
  }
  return n_valid;
}

static int compare_dl_pf_ptrs(const void *a, const void *b)
{
  const nr_dl_candidate_t *ca = *(const nr_dl_candidate_t *const *)a;
  const nr_dl_candidate_t *cb = *(const nr_dl_candidate_t *const *)b;
  /* retx first (INFINITY weight), then highest PF weight */
  float wa = ca->is_retx ? INFINITY : dl_pf_weight(ca->current_mcs, ca->mcs_table, ca->sched_pdsch.nrOfLayers, ca->avg_throughput);
  float wb = cb->is_retx ? INFINITY : dl_pf_weight(cb->current_mcs, cb->mcs_table, cb->sched_pdsch.nrOfLayers, cb->avg_throughput);
  return (wa < wb) - (wa > wb);
}

int nr_dl_beam_select_default(NR_beam_info_t *beam_info,
                              const int16_t *beam_index_list,
                              nr_dl_candidate_t *candidates,
                              int n_candidates,
                              frame_t frame,
                              slot_t slot,
                              int slots_per_frame)
{
  /* Build pointer array sorted by PF priority so retx and high-priority UEs claim beams first. */
  nr_dl_candidate_t *order[MAX_MOBILES_PER_GNB];
  int n_active = 0;
  FOR_EACH_CANDIDATE(cand, candidates, n_candidates)
  if (!cand->skipped)
    order[n_active++] = cand;
  qsort(order, n_active, sizeof(*order), compare_dl_pf_ptrs);

  int n_valid = 0;
  for (int i = 0; i < n_active; i++) {
    nr_dl_candidate_t *cand = order[i];
    NR_beam_alloc_t beam = beam_allocation_procedure(beam_info, frame, slot, cand->alloc_beam_dir, slots_per_frame);
    if (beam.idx < 0) {
      cand->skipped = true;
      continue;
    }

    cand->alloc_beam_idx = beam.idx;
    cand->alloc_new_beam = beam.new_beam;
    n_valid++;
  }
  return n_valid;
}

void nr_dl_mcs_select_default(const nr_cell_sched_t *cell, nr_dl_candidate_t *candidates, int n_candidates)
{
  const NR_bler_options_t *bo = &cell->dl_bler;
  FOR_EACH_CANDIDATE(cand, candidates, n_candidates)
  {
    int mcs;
    if (cand->is_retx) {
      mcs = cand->current_mcs; /* retx MCS is fixed by the HARQ round */
    } else if (bo->harq_round_max == 1) {
      mcs = max(bo->min_mcs, min(bo->max_mcs, cand->max_mcs));
    } else if (!cand->bler_updated) {
      mcs = cand->current_mcs;
    } else {
      mcs = nr_adapt_mcs_from_bler(cand->current_mcs,
                                   bo->min_mcs,
                                   cand->max_mcs,
                                   cand->bler,
                                   bo->lower,
                                   bo->upper,
                                   cand->last_num_sched);
    }
    cand->sched_pdsch.mcs = mcs;
    /* Persist for all candidates — BLER-based MCS ramps even for UEs the
     * policy rejects this slot (failed CCE, no free RBs, etc.). */
    if (!cand->is_retx)
      cand->UE->UE_sched_ctrl.dl_bler_stats.mcs = mcs;
  }
}

static int compare_dl_pf_rb_ptrs(const void *a, const void *b)
{
  const nr_dl_candidate_t *ca = *(const nr_dl_candidate_t *const *)a;
  const nr_dl_candidate_t *cb = *(const nr_dl_candidate_t *const *)b;
  /* retx first, then highest PF weight (uses sched_pdsch.mcs, which is set by mcs_select) */
  float wa =
      ca->is_retx ? INFINITY : dl_pf_weight(ca->sched_pdsch.mcs, ca->mcs_table, ca->sched_pdsch.nrOfLayers, ca->avg_throughput);
  float wb =
      cb->is_retx ? INFINITY : dl_pf_weight(cb->sched_pdsch.mcs, cb->mcs_table, cb->sched_pdsch.nrOfLayers, cb->avg_throughput);
  return (wa < wb) - (wa > wb);
}

/* RB allocation helper used by nr_dl_proportional_fair.
 * SCHE_PF: slice_rb_start < 0 → search full UE BWP (same as legacy get_rb_alloc).
 * SCHE_NS: intersect slice range from slice_prb_allocator with UE BWP, then search
 *          only inside that intersection via get_rb_alloc_slice(). */
static bool nr_dl_get_rb_alloc(const nr_dl_sched_params_t *params,
                               const nr_dl_candidate_t *cand,
                               int rbSize_min,
                               int rbSize_max,
                               const uint16_t *vrb_map,
                               int *rbStart,
                               int *rbSize)
{
  int slice_start, slice_end;
  nr_slice_rb_bounds(params->slice_rb_start,
                     params->slice_rb_end,
                     cand->bwp_start,
                     cand->bwp_size,
                     &slice_start,
                     &slice_end);
  if (slice_start >= slice_end && cand->bwp_size > 0) {
    slice_start = 0;
    slice_end = cand->bwp_size;
  }
  return get_rb_alloc_slice(rbSize_min,
                            rbSize_max,
                            cand->bwp_start,
                            cand->bwp_size,
                            vrb_map,
                            cand->alloc_slbitmap,
                            slice_start,
                            slice_end,
                            rbStart,
                            rbSize);
}

int nr_dl_proportional_fair(const nr_dl_sched_params_t *params, nr_dl_candidate_t *candidates, int n_candidates)
{
  int n_scheduled = 0;

  /* Build pointer array sorted by PF priority (retx first, then highest weight) */
  nr_dl_candidate_t *order[MAX_MOBILES_PER_GNB];
  int n_active = 0;
  FOR_EACH_CANDIDATE(cand, candidates, n_candidates)
  if (!cand->skipped)
    order[n_active++] = cand;
  qsort(order, n_active, sizeof(*order), compare_dl_pf_rb_ptrs);

  /* Phase 1: HARQ retransmissions (highest priority, exact RBs) */
  for (int j = 0; j < n_active; j++) {
    nr_dl_candidate_t *cand = order[j];
    if (!cand->is_retx)
      continue;

    int needed_rbs = cand->retx_rbSize;
    uint16_t *vrb_map = params->vrb_map[cand->alloc_beam_idx];
    int rbStart, rbSize;
    if (!nr_dl_get_rb_alloc(params, cand, needed_rbs, needed_rbs, vrb_map, &rbStart, &rbSize))
      continue;

    COMMIT_ALLOC(params, cand, rbStart, needed_rbs, cand->sched_pdsch.mcs, n_scheduled);
  }

  /* Phase 2: No-data UEs (TA command or beam switch MAC CE, no RLC data) */
  for (int j = 0; j < n_active; j++) {
    nr_dl_candidate_t *cand = order[j];
    if (cand->is_retx || cand->pending_bytes > 0)
      continue;

    uint16_t *vrb_map = params->vrb_map[cand->alloc_beam_idx];
    int rbStart, rbSize;
    if (!nr_dl_get_rb_alloc(params, cand, MIN_RB_SIZE, MIN_RB_SIZE, vrb_map, &rbStart, &rbSize))
      continue;

    COMMIT_ALLOC(params, cand, rbStart, MIN_RB_SIZE, cand->sched_pdsch.mcs, n_scheduled);
  }

  /* BW is the same across all beams, just use beam 0 */
  int max_rbSize = params->n_rb_avail[0];
  int n_remain_ue = params->max_num_ue - n_scheduled;
  if (n_remain_ue <= 0)
    return n_scheduled;
  /* NS can assign fewer PRBs than type-1 min (5); skip new-data PF rather than assert. */
  if (max_rbSize < MIN_RB_SIZE) {
    static frame_t last_warn_frame = -1;
    if (max_rbSize > 0 && params->frame != last_warn_frame) {
      last_warn_frame = params->frame;
      LOG_W(NR_MAC,
            "%4d.%2d DL PF: slice [%d,%d) has %d PRBs < min %d, skip new-data\n",
            params->frame,
            params->slot,
            params->slice_rb_start,
            params->slice_rb_end,
            max_rbSize,
            MIN_RB_SIZE);
    }
    return n_scheduled;
  }
  // share RBs fairly between remaining allocatable UEs
  int n_rb_per_ue = max(MIN_RB_SIZE, max_rbSize / n_remain_ue);

  /* Phase 3: New data UEs — PF priority order, count number of RBs required,
   * store number of excess RBs for UEs. Check two additional UEs in case the
   * first ones cannot be allocated (DCI alloc fail). This is only necessary
   * because we use type-1 allocation, if we used type-0, we could fix the UEs,
   * then iteratively give RBs as needed. */
  uint16_t rbs_ue[MAX_MOBILES_PER_GNB] = {0};
  int excess_total_rbs = max_rbSize;
  for (int j = 0, n = 0; j < n_active && n < n_remain_ue + 2; j++) {
    nr_dl_candidate_t *cand = order[j];
    if (cand->is_retx || cand->pending_bytes == 0)
      continue;

    // calculate the number of RBs that UE would like to have
    int mcs = cand->sched_pdsch.mcs;
    uint8_t Qm = nr_get_Qm_dl(mcs, cand->mcs_table);
    uint16_t R = nr_get_code_rate_dl(mcs, cand->mcs_table);
    const nr_cell_sched_t *cell = params->cell;
    NR_pdsch_dmrs_t dmrs = get_dl_dmrs_params(cell->common_channels.ServingCellConfigCommon,
                                              &cand->UE->current_DL_BWP,
                                              &cand->sched_pdsch.tda_info,
                                              cand->sched_pdsch.nrOfLayers);
    const int oh = 3 * 4 + (cand->UE->UE_sched_ctrl.ta_apply ? 2 : 0);
    uint32_t tbs;
    nr_find_nb_rb(Qm,
                  R,
                  1,
                  cand->sched_pdsch.nrOfLayers,
                  cand->sched_pdsch.tda_info.nrOfSymbols,
                  dmrs.N_PRB_DMRS * dmrs.N_DMRS_SLOT,
                  cand->pending_bytes + oh,
                  MIN_RB_SIZE,
                  max_rbSize,
                  &tbs,
                  &rbs_ue[j]);
    if (n < n_remain_ue) {
      // for the first n_remain_ue UEs: account number of RBs
      // so excess RBs not used by some UEs could be given to others
      excess_total_rbs -= min(rbs_ue[j], n_rb_per_ue);
      excess_total_rbs = max(excess_total_rbs, 0);
    }
    n++;
  }

  /* allocate up to all UEs checked above */
  for (int j = 0; j < n_active; j++) {
    nr_dl_candidate_t *cand = order[j];
    if (cand->is_retx || cand->pending_bytes == 0 || rbs_ue[j] == 0)
      continue;

    // give every UE its chunk of data. If total_rbs indicates excess RBs, give
    // additionally as appropriate.
    int rb_req = min(rbs_ue[j], n_rb_per_ue);
    int excess_req = max(rbs_ue[j] - rb_req, 0);
    if (excess_total_rbs > 0 && excess_req > 0) {
      int excess_ack = min(excess_total_rbs, excess_req);
      rb_req += excess_ack;
      excess_total_rbs -= excess_ack;
      DevAssert(excess_total_rbs >= 0);
    }
    int rbStart, rbSize;
    uint16_t *vrb_map = params->vrb_map[cand->alloc_beam_idx];
    if (!nr_dl_get_rb_alloc(params, cand, MIN_RB_SIZE, rb_req, vrb_map, &rbStart, &rbSize))
      continue;

    int mcs = cand->sched_pdsch.mcs;
    COMMIT_ALLOC(params, cand, rbStart, rbSize, mcs, n_scheduled);
  }

  return n_scheduled;
}

void nr_dl_lcid_alloc_default(const gNB_MAC_INST *mac,
                              const nr_dl_candidate_t *candidate,
                              int tbs_available,
                              int lcid_alloc[NR_MAX_NUM_LCID])
{
  (void)mac;
  (void)tbs_available;
  memset(lcid_alloc, 0, NR_MAX_NUM_LCID * sizeof(int));
  for (int lcid = 0; lcid < NR_MAX_NUM_LCID; lcid++)
    lcid_alloc[lcid] = candidate->pending_bytes_per_lcid[lcid];
}

int nr_dl_slice_proportional_fair(const nr_dl_sched_params_t *params, nr_dl_candidate_t *candidates, int n_candidates)
{
  if (n_candidates == 0)
    return 0;

  gNB_MAC_INST *mac = params->mac;
  slice_scheduler_t *slice_scheduler = mac ? mac->slice_scheduler_dl : NULL;
  if (slice_scheduler == NULL || slice_sch_get_num_slices(slice_scheduler) == 0) {
    return nr_dl_proportional_fair(params, candidates, n_candidates);
  }

  const nr_cell_sched_t *cell = params->cell;
  frame_t frame = params->frame;
  slot_t slot = params->slot;
  const int min_sched_prbs = max((int)cell->min_grant_prb, MIN_RB_SIZE);

  int total_prbs = params->n_rb_avail[0];
  if (total_prbs <= 0 && cell) {
    const NR_ServingCellConfigCommon_t *scc = cell->common_channels.ServingCellConfigCommon;
    if (scc && scc->downlinkConfigCommon && scc->downlinkConfigCommon->frequencyInfoDL &&
        scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.count > 0) {
      total_prbs = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
    }
  }
  if (total_prbs > 0)
    slice_sch_update_total_prbs(slice_scheduler, total_prbs);

  /* Estimate demand per slice from the active candidates */
  int num_slices = slice_sch_get_num_slices(slice_scheduler);
  for (int s = 0; s < num_slices; s++) {
    const slice_nssai_t *slice_nssai = slice_sch_get_slice_nssai(slice_scheduler, s);
    if (slice_nssai == NULL)
      continue;

    int required_prbs = 0;
    FOR_EACH_CANDIDATE(cand, candidates, n_candidates) {
      if (cand->skipped)
        continue;
      NR_UE_sched_ctrl_t *sched_ctrl = &cand->UE->UE_sched_ctrl;
      nssai_t ue_slice = {0};
      nr_mac_get_ue_effective_nssai(sched_ctrl, &ue_slice);
      if (ue_slice.sst != slice_nssai->sst || ue_slice.sd != slice_nssai->sd)
        continue;

      int ue_required_prbs = 0;
      bool srb_needs_dl = false;

      for (int l = 0; l < seq_arr_size(&sched_ctrl->lc_config); ++l) {
        const nr_lc_config_t *lc = seq_arr_at(&sched_ctrl->lc_config, l);
        if (lc->suspended)
          continue;

        const int lcid = lc->lcid;
        const uint16_t rnti = cand->UE->rnti;
        logical_chan_id_t ch = lcid;
        nr_mac_rlc_status_ind(rnti, frame, 1, &ch, &sched_ctrl->rlc_status[lcid]);

        if (lcid == UL_SCH_LCID_SRB1 || lcid == UL_SCH_LCID_SRB2) {
          if (sched_ctrl->rlc_status[lcid].bytes_in_buffer > 0
              || nr_rlc_am_status_triggered(rnti, lcid))
            srb_needs_dl = true;
        }

        if (sched_ctrl->rlc_status[lcid].bytes_in_buffer > 0) {
          const int bytes_per_prb_estimate = 4 - 2;
          int prbs_needed = (sched_ctrl->rlc_status[lcid].bytes_in_buffer + bytes_per_prb_estimate - 1) / bytes_per_prb_estimate;
          ue_required_prbs += prbs_needed;
        }
      }

      for (int harq_pid = sched_ctrl->retrans_dl_harq.head; harq_pid >= 0;
           harq_pid = sched_ctrl->retrans_dl_harq.next[harq_pid]) {
        ue_required_prbs += sched_ctrl->harq_processes[harq_pid].sched_pdsch.rbSize;
      }
      if (srb_needs_dl || ue_required_prbs > 0)
        ue_required_prbs = max(ue_required_prbs, min_sched_prbs);
      required_prbs += ue_required_prbs;
    }

    if (required_prbs > 0)
      required_prbs = max(required_prbs, min_sched_prbs);

    slice_sch_update_require(slice_scheduler, slice_nssai->sst, slice_nssai->sd, required_prbs);
  }

  /* Run four-pass slice PRB allocator */
  slice_sch_schedule(slice_scheduler);

  int num_ranges = 0;
  const slice_prb_range_t *allocation = slice_sch_get_allocation(slice_scheduler, &num_ranges);
  if (allocation == NULL || num_ranges == 0)
    return 0;

  const int slots_per_frame = cell->frame_structure.numb_slots_frame;
  const int start = (frame * slots_per_frame + slot) % num_ranges;

  int total_scheduled = 0;
  int remaining_ues = params->max_num_ue;

  /* HARQ retx has a fixed TBS/rbSize from the initial grant. Under NS the
   * per-slice PRB window moves and often shrinks below that size, so retx
   * confined to the slice never gets placed → RLC/SRB timeout → RLF.
   * This failure mode is NS-only (PF searches the full BWP). Schedule all
   * DL retx on the full carrier first; new data stays slice-isolated below. */
  {
    nr_dl_candidate_t retx_cands[MAX_MOBILES_PER_GNB];
    int retx_idx[MAX_MOBILES_PER_GNB];
    int n_retx = 0;
    for (int j = 0; j < n_candidates; j++) {
      if (candidates[j].skipped || candidates[j].scheduled || !candidates[j].is_retx)
        continue;
      retx_idx[n_retx] = j;
      retx_cands[n_retx] = candidates[j];
      n_retx++;
    }
    if (n_retx > 0 && remaining_ues > 0) {
      nr_dl_sched_params_t retx_params = *params;
      retx_params.max_num_ue = remaining_ues;
      retx_params.slice_rb_start = -1;
      retx_params.slice_rb_end = -1;
      const int full_bw = total_prbs > 0 ? total_prbs : params->n_rb_avail[0];
      for (int b = 0; b < params->num_beams; b++)
        retx_params.n_rb_avail[b] = full_bw;

      int n_sched = nr_dl_proportional_fair(&retx_params, retx_cands, n_retx);
      for (int k = 0; k < n_retx; k++)
        candidates[retx_idx[k]] = retx_cands[k];
      total_scheduled += n_sched;
      remaining_ues -= n_sched;
    }
  }

  /* Visit slices in rotating order, but pull control-critical slices
   * (HARQ retx / SRB) ahead so a tight max_num_ue cannot starve them. */
  enum { NR_NS_MAX_SLICE_ORDER = 64 };
  int order[NR_NS_MAX_SLICE_ORDER];
  int priority[NR_NS_MAX_SLICE_ORDER];
  int n_order = 0;
  const int n_ranges_cap = num_ranges < NR_NS_MAX_SLICE_ORDER ? num_ranges : NR_NS_MAX_SLICE_ORDER;
  for (int i = 0; i < n_ranges_cap; i++) {
    const int s = (start + i) % num_ranges;
    order[n_order] = s;
    int prio = 0;
    if (allocation[s].num_prbs > 0) {
      for (int j = 0; j < n_candidates; j++) {
        if (candidates[j].skipped || candidates[j].scheduled)
          continue;
        nssai_t ue_slice = {0};
        nr_mac_get_ue_effective_nssai(&candidates[j].UE->UE_sched_ctrl, &ue_slice);
        if (ue_slice.sst != allocation[s].slice_id.sst || ue_slice.sd != allocation[s].slice_id.sd)
          continue;
        if (candidates[j].is_retx)
          prio = 2;
        else {
          NR_UE_sched_ctrl_t *sc = &candidates[j].UE->UE_sched_ctrl;
          if (sc->rlc_status[UL_SCH_LCID_SRB1].bytes_in_buffer > 0
              || sc->rlc_status[UL_SCH_LCID_SRB2].bytes_in_buffer > 0)
            prio = max(prio, 1);
        }
      }
    }
    priority[n_order] = prio;
    n_order++;
  }
  for (int a = 0; a < n_order; a++) {
    for (int b = a + 1; b < n_order; b++) {
      if (priority[b] > priority[a]) {
        int ts = order[a], tp = priority[a];
        order[a] = order[b];
        priority[a] = priority[b];
        order[b] = ts;
        priority[b] = tp;
      }
    }
  }

  for (int i = 0; i < n_order; i++) {
    if (remaining_ues <= 0)
      break;
    const int s = order[i];
    if (allocation[s].num_prbs <= 0)
      continue;

    /* Build candidate sub-array for this slice */
    nr_dl_candidate_t slice_candidates[MAX_MOBILES_PER_GNB];
    int slice_cand_count = 0;
    int slice_cand_indices[MAX_MOBILES_PER_GNB];

    for (int j = 0; j < n_candidates; j++) {
      if (candidates[j].skipped || candidates[j].scheduled)
        continue;
      nssai_t ue_slice = {0};
      nr_mac_get_ue_effective_nssai(&candidates[j].UE->UE_sched_ctrl, &ue_slice);
      if (ue_slice.sst == allocation[s].slice_id.sst && ue_slice.sd == allocation[s].slice_id.sd) {
        slice_cand_indices[slice_cand_count] = j;
        slice_candidates[slice_cand_count] = candidates[j];
        slice_cand_count++;
      }
    }

    if (slice_cand_count == 0)
      continue;

    nr_dl_sched_params_t slice_params = *params;
    slice_params.max_num_ue = remaining_ues;
    slice_params.slice_rb_start = allocation[s].start_prb;
    slice_params.slice_rb_end = allocation[s].end_prb;
    for (int b = 0; b < params->num_beams; b++)
      slice_params.n_rb_avail[b] = allocation[s].num_prbs;

    int n_sched = nr_dl_proportional_fair(&slice_params, slice_candidates, slice_cand_count);

    /* Write back candidate decisions */
    for (int k = 0; k < slice_cand_count; k++) {
      int orig_idx = slice_cand_indices[k];
      candidates[orig_idx] = slice_candidates[k];
    }

    total_scheduled += n_sched;
    remaining_ues -= n_sched;
  }

  return total_scheduled;
}
