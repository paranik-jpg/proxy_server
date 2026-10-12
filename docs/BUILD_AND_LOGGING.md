# Build System & Diagnostics Pipeline

## 1. CMake Target Profiles
* **Sanitizer Debug Build:**
  ```bash
  cmake -B build_asan -S . -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
  cmake --build build_asan -j$(nproc)
