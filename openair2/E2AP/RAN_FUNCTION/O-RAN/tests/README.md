This directory contains the **canonical O-RAN E2SM-RC** RRM Policy Ratio
control-message builders and tests for OAI NS / PRB control.

| File | Role |
|------|------|
| `rc_enc_asn_xapp_lib.h` / `.c` | **Main O-RAN-compliant** encoder (`get_ctrl_msg_frmt_1`) — S-NSSAI SST/SD (1+3 octet), PLMN, Min/Max/Dedicated PRB ratios |
| `ran_func_rc_test.c` | Unit test: encode → `extract_slice_config_from_ctrl_req` |

Currently, `test_extract_slice_config` checks extraction of `{min,max,dedicated,sst,sd}` from an `e2sm_rc_ctrl_msg_frmt_1_t`.

To run the test:
```bash
bash openairinterface5g/openair2/E2AP/RAN_FUNCTION/O-RAN/tests/run_rc_test.sh
```
