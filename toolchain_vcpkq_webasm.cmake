include("/home/simon/src/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")

set(VCPKG_TARGET_TRIPLET "wasm32-emscripten" CACHE STRING "")

include("/home/simon/.local/share/vcpkg/scripts/buildsystems/vcpkg.cmake")