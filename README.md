# WxBox

## 第三方依赖

- gRPC
- protobuf
- frida-gum
- Qt5
- spdlog
- meson+ninja
- cmake
- pkg-config
- lua

## 构建方法

​	首先需要安装meson+ninja构建工具，可以到以下路径下载：https://github.com/mesonbuild/meson/releases。

​	spdbuf需要cmake构建工具。

### Qt SDK环境路径

​	在Windows中要把qmake的路径加入Path中，否则meson识别不到Qt5

```bash
# 下面是一个msvc2017构建kit路径的例子，注意必须是32位的版本
F:\Qt\Qt5.14.2\5.14.2\msvc2017\bin
```

​	Qt SDK下载路径：https://download.qt.io/archive/qt/，官方镜像列表：https://download.qt.io/static/mirrorlist/，尽量使用Qt 5.11~ Qt 5.15版本的SDK。

#### Qt依赖处理

​	在Windows下使用windeployqt，Mac OS下使用macdeployqt，这两个都是Qt官方的工具。

### lua wrap补丁

​	meson官方的lua wrap生成的是动态库，但是我们需要的是静态库，所以对meson官方的lua_5.4.3-1 wrap打了个补丁，修改为生成静态库。这个直接以zip包方式放在仓库上，wrap下回来的包使用的是这个。

​	

```bash
# 打补丁的位置是lua-5.4.3/meson.build
# 补丁内容如下：
index a9c4d56..2004cfb 100644
--- a/meson_original.build
+++ b/meson.build
@@ -49,7 +49,7 @@ if get_option('line_editing')
 endif

 # Targets.
-lua_lib = library('lua',
+lua_lib = static_library('lua',
   'src/lapi.c',
   'src/lauxlib.c',
   'src/lbaselib.c',
@@ -83,8 +83,6 @@ lua_lib = library('lua',
   'src/lvm.c',
   'src/lzio.c',
   dependencies: lua_lib_deps,
-  version: meson.project_version(),
-  soversion: lua_versions[0] + '.' + lua_versions[1],
```



### 构建依赖

​	由于只有gRPC构建出来比较麻烦，所以这里只记录gRPC的构建方法。另外在编译gRPC的时候protobuf也一起编译了，所以不需要另外编译protobuf了。

#### gRPC构建方法

​	如果在Mac OS环境下能用brew安装gRPC的dev包，那么就不需要，对于Windows已经编译好了一份Debug和一份Release版本的了，它们将放在仓库上，并且已经给gRPC写了一个meson的wrap，在使用meson构建时将会自动下载。

```bash
# 克隆gRPC
git clone --recurse-submodules -b v1.43.0 https://github.com/grpc/grpc

# 如果克隆下来后因为网络原因导致一些submodules未下回来，可以先把grpc克隆回来，然后后面再用下面的命令来把submodule拉回来
git submodule init
git submodule update

# 对于Windows 需要下载依赖
sudo choco install nasm

# 对于Mac OS 需要下载以下依赖
brew install autoconf automake libtool pkg-config

# 另外注意，这是用cmake来构建configure的所以cmake在两个平台下都需要

# Windows环境下编译gRPC Debug版本（先切到gRPC仓库的根目录后执行下面命令）
mkdir -p cmake/build/debug & cd cmake/build/debug
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug -DgRPC_BUILD_TESTS=OFF -DgRPC_INSTALL=ON -DgRPC_BUILD_GRPC_CSHARP_PLUGIN=OFF -DgRPC_BUILD_GRPC_NODE_PLUGIN=OFF -DgRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN=OFF -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF -DgRPC_BUILD_GRPC_PYTHON_PLUGIN=OFF -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF -DgRPC_BUILD_CSHARP_EXT=OFF -DCMAKE_INSTALL_PREFIX=../../install/debug ../..
nmake
nmake install

# Windows环境下编译gRPC Release版本（先切到gRPC仓库的根目录后执行下面命令）
mkdir -p cmake/build/debug & cd cmake/build/debug
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DgRPC_BUILD_TESTS=OFF -DgRPC_INSTALL=ON -DgRPC_BUILD_GRPC_CSHARP_PLUGIN=OFF -DgRPC_BUILD_GRPC_NODE_PLUGIN=OFF -DgRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN=OFF -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF -DgRPC_BUILD_GRPC_PYTHON_PLUGIN=OFF -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF -DgRPC_BUILD_CSHARP_EXT=OFF -DCMAKE_INSTALL_PREFIX=../../install/release ../..
nmake
nmake install

# Mac OS环境下编译gRPC Debug版本（先切到gRPC仓库的根目录后执行下面命令）
mkdir -p cmake/build/debug & cd cmake/build/debug
cmake -G -DCMAKE_BUILD_TYPE=Debug -DgRPC_BUILD_TESTS=OFF -DgRPC_INSTALL=ON -DgRPC_BUILD_GRPC_CSHARP_PLUGIN=OFF -DgRPC_BUILD_GRPC_NODE_PLUGIN=OFF -DgRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN=OFF -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF -DgRPC_BUILD_GRPC_PYTHON_PLUGIN=OFF -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF -DgRPC_BUILD_CSHARP_EXT=OFF -DCMAKE_INSTALL_PREFIX=../../install/debug ../..
make -j
make install

# Mac OS环境下编译gRPC Release版本（先切到gRPC仓库的根目录后执行下面命令）
mkdir -p cmake/build/debug & cd cmake/build/debug
cmake -DCMAKE_BUILD_TYPE=Release -DgRPC_BUILD_TESTS=OFF -DgRPC_INSTALL=ON -DgRPC_BUILD_GRPC_CSHARP_PLUGIN=OFF -DgRPC_BUILD_GRPC_NODE_PLUGIN=OFF -DgRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN=OFF -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF -DgRPC_BUILD_GRPC_PYTHON_PLUGIN=OFF -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF -DgRPC_BUILD_CSHARP_EXT=OFF -DCMAKE_INSTALL_PREFIX=../../install/release ../..
make
make install
```

