
30/09

to run integration tests verifying with saved json file:
./run_tests.py -c
194. trace_replayBlockTransactions/test_01.json.tar               Failed
195. trace_replayBlockTransactions/test_02.json.tar               Failed
224. trace_replayTransaction/test_16.json.tar                     Failed
231. trace_replayTransaction/test_23.json.tar                     Failed
256. txpool_content/test_1.json                                   Failed
                                                                                    
Number of executed tests:     260/260
Number of NOT executed tests: 0
Number of success tests:      255
Number of failed tests:       5



to run integration tests verifying with RPCdaemon:
./run_tests.py -d -c
014. debug_traceCall/test_08.json                                 Failed(bad json format on expected rsp)
016. debug_traceCall/test_10.json                                 Failed
020. debug_traceCall/test_14.json                                 Failed
045. erigon_getBlockByTimestamp/test_1.json                       Skipped
049. erigon_issuance/test_1.json                                  Failed
060. eth_callBundle/test_1.json                                   Failed
061. eth_callBundle/test_2.json                                   Failed
062. eth_callBundle/test_3.json                                   Failed
063. eth_callBundle/test_4.json                                   Failed
070. eth_getBlockByHash/test_1.json                               Skipped
071. eth_getBlockByHash/test_2.json                               Skipped
072. eth_getBlockByNumber/test_1.json                             Skipped
073. eth_getBlockByNumber/test_2.json                             Skipped
107. parity_getBlockReceipts/test_1.json                          Skipped
108. parity_listStorageKeys/test_1.json                           Failed
194. trace_replayBlockTransactions/test_01.json.tar               Failed
195. trace_replayBlockTransactions/test_02.json.tar               Failed
207. trace_replayBlockTransactions/test_14.json                   Failed
224. trace_replayTransaction/test_16.json.tar                     Failed
231. trace_replayTransaction/test_23.json.tar                     Failed
256. txpool_content/test_1.json                                   Failed
                                                                                    
Number of executed tests:     254/260
Number of NOT executed tests: 6
Number of success tests:      239
Number of failed tests:       15

