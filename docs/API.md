## RPC API Implementation Status

The following table shows the current [JSON RPC API](https://eth.wiki/json-rpc/API) implementation status in `Silkrpc`.

| Command                                 | Availability | Notes                                      |
| :-------------------------------------- | :----------: | -----------------------------------------: |
| web3_clientVersion                      | Yes          | hard-coded (needs ethbackend integration)  |
| web3_sha3                               | Yes          |                                            |
|                                         |              |                                            |
| net_listening                           | Yes          | hard-coded (needs p2pSentry integration)   |
| net_peerCount                           | Yes          | hard-coded (needs p2pSentry integration)   |
| net_version                             | Yes          | hard-coded (needs ethbackend integration)  |
|                                         |              |                                            |
| eth_blockNumber                         | Yes          |                                            |
| eth_chainId                             | Yes          |                                            |
| eth_protocolVersion                     | Yes          |                                            |
| eth_syncing                             | Yes          |                                            |
| eth_gasPrice                            | -            | not yet implemented                        |
|                                         |              |                                            |
| eth_getBlockByHash                      | Yes          | not yet tested for performance             |
| eth_getBlockByNumber                    | Yes          | not yet tested for performance             |
| eth_getBlockTransactionCountByHash      | Yes          |                                            |
| eth_getBlockTransactionCountByNumber    | Yes          |                                            |
| eth_getUncleByBlockHashAndIndex         | Yes          |                                            |
| eth_getUncleByBlockNumberAndIndex       | Yes          |                                            |
| eth_getUncleCountByBlockHash            | Yes          |                                            |
| eth_getUncleCountByBlockNumber          | Yes          |                                            |
| eth_getBlockReceipts                    | Yes          | same as parity_getBlockReceipts            |
|                                         |              |                                            |
| eth_getTransactionByHash                | Yes          | partially implemented                      |
| eth_getTransactionByBlockHashAndIndex   | Yes          |                                            |
| eth_getTransactionByBlockNumberAndIndex | Yes          |                                            |
| eth_getTransactionReceipt               | Yes          | partially implemented                      |
|                                         |              |                                            |
| eth_estimateGas                         | -            | not yet implemented                        |
| eth_getBalance                          | Yes          |                                            |
| eth_getCode                             | Yes          |                                            |
| eth_getTransactionCount                 | Yes          |                                            |
| eth_getStorageAt                        | Yes          |                                            |
| eth_call                                | -            | work in progress                           |
|                                         |              |                                            |
| eth_newFilter                           | -            | not yet implemented                        |
| eth_newBlockFilter                      | -            | not yet implemented                        |
| eth_newPendingTransactionFilter         | -            | not yet implemented                        |
| eth_getFilterChanges                    | -            | not yet implemented                        |
| eth_uninstallFilter                     | -            | not yet implemented                        |
| eth_getLogs                             | Yes          |                                            |
|                                         |              |                                            |
| eth_accounts                            | No           | deprecated                                 |
| eth_sendRawTransaction                  | -            | not yet implemented, remote only           |
| eth_sendTransaction                     | -            | not yet implemented                        |
| eth_sign                                | No           | deprecated                                 |
| eth_signTransaction                     | -            | not yet implemented                        |
| eth_signTypedData                       | -            | ????                                       |
|                                         |              |                                            |
| eth_getProof                            | -            | not yet implemented                        |
|                                         |              |                                            |
| eth_mining                              | -            | not yet implemented                        |
| eth_coinbase                            | Yes          |                                            |
| eth_hashrate                            | -            | not yet implemented                        |
| eth_submitHashrate                      | -            | not yet implemented                        |
| eth_getWork                             | -            | not yet implemented                        |
| eth_submitWork                          | -            | not yet implemented                        |
|                                         |              |                                            |
| eth_subscribe                           | -            | not yet implemented                        |
| eth_unsubscribe                         | -            | not yet implemented                        |
|                                         |              |                                            |
| debug_accountRange                      | -            | not yet implemented                        |
| debug_getModifiedAccountsByNumber       | -            | not yet implemented                        |
| debug_getModifiedAccountsByHash         | -            | not yet implemented                        |
| debug_storageRangeAt                    | -            | not yet implemented                        |
| debug_traceTransaction                  | -            | not yet implemented                        |
| debug_traceCall                         | -            | not yet implemented                        |
|                                         |              |                                            |
| trace_call                              | -            | not yet implemented                        |
| trace_callMany                          | -            | not yet implemented                        |
| trace_rawTransaction                    | -            | not yet implemented                        |
| trace_replayBlockTransactions           | -            | not yet implemented                        |
| trace_replayTransaction                 | -            | not yet implemented                        |
| trace_block                             | -            | not yet implemented                        |
| trace_filter                            | -            | not yet implemented                        |
| trace_get                               | -            | not yet implemented                        |
| trace_transaction                       | -            | not yet implemented                        |
|                                         |              |                                            |
| eth_getCompilers                        | No           | deprecated                                 |
| eth_compileLLL                          | No           | deprecated                                 |
| eth_compileSolidity                     | No           | deprecated                                 |
| eth_compileSerpent                      | No           | deprecated                                 |
|                                         |              |                                            |
| db_putString                            | No           | deprecated                                 |
| db_getString                            | No           | deprecated                                 |
| db_putHex                               | No           | deprecated                                 |
| db_getHex                               | No           | deprecated                                 |
|                                         |              |                                            |
| shh_post                                | No           | deprecated                                 |
| shh_version                             | No           | deprecated                                 |
| shh_newIdentity                         | No           | deprecated                                 |
| shh_hasIdentity                         | No           | deprecated                                 |
| shh_newGroup                            | No           | deprecated                                 |
| shh_addToGroup                          | No           | deprecated                                 |
| shh_newFilter                           | No           | deprecated                                 |
| shh_uninstallFilter                     | No           | deprecated                                 |
| shh_getFilterChanges                    | No           | deprecated                                 |
| shh_getMessages                         | No           | deprecated                                 |
|                                         |              |                                            |
| tg_getHeaderByHash                      | Yes          |                                            |
| tg_getHeaderByNumber                    | Yes          |                                            |
| tg_getLogsByHash                        | Yes          |                                            |
| tg_forks                                | Yes          |                                            |
| tg_issuance                             | Yes          |                                            |
|                                         |              |                                            |
| parity_getBlockReceipts                 | Yes          | same as eth_getBlockReceipts               |

This table is constantly updated. Please visit again.