##### gRPC引用注意

​	gRPC在install后是会打包好给pkg-config用的pc文件的，但是由于生成的lib文件非常的多，执行一次pkg-config搜索非常的慢，所以项目中用的方法是，调用一次pkg-config然后把参数记录下来，直接写进meson的wrap中。

```bash
# 下面是在Windows环境下执行的pkg-config，PKG_CONFIG_PATH需要根据实际情况修改
PKG_CONFIG_PATH=/g/Tutorial/meson/testgrpc/grpc/lib/pkgconfig ./pkg-config.exe grpc++ --cflags --libs

# 结果如下：
/W3 /DNOMINMAX /DWIN32_LEAN_AND_MEAN /D_CRT_SECURE_NO_WARNINGS /D_SCL_SECURE_NO_WARNINGS /D_ENABLE_EXTENDED_ALIGNED_STORAGE /bigobj /wd4005 /wd4068 /wd4180 /wd4244 /wd4267 /wd4503 /wd4800 -IG:/Tutorial/meson/testgrpc/grpc/include  -ignore:4221 -LG:/Tutorial/meson/testgrpc/grpc/lib -lgrpc++ -lgrpc -laddress_sorting -lre2 -lupb -lcares -lz -lgpr -labsl_statusor -lssl -lcrypto -labsl_hash -labsl_raw_hash_set -labsl_hashtablez_sampler -labsl_city -labsl_low_level_hash -labsl_random_distributions -labsl_random_seed_sequences -labsl_random_internal_pool_urbg -labsl_random_internal_randen -labsl_random_seed_gen_exception -labsl_random_internal_randen_hwaes -labsl_random_internal_randen_slow -labsl_random_internal_randen_hwaes_impl -labsl_random_internal_platform -labsl_random_internal_seed_material -labsl_status -labsl_cord -labsl_cordz_info -labsl_cord_internal -labsl_cordz_functions -labsl_cordz_handle -labsl_exponential_biased -labsl_synchronization -labsl_bad_optional_access -labsl_str_format_internal -labsl_graphcycles_internal -labsl_stacktrace -labsl_symbolize -labsl_time -labsl_debugging_internal -labsl_demangle_internal -labsl_malloc_internal -labsl_civil_time -labsl_strings -labsl_time_zone -labsl_strings_internal -labsl_int128 -labsl_throw_delegate -labsl_base -labsl_spinlock_wait -labsl_bad_variant_access -labsl_raw_logging_internal -labsl_log_severity

# 处理过之后编译参数整理为：
['/DNOMINMAX', '/DWIN32_LEAN_AND_MEAN', '/D_CRT_SECURE_NO_WARNINGS', '/D_SCL_SECURE_NO_WARNINGS', '/D_ENABLE_EXTENDED_ALIGNED_STORAGE', '/bigobj', '/wd4005', '/wd4068', '/wd4180', '/wd4244', '/wd4267', '/wd4503', '/wd4800']

# 依赖的静态库文件整理如下（这里还实际调试把漏掉的补上了）：
['grpc++', 'grpc', 'address_sorting', 're2', 'upb', 'cares', 'zlib', 'gpr', 'absl_statusor', 'ssl', 'crypto', 'absl_hash', 'absl_raw_hash_set', 'absl_hashtablez_sampler', 'absl_city', 'absl_low_level_hash', 'absl_random_distributions', 'absl_random_seed_sequences', 'absl_random_internal_pool_urbg', 'absl_random_internal_randen', 'absl_random_seed_gen_exception', 'absl_random_internal_randen_hwaes', 'absl_random_internal_randen_slow', 'absl_random_internal_randen_hwaes_impl', 'absl_random_internal_platform', 'absl_random_internal_seed_material', 'absl_status', 'absl_cord', 'absl_cordz_info', 'absl_cord_internal', 'absl_cordz_functions', 'absl_cordz_handle', 'absl_exponential_biased', 'absl_synchronization', 'absl_bad_optional_access', 'absl_str_format_internal', 'absl_graphcycles_internal', 'absl_stacktrace', 'absl_symbolize', 'absl_time', 'absl_debugging_internal', 'absl_demangle_internal', 'absl_malloc_internal', 'absl_civil_time', 'absl_strings', 'absl_time_zone', 'absl_strings_internal', 'absl_int128', 'absl_throw_delegate', 'absl_base', 'absl_spinlock_wait', 'absl_bad_variant_access', 'absl_raw_logging_internal', 'absl_log_severity', 'libprotobuf', 'grpc++_reflection']
```



### Windows下构建

```bash
# 在Visual Studio “x86”开发人员命令提示符执行以下命令
meson setup build/release --buildtype release
meson compile -C build/release
```

### Mac OS下构建

​	note: 暂时还不清楚

## 开发时的调试方法

​	开发调试不使用ninja作为backend，也不使用vscode作为编辑器，而是切为IDE，在Windows下用Visual Studio，在Mac OS下用xcode。

### Windows下使用Visual Studio IDE来开发并调试

```bash
# 以Visual Studio为backend创建工程
meson setup build/vsdebug --buildtype debug --backend vs

# 构建vs工程完成后，就可以打开build/vsdebug/WxBox.sln工程，然后把“wxbox@exe”修改为启动项目，接着就可以开发调试了
```

### Mac OS下使用xcode IDE来开发并调试

```bash
# 以xcode为backend创建工程
meson setup build/xcodedebug --buildtype debug --backend xcode
```



## install与打包方法

​	先不考虑，开发得差不多再说。
