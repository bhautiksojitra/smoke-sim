# How to Build and Run

On all platforms: the code must be executed from the `src` directory. All files opened by your program will opened from that directory. Do **not** hard-code absolute paths. Use paths relative to `src`. Place all shaders and assets in `src`, or in subdirectories.

You can edit GLSL files in any text editor, but major source code editors like [Visual Studio Code](https://code.visualstudio.com) have GLSL modes available, with context-sensitive editing.

## Windows (Visual Studio)

Open the solution `opengl.sln` in [Visual Studio](https://visualstudio.microsoft.com). It has been tested using the free Community Edition. Build and run.

If you get errors about missing build tools, you may be running a different version of Visual Studio. Right-click on *Solution 'opengl'* and select `Retarget solution`. This should point to the correct build tools.

If you get errors about a missing Windows SDK, you will need to point to the version installed on your system. With the solution open, right-click on the project `opengl` (underneath *Solution 'opengl'*) in the Solution Explorer, and select `Properties`. In Configuration Properties / General, look for `Windows SDK Version` and use the drop-down to pick a version installed on your system.

To run a different example or your own code, expand the `Source Files` in the Solution Explorer, and delete `example0.cpp`. Using Windows Explorer or the command line, copy/move the desired CPP file to the `src` folder in the project. Add **that copy** to the project. Make sure you also copy all shaders and other assets to the `src` folder. Build and run.

Shader files don't need to be added to the project. You can edit them in Visual Studio, but it has no GLSL mode; an external editor may be better.

## Both macOS and Linux (command line)

Use the included Makefile to build the project. All build products are placed in the `build` directory.

You must run `make` from the src directory, and run the executables there too; for example, while located in the src directory:

```
make  
../build/example0
```

By default, the Makefile will find every CPP file whose name starts with the word `example` (like `example0.cpp`, `example1.cpp`, `example_test.cpp`), build an executable of the same name (like `example0`, `example1`, `example_test`), and place it in the build directory. Any CPP file in the src directory whose name does *not* begin with `example` will be compiled and linked to the executable.

You can edit the Makefile to change the pattern of CPP file names. Change `SRC_PREFIX` to be the (unique) prefix of the filenames you want to use for your executables. For example, if you have code in `a1q1.cpp` and `a1q2.cpp` then `SRC_PREFIX=a1q` will build executables `a1q1` and `a1q2` as described above. If you do this, all file names starting with `example` need to be removed, otherwise you will get linker errors!

## macOS

All dependencies are included.

The file `glew/lib/libGLEW.a` is a fat binary with both x64 and arm64 versions of the glew library. It was built on macOS Big Sur. If you are running an older macOS and it complains that the library is too new, replace it with `libGLEW.a-old` from the same directory.

## Linux

You must first install dependencies. On Debian or Ubuntu:

```
apt-get install libglu1-mesa-dev freeglut3-dev mesa-common-dev libglew-dev
```

Also, you must either install the `clang` package, or change `CC` in the Makefile to `g++`.

On my system `-DEXPERIMENTAL` needs to be added to `CFLAGS` to allow glew to expose "modern" OpenGL features. This may not be required on your system.
