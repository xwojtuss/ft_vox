export SDKROOT=$(xcrun --show-sdk-path)
LLVM=$(brew --prefix llvm)

cmake --preset dev-clang \
  -DCMAKE_C_COMPILER=$LLVM/bin/clang \
  -DCMAKE_CXX_COMPILER=$LLVM/bin/clang++ \
  -DCMAKE_OSX_SYSROOT=$SDKROOT \
  -DCMAKE_EXE_LINKER_FLAGS="-L$LLVM/lib/c++ -Wl,-rpath,$LLVM/lib/c++"

cmake --build --preset dev-clang