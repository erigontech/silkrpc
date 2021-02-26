#!/bin/bash

VEGETA_BODY_FILE=/tmp/turbo_geth_stress_test/vegeta_geth_eth_getLogs.txt
cat $VEGETA_BODY_FILE | vegeta attack -keepalive -rate=100 -format=json -duration=30s -timeout=300s | vegeta report -type=text > getLogs_100qps_30s_silkrpc_perf.hrd
