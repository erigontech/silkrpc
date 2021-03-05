# Performance Tests

These are the instructions to setup the performance comparison tests between Silkrpc and Turbo-Geth (TG) RPCDaemon. Currently such performance testing is **not** automated, so it is necessary to manually checkout/build/execute the components.

## Software Versions

In order to reproduce the environment used in last performance testing session, pick the following source code versions:

* TG RPCDaemon commit: [3388c1f](https://github.com/ledgerwatch/turbo-geth/commit/3388c1f1af6c65808830e5839a0c6d5d78f018fa)
* Silkrpc commit: [072dbc0](https://github.com/torquem-ch/silkrpc/commit/072dbc0314f383fbe236fc0c26e34187fe2191ca)

## Build

Follow the instructions for building:

* TG RPCDaemon [build](https://github.com/)
* Silkrpc [build](https://github.com/torquem-ch/silkrpc/tree/eth_get_logs#linux--macos)

## Activation

The command lines to activate such components for performance testing are listed below (you can also experiment allocating a different number of cores or removing `taskset`).

### _TG Core_
From Turbo-Geth project directory:
```
build/bin/tg --goerli --private.api.addr=localhost:9090
```

### _TG RPCDaemon_
From Turbo-Geth project directory:
```
taskset -c 1 build/bin/rpcdaemon --private.api.addr=localhost:9090 --http.api=eth,debug,net,web3
```
RPCDaemon will be running on port 8545

### _Silkrpc_
From Silkrpc project directory:
```
taskset -c 0-1 build_gcc_release/silkrpc/silkrpcdaemon --target localhost:9090
```
Silkrpc will be running on port 51515

## Test Workload

Currently the performance workload targets just the [eth_getLogs](https://eth.wiki/json-rpc/API#eth_getlogs) Ethereum API. The test workloads are executed using requests files of [Vegeta](https://github.com/tsenart/vegeta/), a HTTP load testing tool.

### _Workload Generation_

Execute Turbo-Geth [bench8 tool](https://github.com/ledgerwatch/turbo-geth/blob/3388c1f1af6c65808830e5839a0c6d5d78f018fa/cmd/rpctest/rpctest/bench8.go) against both Turbo-Geth RPCDaemon and Silkrpc using the following command line:

```
build/bin/rpctest bench8 --tgUrl http://localhost:8545 --gethUrl http://localhost:51515 --needCompare --block 200000
```

Vegeta request files are written to `/tmp/turbo_geth_stress_test`:
* results_geth_debug_getModifiedAccountsByNumber.csv, results_geth_eth_getLogs.csv
* results_turbo_geth_debug_getModifiedAccountsByNumber.csv, results_turbo_geth_eth_getLogs.csv
* vegeta_geth_debug_getModifiedAccountsByNumber.txt, vegeta_geth_eth_getLogs.txt
* vegeta_turbo_geth_debug_getModifiedAccountsByNumber.txt, vegeta_turbo_geth_eth_getLogs.txt

### _Workload Activation_

From Silkrpc project directory execute the Vegeta attack using the scripts in [tests/perf](https://github.com/torquem-ch/silkrpc/tree/072dbc0314f383fbe236fc0c26e34187fe2191ca/tests/perf):
```
tests/perf/vegeta_attack_getLogs_rpcdaemon.sh [rate] [duration]
tests/perf/vegeta_attack_getLogs_silkrpc.sh [rate] [duration]
```
where `[rate]` indicates the target query-per-seconds during the attack (optional, default: 200) and `[duration]` is the duration in seconds of the attack (optional, default: 30)

Vegeta reports in text format are written to the working directory.