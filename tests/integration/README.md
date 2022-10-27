json-diff install:
------------------
- sudo apt update
- sudo apt install npm
- npm install -g json-diff


# Integration test (09/10)

### To run integration tests comparing results with json file: ./run_tests.py -c

```
Test time-elapsed (secs):     89
Number of executed tests:     282/282
Number of NOT executed tests: 0
Number of success tests:      282
Number of failed tests:       0

```


### To run integration tests comparing results with RPCdaemon response: ./run_tests.py -d -c
```
016. debug_traceCall/test_10.json                                 Skipped
020. debug_traceCall/test_14.json                                 Skipped
127. parity_getBlockReceipts/test_1.json                          Skipped
278. txpool_content/test_1.json                                   Skipped
                                                                                    
Test time-elapsed (secs):     68
Number of executed tests:     278/282
Number of NOT executed tests: 4
Number of success tests:      278
Number of failed tests:       0
```

### Skipped tests reasons
```
debug_traceCall:            gasCost is not alligned between Silk and RPCDaemon for CALL/DELEGATE call temporary patch to RPCdaemon not cover all cases
parity_getBlockReceipts:    not supported by RPCdaemon

```
