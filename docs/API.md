## RPC API Implementation Status

The following table shows the current [JSON RPC API](https://eth.wiki/json-rpc/API) implementation status in `Silkrpc`.

| Command                                    | Availability | Notes                                      |
| :------------------------------------------| :----------: | -----------------------------------------: |
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
| eth_getRawTransactionByHash                | -            | not yet implemented                        |
| eth_getTransactionByBlockHashAndIndex      | Yes          |                                            |
| eth_getRawTransactionByBlockHashAndIndex   | -            | not yet implemented                        |
| eth_getTransactionByBlockNumberAndIndex    | Yes          |                                            |
| eth_getRawTransactionByBlockNumberAndIndex | -            | not yet implemented                        |
| eth_getTransactionReceipt                  | Yes          | partially implemented                      |
| eth_getBlockReceipts                       | Yes          | same as parity_getBlockReceipts            |
|                                            |              |                                            |
| eth_estimateGas                            | Yes          |                                            |
| eth_getBalance                             | Yes          |                                            |
| eth_getCode                                | Yes          |                                            |
| eth_getTransactionCount                    | Yes          |                                            |
| eth_getStorageAt                           | Yes          |                                            |
| eth_call                                   | Yes          |                                            |
| eth_callBundle                             | -            | not yet implemented                        |
| eth_createAccessList                       | -            | not yet implemented                        |
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
| engine_forkchoiceUpdatedV1                 | -            | not yet implemented                        |
| engine_getPayloadV1                        | Yes          |                                            |
| engine_exchangeTransitionConfigurationV1   | -            | not yet implemented                        |
|                                            |              |                                            |
| debug_accountRange                         | Yes          |                                            |
| debug_getModifiedAccountsByHash            | Yes          |                                            |
| debug_getModifiedAccountsByNumber          | Yes          |                                            |
| debug_storageRangeAt                       | Yes          |                                            |
| debug_traceBlockByHash                     | -            | not yet implemented                        |
| debug_traceBlockByNumber                   | -            | not yet implemented                        |
| debug_traceTransaction                     | Yes          |                                            |
| debug_traceCall                            | Yes          |                                            |
|                                            |              |                                            |
| trace_call                                 | -            | not yet implemented                        |
| trace_callMany                             | -            | not yet implemented                        |
| trace_rawTransaction                       | -            | not yet implemented                        |
| trace_replayBlockTransactions              | -            | not yet implemented                        |
| trace_replayTransaction                    | -            | not yet implemented                        |
| trace_block                                | -            | not yet implemented                        |
| trace_filter                               | -            | not yet implemented                        |
| trace_get                                  | -            | not yet implemented                        |
| trace_transaction                          | -            | not yet implemented                        |
|                                            |              |                                            |
| txpool_content                             | -            | not yet implemented                        |
| txpool_status                              | -            | not yet implemented                        |
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
| tg_getHeaderByHash                         | Yes          |                                            |
| tg_getHeaderByNumber                       | Yes          |                                            |
| tg_getLogsByHash                           | Yes          |                                            |
| tg_forks                                   | Yes          |                                            |
| tg_issuance                                | Yes          |                                            |
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

This table is constantly updated. Please visit again.
