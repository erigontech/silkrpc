json-diff install:
------------------
- sudo apt update
- sudo apt install npm
- npm install -g json-diff
- pip install pyjwt


# Integration test (09/10)

### To run integration tests comparing results with json file: ./run_tests.py -c -k jwt.hex

```
Test time-elapsed (secs):     54
Number of executed tests:     367/367
Number of NOT executed tests: 0
Number of success tests:      367
Number of failed tests:       0
```


### To run integration tests comparing results with RPCdaemon response: ./run_tests.py -d -c -k jwt.hex
```
008. debug_traceBlockByHash/test_02.tar                           Skipped
009. debug_traceBlockByHash/test_03.tar                           Skipped
010. debug_traceBlockByHash/test_04.tar                           Skipped
012. debug_traceBlockByNumber/test_02.tar                         Skipped
029. debug_traceCall/test_10.json                                 Skipped
033. debug_traceCall/test_14.json                                 Skipped
036. debug_traceCall/test_17.json                                 Skipped
200. parity_getBlockReceipts/test_1.json                          Skipped
300. trace_rawTransaction/test_01.json                            Skipped
363. txpool_content/test_1.json                                   Skipped
                                                                                    
Test time-elapsed (secs):     69
Number of executed tests:     357/367
Number of NOT executed tests: 10
Number of success tests:      357
Number of failed tests:       0
```

#For local file
./run_tests.py -c -k /newDisk/jwt.hex 
055. engine_forkchoiceUpdatedV1/test_1.json                       Failed
056. engine_getPayloadV1/test_1.json                              Failed
057. engine_newPayloadV1/test_1.json                              Failed
072. erigon_nodeInfo/test_1.json                                  Failed
098. eth_coinbase/test_1.json                                     Failed
185. eth_getWork/test_1.json                                      Failed
186. eth_mining/test_1.json                                       Failed
187. eth_protocolVersion/test_1.json                              Failed
191. eth_submitHashrate/test_1.json                               Failed
192. eth_submitWork/test_1.json                                   Failed
195. net_peerCount/test_1.json                                    Failed
196. net_version/test_1.json                                      Failed
363. txpool_content/test_1.json                                   Failed
364. txpool_status/test_1.json                                    Failed
366. web3_clientVersion/test_1.json                               Failed
                                                                                    
Test time-elapsed (secs):     50
Number of executed tests:     367/367
Number of NOT executed tests: 0
Number of success tests:      352
Number of failed tests:       15




### Skipped tests reasons
```
debug_traceCall:            gasCost is not alligned between Silk and RPCDaemon for CALL/DELEGATE call temporary patch to RPCdaemon not cover all cases
parity_getBlockReceipts:    not supported by RPCdaemon
```
