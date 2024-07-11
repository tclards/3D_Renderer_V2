I created this 3D renderer using C++, DirectX, and HLSL last year as part of a school project. I then spent significant time on my own upgrading and refining it. It was also used as the backbone of a school game project written entirely in C++ without an engine.

This project made use of a free-use 3rd party library called Gateware, developed and provided by professors at Full Sail University. I intend to finish removing this dependency eventually, as it is mostly used for matrix math (and a few draw functions) and I have created my own matrix math library to replace it with.

Build with cmake using "cmake -S ./ -B ./build"

/*
Debug Keys:
Num Pad 1 - Toggle Debug Free Cams
Num Pad 2 - Toggle Splitscreen (DISABLED)
Num Pad 3 - Toggle WireFrame mode
Num Pad 6 - Toggle Orthographic Projection Mode
F1 - Load Next Level
Num Pad 7 & 8 - Change Health
Num Pad 4 & 5 - Change Ammo

FreeCam Controls :
Keyboard -
Q/E - Rotate Camera Orientation
W/A/S/D - Move Camera
Mouse - Look Around
L Shift/Space - Move up and down

Controller -
A/B - Rotate Camera Orientation
Left Stick - Move Camera
Right Stick - Look Around
Right Bumper - Load Next Level
Left/Right Trigger - move up and down
*/
