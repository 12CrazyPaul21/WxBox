# WxBox lua Plugin Template

​	这是一个编写WxBox lua动态库扩展的模板。这里动态库扩展指的是以dll/so形式发布的lua扩展，供在WxBox的lua扩展脚本中调用。

##  编译

```bash
# 在Visual Studio “x86”开发人员命令提示符执行以下命令
meson setup build/release --buildtype release
meson compile -C build/release
```

## 安装

```bash
# 会安装到plugins/libs目录下
meson install -C build\release --tags runtime
```

