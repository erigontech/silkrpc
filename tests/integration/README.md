json-diff install:
------------------
- sudo apt update
- sudo apt install npm
- npm install -g json-diff


# Integration test (09/10)

### To run integration tests comparing results with json file: ./run_tests.py -c

```
Test Elapsed secs: 49
Number of executed tests:     266/266
Number of NOT executed tests: 0
Number of success tests:      266
Number of failed tests:       0
```


### To run integration tests comparing results with RPCdaemon response: ./run_tests.py -d -c
```
014. debug_traceCall/test_08.json                                 Failed(bad json format on expected rsp)
016. debug_traceCall/test_10.json                                 Failed
020. debug_traceCall/test_14.json                                 Failed
045. erigon_getBlockByTimestamp/test_1.json                       Skipped
060. eth_callBundle/test_1.json                                   Failed
061. eth_callBundle/test_2.json                                   Failed
062. eth_callBundle/test_3.json                                   Failed
063. eth_callBundle/test_4.json                                   Failed
069. eth_gasPrice/test_1.json                                     Failed
070. eth_getBlockByHash/test_1.json                               Skipped
071. eth_getBlockByHash/test_2.json                               Skipped
072. eth_getBlockByNumber/test_1.json                             Skipped
073. eth_getBlockByNumber/test_2.json                             Skipped
111. parity_getBlockReceipts/test_1.json                          Skipped
200. trace_replayBlockTransactions/test_01.json.tar               Failed
201. trace_replayBlockTransactions/test_02.json.tar               Failed
213. trace_replayBlockTransactions/test_14.json                   Failed
230. trace_replayTransaction/test_16.json.tar                     Failed
237. trace_replayTransaction/test_23.json.tar                     Failed
262. txpool_content/test_1.json                                   Skipped
                                                                                    
Test Elapsed secs: 72
Number of executed tests:     259/266
Number of NOT executed tests: 7
Number of success tests:      246
Number of failed tests:       13

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
