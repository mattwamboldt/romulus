#pragma once
#include "common.h"

struct SpriteRenderUnit
{
    uint8 patternLoShift;
    uint8 patternHiShift;

    uint8 oamIndex;

    bool isEnabled;

    void tick();
    void reset();

    void setAttribute(uint8 value);
    void setX(uint8 value);
    uint8 getX();
    uint8 getPriority() { return priority; }
    bool isVerticallyFlipped() { return flipVertical; }
    bool isHorizontallyFlipped() { return flipHorizontal; }

    uint8 calculatePixel();

private:
    uint8 pallete;
    uint8 priority;
    uint8 xCounter;
    uint8 xPosition;

    bool flipHorizontal;
    bool flipVertical;

    uint8 currentPixel;
};
