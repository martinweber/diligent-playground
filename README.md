# Diligent Playground

This repository is my playground to work with
[DiligentCore](https://github.com/DiligentGraphics/DiligentCore).

I have my own
[fork](https://github.com/martinweber/DiligentCore/tree/cmake_package_config) of DiligentCore that has some changes that allow to create a CMake package
that can be used instead of `add_subdirectory()`.

So far I only work on Windows using Visual Studio 2022.

## Dependencies

- [g3log](https://github.com/KjellKod/g3log) for logging
- [GLFW](https://www.glfw.org/) for window management
- DiligentCore

## Build

Configure using:

```shell
cmake -B build
```

Then open the Visual Studio solution in the `build` folder.

Make sure to run the `Copy_Shaders` target once before running the
application. This will copy the shader files to the executable output
folder.
