# SilkRPC Architecture

The context diagram of Silkrpc daemon is:

<img src="silkrpc-contextdiagram.png">

where the following interfaces are highlighted:

- **IF#1: TG gRPC KV / SM**: 
- **IF#2: ETH JSON RPC**: 

The Silkrpc component is decomposed into the following packages:

- **JSON RPC Server**: 
- **Session State**: 
- **State Read/Write**: 
- **KV Client**: 
- **Asynchronous Engine**: 

and internally uses **Silkworm**.

<img src="silkrpc-architecture.png">
