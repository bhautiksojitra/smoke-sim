

# How to Build and Run

On all platforms: the code must be executed from the `src` directory. All files opened by your program will opened from that directory. Do **not** hard-code absolute paths. Use paths relative to `src`. Place all shaders and assets in `src`, or in subdirectories.

You can edit GLSL files in any text editor, but major source code editors like [Visual Studio Code](https://code.visualstudio.com) have GLSL modes available, with context-sensitive editing.

## Windows (Visual Studio)

Open the solution `opengl.sln` in [Visual Studio](https://visualstudio.microsoft.com). It has been tested using the free Community Edition. Build and run.

If you get errors about missing build tools, you may be running a different version of Visual Studio. Right-click on *Solution 'opengl'* and select `Retarget solution`. This should point to the correct build tools.

If you get errors about a missing Windows SDK, you will need to point to the version installed on your system. With the solution open, right-click on the project `opengl` (underneath *Solution 'opengl'*) in the Solution Explorer, and select `Properties`. In Configuration Properties / General, look for `Windows SDK Version` and use the drop-down to pick a version installed on your system.

To run a different example or your own code, expand the `Source Files` in the Solution Explorer, and delete `example0.cpp`. Using Windows Explorer or the command line, copy/move the desired CPP file to the `src` folder in the project. Add **that copy** to the project. Make sure you also copy all shaders and other assets to the `src` folder. Build and run.

Shader files don't need to be added to the project. You can edit them in Visual Studio, but it has no GLSL mode; an external editor may be better.
