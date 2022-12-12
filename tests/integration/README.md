json-diff install:
------------------
- sudo apt update
- sudo apt install npm
- npm install -g json-diff
- pip install pyjwt


# Integration test (09/10)

### To run integration tests comparing results with json file: ./run_tests.py -c -k jwt.hex

```
Test time-elapsed (secs):     52
Number of executed tests:     320/320
Number of NOT executed tests: 0
Number of success tests:      320
Number of failed tests:       0

```


### To run integration tests comparing results with RPCdaemon response: ./run_tests.py -d -c -k jwt.hex
```
016. debug_traceCall/test_10.json                                 Skipped
020. debug_traceCall/test_14.json                                 Skipped
153. parity_getBlockReceipts/test_1.json                          Skipped
253. trace_rawTransaction/test_01.json                            Skipped
316. txpool_content/test_1.json                                   Skipped
                                                                                    
Test time-elapsed (secs):     70
Number of executed tests:     315/320
Number of NOT executed tests: 5
Number of success tests:      315
Number of failed tests:       0

```

### Skipped tests reasons
```
debug_traceCall:            gasCost is not alligned between Silk and RPCDaemon for CALL/DELEGATE call temporary patch to RPCdaemon not cover all cases
parity_getBlockReceipts:    not supported by RPCdaemon
```
