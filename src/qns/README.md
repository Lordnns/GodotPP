Build extension:

cmake --build build --target editor --config Debug

Build core:

cmake --build build --target QNSCore --config Debug


\
\
\
\


cmake --build build --target editor

This is a multi-config generator; CMAKE_BUILD_TYPE is usually empty at configure time. That means Rust might always build debug unless you explicitly pass config.

So for Release you should do:

cmake --build build --target editor --config Release