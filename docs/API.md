## RPC API Implementation Status

The following table shows the current [JSON RPC API](https://eth.wiki/json-rpc/API) implementation status in `Silkrpc`.

| Command                                    | Availability | Notes                                      |
| :------------------------------------------| :----------: | -----------------------------------------: |
| admin_nodeInfo                             | -            | not yet implemented                        |
|                                            |              |                                            |
| web3_clientVersion                         | Yes          |                                            |
| web3_sha3                                  | Yes          |                                            |
|                                            |              |                                            |
| net_listening                              | Yes          | hard-coded (needs ethbackend integration)  |
| net_peerCount                              | Yes          |                                            |
| net_version                                | Yes          |                                            |
|                                            |              |                                            |
| eth_blockNumber                            | Yes          |                                            |
| eth_chainId                                | Yes          |                                            |
| eth_protocolVersion                        | Yes          |                                            |
| eth_syncing                                | Yes          |                                            |
| eth_gasPrice                               | Yes          |                                            |
|                                            |              |                                            |
| eth_getBlockByHash                         | Yes          |                                            |
| eth_getBlockByNumber                       | Yes          |                                            |
| eth_getBlockTransactionCountByHash         | Yes          |                                            |
| eth_getBlockTransactionCountByNumber       | Yes          |                                            |
| eth_getUncleByBlockHashAndIndex            | Yes          |                                            |
| eth_getUncleByBlockNumberAndIndex          | Yes          |                                            |
| eth_getUncleCountByBlockHash               | Yes          |                                            |
| eth_getUncleCountByBlockNumber             | Yes          |                                            |
|                                            |              |                                            |
| eth_getTransactionByHash                   | Yes          | partially implemented                      |
| eth_getRawTransactionByHash                | Yes          | partially implemented                      |
| eth_getTransactionByBlockHashAndIndex      | Yes          |                                            |
| eth_getRawTransactionByBlockHashAndIndex   | Yes          | partially implemented                      |
| eth_getTransactionByBlockNumberAndIndex    | Yes          |                                            |
| eth_getRawTransactionByBlockNumberAndIndex | Yes          | partially implemented                      |
| eth_getTransactionReceipt                  | Yes          | partially implemented                      |
| eth_getBlockReceipts                       | Yes          | same as parity_getBlockReceipts            |
| eth_getTransactionReceiptsByBlockNumber    | -            | not yet implemented (eth_getBlockReceipts) |
| eth_getTransactionReceiptsByBlockHash      | -            | not yet implemented (eth_getBlockReceipts) |
|                                            |              |                                            |
| eth_estimateGas                            | Yes          |                                            |
| eth_getBalance                             | Yes          |                                            |
| eth_getCode                                | Yes          |                                            |
| eth_getTransactionCount                    | Yes          |                                            |
| eth_getStorageAt                           | Yes          |                                            |
| eth_call                                   | Yes          |                                            |
| eth_callMany                               | -            | not yet implemented                        |
| eth_callBundle                             | Yes          |                                            |
| eth_createAccessList                       | Yes          |                                            |
|                                            |              |                                            |
| eth_newFilter                              | -            | not yet implemented                        |
| eth_newBlockFilter                         | -            | not yet implemented                        |
| eth_newPendingTransactionFilter            | -            | not yet implemented                        |
| eth_getFilterChanges                       | -            | not yet implemented                        |
| eth_uninstallFilter                        | -            | not yet implemented                        |
| eth_getLogs                                | Yes          |                                            |
|                                            |              |                                            |
| eth_accounts                               | No           | deprecated                                 |
| eth_sendRawTransaction                     | Yes          | remote only                                |
| eth_sendTransaction                        | -            | not yet implemented                        |
| eth_sign                                   | No           | deprecated                                 |
| eth_signTransaction                        | -            | deprecated                                 |
| eth_signTypedData                          | -            | ????                                       |
|                                            |              |                                            |
| eth_getProof                               | -            | not yet implemented                        |
|                                            |              |                                            |
| eth_mining                                 | Yes          |                                            |
| eth_coinbase                               | Yes          |                                            |
| eth_hashrate                               | Yes          |                                            |
| eth_submitHashrate                         | Yes          |                                            |
| eth_getWork                                | Yes          |                                            |
| eth_submitWork                             | Yes          |                                            |
|                                            |              |                                            |
| eth_subscribe                              | -            | not yet implemented                        |
| eth_unsubscribe                            | -            | not yet implemented                        |
|                                            |              |                                            |
| engine_newPayloadV1                        | Yes          |                                            |
| engine_forkchoiceUpdatedV1                 | Yes          |                                            |
| engine_getPayloadV1                        | Yes          |                                            |
| engine_exchangeTransitionConfigurationV1   | Yes          |                                            |
|                                            |              |                                            |
| debug_accountRange                         | Yes          |                                            |
| debug_getModifiedAccountsByHash            | Yes          |                                            |
| debug_getModifiedAccountsByNumber          | Yes          |                                            |
| debug_storageRangeAt                       | Yes          |                                            |
| debug_traceBlockByHash                     | Yes          |                                            |
| debug_traceBlockByNumber                   | Yes          |                                            |
| debug_traceTransaction                     | Yes          |                                            |
| debug_traceCall                            | Yes          |                                            |
|                                            |              |                                            |
| trace_call                                 | Yes          |                                            |
| trace_callMany                             | Yes          |                                            |
| trace_rawTransaction                       | -            | not yet implemented                        |
| trace_replayBlockTransactions              | Yes          |                                            |
| trace_replayTransaction                    | Yes          |                                            |
| trace_block                                | Yes          |                                            |
| trace_filter                               | -            | not yet implemented                        |
| trace_get                                  | Yes          |                                            |
| trace_transaction                          | Yes          |                                            |
|                                            |              |                                            |
| txpool_content                             | Yes          |                                            |
| txpool_status                              | Yes          |                                            |
|                                            |              |                                            |
| eth_getCompilers                           | No           | deprecated                                 |
| eth_compileLLL                             | No           | deprecated                                 |
| eth_compileSolidity                        | No           | deprecated                                 |
| eth_compileSerpent                         | No           | deprecated                                 |
|                                            |              |                                            |
| db_putString                               | No           | deprecated                                 |
| db_getString                               | No           | deprecated                                 |
| db_putHex                                  | No           | deprecated                                 |
| db_getHex                                  | No           | deprecated                                 |
|                                            |              |                                            |
| shh_post                                   | No           | deprecated                                 |
| shh_version                                | No           | deprecated                                 |
| shh_newIdentity                            | No           | deprecated                                 |
| shh_hasIdentity                            | No           | deprecated                                 |
| shh_newGroup                               | No           | deprecated                                 |
| shh_addToGroup                             | No           | deprecated                                 |
| shh_newFilter                              | No           | deprecated                                 |
| shh_uninstallFilter                        | No           | deprecated                                 |
| shh_getFilterChanges                       | No           | deprecated                                 |
| shh_getMessages                            | No           | deprecated                                 |
|                                            |              |                                            |
| erigon_cumulativeChainTraffic              | -            | not yet implemented                        |
| erigon_getHeaderByHash                     | Yes          |                                            |
| erigon_getHeaderByNumber                   | Yes          |                                            |
| erigon_getBlockByTimestamp                 | Yes          |                                            |
| erigon_getBalanceChangesInBlock            | -            | not yet implemented                        |
| erigon_getLogsByHash                       | Yes          |                                            |
| erigon_forks                               | Yes          |                                            |
| erigon_issuance                            | Yes          |                                            |
| erigon_watchTheBurn                        | -            | not yet implemented (same as _issuance?)   |
| erigon_nodeInfo                            | -            | not yet implemented                        |
|                                            |              |                                            |
| starknet_call                              | -            | not yet implemented                        |
|                                            |              |                                            |
| bor_getSnapshot                            | -            | not yet implemented                        |
| bor_getAuthor                              | -            | not yet implemented                        |
| bor_getSnapshotAtHash                      | -            | not yet implemented                        |
| bor_getSigners                             | -            | not yet implemented                        |
| bor_getSignersAtHash                       | -            | not yet implemented                        |
| bor_getCurrentProposer                     | -            | not yet implemented                        |
| bor_getCurrentValidators                   | -            | not yet implemented                        |
| bor_getRootHash                            | -            | not yet implemented                        |
|                                            |              |                                            |
| parity_getBlockReceipts                    | Yes          | same as eth_getBlockReceipts               |
| parity_listStorageKeys                     | Yes          |                                            |
|                                            |              |                                            |
| ots_getApiLevel                            | -            | not yet implemented                        |
| ots_getInternalOperations                  | -            | not yet implemented                        |
| ots_searchTransactionsBefore               | -            | not yet implemented                        |
| ots_searchTransactionsAfter                | -            | not yet implemented                        |
| ots_getBlockDetails                        | -            | not yet implemented                        |
| ots_getBlockDetailsByHash                  | -            | not yet implemented                        |
| ots_getBlockTransactions                   | -            | not yet implemented                        |
| ots_hasCode                                | -            | not yet implemented                        |
| ots_traceTransaction                       | -            | not yet implemented                        |
| ots_getTransactionError                    | -            | not yet implemented                        |
| ots_getTransactionBySenderAndNonce         | -            | not yet implemented                        |
| ots_getContractCreator                     | -            | not yet implemented                        |

This table is constantly updated. Please visit again.
