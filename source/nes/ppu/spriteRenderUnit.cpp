#include "spriteRenderUnit.h"

void SpriteRenderUnit::tick()
{
    if (!isEnabled)
    {
        return;
    }

    if (xCounter > 0)
    {
        --xCounter;
        return;
    }

    ++currentPixel;
    if (currentPixel >= 8)
    {
        reset();
        return;
    }

    if (flipHorizontal)
    {
        patternLoShift >>= 1;
        patternHiShift >>= 1;
    }
    else
    {
        patternLoShift <<= 1;
        patternHiShift <<= 1;
    }
}

void SpriteRenderUnit::reset()
{
    isEnabled = false;
    setAttribute(0);
    setX(0);
    patternLoShift = 0;
    patternHiShift = 0;
    currentPixel = 0;
    oamIndex = 0;
}

void SpriteRenderUnit::setX(uint8 value)
{
    xCounter = xPosition = value;
}

uint8 SpriteRenderUnit::getX()
{
    return xPosition;
}

void SpriteRenderUnit::setAttribute(uint8 attribute)
{
    pallete = (attribute & 0x03) << 2;
    priority = (attribute & BIT_5) >> 5;
    flipHorizontal = (attribute & BIT_6) > 0;
    flipVertical = (attribute & BIT_7) > 0;
}

uint8 SpriteRenderUnit::calculatePixel()
{
    if (!isEnabled || xCounter > 0)
    {
        return 0;
    }

    uint8 bit0 = 0;
    uint8 bit1 = 0;

    if (flipHorizontal)
    {
        bit0 = patternLoShift & BIT_0;
        bit1 = (patternHiShift & BIT_0) << 1;
    }
    else
    {
        bit0 = (patternLoShift & BIT_7) >> 7;
        bit1 = (patternHiShift & BIT_7) >> 6;
    }

    return BIT_4 | pallete | bit1 | bit0;
}
