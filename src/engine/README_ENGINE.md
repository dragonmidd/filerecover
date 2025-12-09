Engine 模块（开发说明）

位置: `src/engine`

包含:
- `include/fr.h`：C API 头文件草案
- `src/fr_stub.cpp`：引擎 stub 实现（供单元测试与早期集成使用）
- `CMakeLists.txt`：用于构建引擎与单元测试的 CMake 配置

快速构建（在有 CMake、git、C++ 编译器环境下）:

```powershell
mkdir build; cd build
cmake ..\engine -DBUILD_ENGINE_TESTS=ON
cmake --build . --config Release
ctest -C Release --output-on-failure
```

后续工作:
- 用真实的 DiskIO、NTFS/ exFAT 解析器替换 stub
- 定义更完整的错误码与日志系统
- 补充 fr.h 中的数据结构（例如碎片信息、时间戳、原始元数据）
