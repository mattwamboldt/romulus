#pragma once
#include "../../romulus/platform.h"

// Reads data from xinput compatible devices and returns their states
// Doing this to implent directInput as well, since I want to support non standard controllers, like the sixaxis
// (I only have one 360 controller and I'm cheap...)

void LoadXInput();
void UpdateXInputState(InputState* inputState);