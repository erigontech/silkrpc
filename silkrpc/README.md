# SilkRPC

see the [README](../README.md) for more information

To test:

In one terminal window:

```
cd build
make
./silkrpc/silkrpc
```

In another terminal window:

```
cd build
watch -n .2 ./cmd/test_client
```

You should see the client continually hitting the server.
