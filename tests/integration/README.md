json-diff install:
------------------
- sudo apt update
- sudo apt install npm
- npm install -g json-diff


# Integration test (09/10)

### To run integration tests comparing results with json file: ./run_tests.py -c

```
Test time-elapsed (secs):     47
Number of executed tests:     268/268
Number of NOT executed tests: 0
Number of success tests:      268
Number of failed tests:       0

```


### To run integration tests comparing results with RPCdaemon response: ./run_tests.py -d -c
```
014. debug_traceCall/test_08.json                                 Failed (bad json format on expected rsp)
016. debug_traceCall/test_10.json                                 Failed
020. debug_traceCall/test_14.json                                 Failed
045. erigon_getBlockByTimestamp/test_1.json                       Skipped
070. eth_getBlockByHash/test_1.json                               Skipped
071. eth_getBlockByHash/test_2.json                               Skipped
072. eth_getBlockByNumber/test_1.json                             Skipped
073. eth_getBlockByNumber/test_2.json                             Skipped
113. parity_getBlockReceipts/test_1.json                          Skipped
202. trace_replayBlockTransactions/test_01.json.tar               Failed
203. trace_replayBlockTransactions/test_02.json.tar               Failed
215. trace_replayBlockTransactions/test_14.json                   Failed
232. trace_replayTransaction/test_16.json.tar                     Failed
239. trace_replayTransaction/test_23.json.tar                     Failed
264. txpool_content/test_1.json                                   Skipped
                                                                                    
Test time-elapsed (secs):     76
Number of executed tests:     261/268
Number of NOT executed tests: 7
Number of success tests:      253
Number of failed tests:       8

```

### Skipped tests reasons
```
erigon_getBlockByTimestamp: RPCdaemon contains always chainId even if not valid and doesn't contain accessList 
                            if empty. Both seems errors
eth_getBlockByHash:         RPCdaemon contains always chainId even if not valid. Seems wrong
eth_getBlockByNumber:       RPCdaemon contains always chainId even if not valid. Seems wrong
txpool_content:             All RPCDaemon transactions contains always chainId even if not valid and doesn't contain 
                            accessList if empty. Both seems errors

```
