To build/run openGL assignments using Visual Studio in Windows:

Open folder "Assignment_X" in Visual Studio.
Open a terminal and type the following commands:

**mkdir build**

**cd build**

**cmake -G "Visual Studio 17 2022" -A x64 ..**

This builds the solution in the /build directory.
In the Visual Studio File navigation go to the solution and double click it to open it.

Once you're in the solution right click on "assignment_XX_copy_shader" and click "build".
This will copy the shaders into the /bin directory.

After this set the middle drop-down to "Release" instead of "Debug" and click the play button.
