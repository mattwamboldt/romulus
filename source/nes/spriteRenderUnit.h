#pragma once
#include "../common.h"

struct SpriteRenderUnit
{
    uint8 patternLoShift;
    uint8 patternHiShift;
    uint8 xCounter;

    bool isEnabled;

    void tick();
    void reset();

    void setAttribute(uint8 value);
    uint8 getPriority() { return priority; }
    bool isVerticallyFlipped() { return flipVertical; }
    bool isHorizontallyFlipped() { return flipHorizontal; }

    uint8 calculatePixel();

private:
    uint8 pallete;
    uint8 priority;
    bool flipHorizontal;
    bool flipVertical;

    uint8 currentPixel;
};
