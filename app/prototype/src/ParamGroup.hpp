#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include "GpioSet.h"

#ifdef PROTO
#define TITLE_ROW 3
#define POTS_ROW 0
#else
#define TITLE_ROW 0
#define POTS_ROW 1
#endif

byte mode0 = 0;
byte mode1 = 1;
byte mode2 = 2;

static const char *_assignMode[] = {"absolute", "relative"};

class ParamGroup
{
public:
    ParamGroup()
    {
    }

    void init(U8G2 *pU8g2)
    {
        _pU8g2 = pU8g2;
        _offsetX = 0;
        _maxWidth = 127;
        _height = 16;
        _frameHeight = 13;
    }

    void setMaxWidth(byte maxWidth)
    {
        _maxWidth = maxWidth;
    }

    void setOffset(byte offsetX)
    {
        _offsetX = offsetX;
    }

    void attachNames(const char *names[])
    {
        _pTitle = names[0];
        _pValueName[0] = names[1];
        _pValueName[1] = names[2];
        _pValueName[2] = names[3];
    }

    void attachValues(byte *values[3][4])
    {
        for (byte i = 0; i < 3; ++i)
        {
            _pValueItems[i] = values[i][0];
            _MinItems[i] = *values[i][1];
            _MaxItems[i] = *values[i][2];
            _DispMode[i] = *values[i][3];
        }
    }

    void dispParamGroup(uint16_t values[POTS_MAX])
    {
        static char disp_buf[20] = {0};
        _pU8g2->setFont(u8g2_font_6x13_tf);

        // pots
        for (byte i = 0; i < 3; ++i)
        {
            // setting label and values
            byte valueItem = (byte)(*_pValueItems[i]);
            switch (_DispMode[i])
            {
            case 0:
                strcpy(disp_buf, _pValueName[i]);
                break;
            case 1:
                sprintf(disp_buf, "%s:%03d", _pValueName[i], valueItem);
                break;
            case 2:
                sprintf(disp_buf, "%s:%s", _pValueName[i], _assignMode[valueItem]);
                break;
            
            default:
                strcpy(disp_buf, _pValueName[i]);
                break;
            }

            byte height = _height * (i + POTS_ROW);
            _pU8g2->drawFrame(_offsetX, height, _maxWidth, _frameHeight);
            _pU8g2->drawStr(_offsetX + 2, height, disp_buf);

            byte value = (byte)constrain(map(valueItem, _MinItems[i], _MaxItems[i], 1, _maxWidth - 1), 1, _maxWidth - 1);

            // value fill
            if (value > 1)
                _pU8g2->drawBox(_offsetX + 1, height + 1, value, _frameHeight - 2);

            // pots indicator
            uint16_t current = values[i];
            value = (byte)map(current, 0, POTS_MAX_VALUE, 1, _maxWidth - 2);
            byte x = _offsetX + value;
            byte y = height + _frameHeight;
            _pU8g2->drawTriangle(x, y - 1, x + 4, y + 3, x - 4, y + 3);
            _pU8g2->drawVLine(x, height + 1, _frameHeight - 2);
        }

    }

    void dispTitle()
    {
        static char disp_buf[20] = {0};
        // Setting title
        _pU8g2->setFont(u8g2_font_8x13B_tf);
        strcpy(disp_buf, _pTitle);
        _pU8g2->drawStr(0, _height * TITLE_ROW, disp_buf);
    }

    void dispTitle(byte index, const char *mapName)
    {
        static char disp_buf[20] = {0};
        // Setting title
        _pU8g2->setFont(u8g2_font_8x13B_tf);
        sprintf(disp_buf, "%s%d: %s", mapName, index, _pTitle);
        _pU8g2->drawStr(0, _height * TITLE_ROW, disp_buf);
    }

protected:
    U8G2 *_pU8g2;
    byte _offsetX;
    byte _maxWidth;
    byte _height;
    byte _frameHeight;
    byte *_pValueItems[3];
    byte _MinItems[3];
    byte _MaxItems[3];
    byte _DispMode[3];
    const char *_pTitle;
    const char *_pValueName[3];
};
