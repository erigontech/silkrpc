## JSON library

This library was extracted from gRPC source. It is Apache licensed and not exposed by gRPC (it's internal).

In the future, we will either:

1) learn to use is as is from the gRPC source, or
2) alter it for our purposes for the daemon (it's Apache licensed which is why I'm including it here), or
3) find a different JSON library

### Changes made so far

1. cp -p build/_deps/grpc-src/src/core/lib/json/* .
2. Changed code filenames from .cc to .cpp
3. Changed header filenames from .h to .hpp
4. Removed the code in json_utils.h (not needed)
