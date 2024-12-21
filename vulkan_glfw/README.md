Compile:

```shell
$ cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$VCPKG_HOME/scripts/buildsystems/vcpkg.cmake

$ cmake --build build --config Release
```

On OSX this requires the installed Vulkan SDK - otherwise the initialization will fail.

To debug initialization, add envvar:

```shell
VK_LOADER_DEBUG=all
```