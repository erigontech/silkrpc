json-diff install:
------------------
- sudo apt update
- sudo apt install npm
- npm install -g json-diff


# Integration test (09/10)

### To run integration tests comparing results with json file: ./run_tests.py -c

```
Test time-elapsed (secs):     46
Number of executed tests:     272/272
Number of NOT executed tests: 0
Number of success tests:      272
Number of failed tests:       0

```


### To run integration tests comparing results with RPCdaemon response: ./run_tests.py -d -c
```
016. debug_traceCall/test_10.json                                 Skipped
020. debug_traceCall/test_14.json                                 Skipped
045. erigon_getBlockByTimestamp/test_1.json                       Skipped
071. eth_getBlockByHash/test_1.json                               Skipped
072. eth_getBlockByHash/test_2.json                               Skipped
073. eth_getBlockByNumber/test_1.json                             Skipped
074. eth_getBlockByNumber/test_2.json                             Skipped
117. parity_getBlockReceipts/test_1.json                          Skipped
268. txpool_content/test_1.json                                   Skipped
                                                                                    
Test time-elapsed (secs):     64
Number of executed tests:     263/272
Number of NOT executed tests: 9
Number of success tests:      263
Number of failed tests:       0

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
