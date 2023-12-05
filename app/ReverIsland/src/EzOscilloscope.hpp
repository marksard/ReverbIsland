/*!
 * EzOscilloscope class
 * Copyright 2023 marksard
 * Referenced source: http://radiopench.blog96.fc2.com/blog-entry-890.html
 * This software is released under the MIT license.
 * see https://opensource.org/licenses/MIT
 */

#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include "SmoothAnalogRead.hpp"

#define DATA_BIT 12
#define DATA_MAX_VALUE 4095
#define DATA_MAX_VALUE_F 4095.0
#define DATA_MAX_VOLT 5.0
#define DATA_BUF_MAX 200
#define DATA_BUF_HALF 100
#define SCAN_DELAY_MAX 12800

#define FRM_LFT 26
#define FRM_RGT 127
#define FRM_TOP 9
#define FRM_BTM 47
#define FRM_CTR (FRM_TOP + ((FRM_BTM - FRM_TOP) >> 1))
#define FRM_LEN 4

class EzOscilloscope
{
public:
    EzOscilloscope(U8G2 *pU8g2, SmoothAnalogRead *pCv, byte dispOffsetTop)
    {
        init(pU8g2, pCv, dispOffsetTop);
    }

    EzOscilloscope()
    {
        init(NULL, NULL, 0);
    }

    void init(U8G2 *pU8g2, SmoothAnalogRead *pCv, byte dispOffsetTop)
    {
        _pU8g2 = pU8g2;
        _pCv = pCv;
        _delay = 100;
        _dataAve = 0;
        _rangeMax = DATA_MAX_VALUE;
        _rangeMin = 0;
        _triggerPoint = 10;
        _left = 0;
        _top = dispOffsetTop;
        clearDataBuff();
    }

    void play()
    {
        byte drawLastIndex = DATA_BUF_HALF;
        if (_delay >= SCAN_DELAY_MAX)
        {
            drawLastIndex = readDataLong();
            calcDataLong();
        }
        else
        {
            readData();
            calcData();
        }

        _pU8g2->clearBuffer();
        drawFrame();
        drawData(drawLastIndex);
        drawString();
        _pU8g2->sendBuffer();
    }

    void incDelay()
    {
        _delay = constrainCyclic((int)(_delay << 1), 25, SCAN_DELAY_MAX);
    }

    void decDelay()
    {
        _delay = constrainCyclic((int)(_delay >> 1), 25, SCAN_DELAY_MAX);
    }

protected:
    float _convertVoltCoff = DATA_MAX_VOLT / DATA_MAX_VALUE_F;
    U8G2 *_pU8g2;
    SmoothAnalogRead *_pCv;
    int16_t _delay;
    int16_t _dataBuff[DATA_BUF_MAX];
    int16_t _dataAve;
    int16_t _rangeMax;
    int16_t _rangeMin;
    int16_t _triggerPoint;
    byte _left;
    byte _top;

private:
    template <typename su = uint8_t>
    su constrainCyclic(su value, su min, su max)
    {
        if (value > max)
            return min;
        if (value < min)
            return max;
        return value;
    }

    void clearDataBuff()
    {
        for (byte i = 0; i < DATA_BUF_MAX; ++i)
        {
            _dataBuff[i] = 0;
        }
    }

    byte readDataLong()
    {
        static byte index = 0;
        _dataBuff[index] = _pCv->analogRead(false);
        index++;
        // 線描画の終端の関係で+1
        if (index >= DATA_BUF_HALF)
        {
            clearDataBuff();
            index = 0;
        }

        return index;
    }

    void calcDataLong()
    {
        _rangeMax = DATA_MAX_VALUE;
        _rangeMin = 0;
        int16_t tmp = 0;
        long sum = 0;
        for (byte i = 0; i < DATA_BUF_MAX; ++i)
        {
            tmp = _dataBuff[i];
            sum += tmp;
        }

        _dataAve = sum / DATA_BUF_MAX;

        _triggerPoint = 10;
    }

    void readData()
    {
        for (byte i = 0; i < DATA_BUF_MAX; ++i)
        {
            _dataBuff[i] = _pCv->analogRead(false);
            delayMicroseconds(_delay);
        }
    }

    void calcData()
    {
        int16_t dataMin = DATA_MAX_VALUE;
        int16_t dataMax = 0;
        int16_t tmp = 0;
        long sum = 0;
        for (byte i = 0; i < DATA_BUF_MAX; ++i)
        {
            tmp = _dataBuff[i];
            sum += tmp;
            dataMin = min(dataMin, tmp);
            dataMax = max(dataMax, tmp);
        }

        _dataAve = sum / DATA_BUF_MAX;

        // データ表示範囲を最大±10拡大
        _rangeMin = dataMin - 20;
        _rangeMin = max((_rangeMin / 10) * 10, 0);
        _rangeMax = dataMax + 20;
        _rangeMax = min((_rangeMax / 10) * 10, DATA_MAX_VALUE);

        // 9～109の範囲でデータ表示範囲値からの立ち上がりを探す
        // なければ10
        _triggerPoint = 10;
        long triggerRange = (dataMax + dataMin) >> 1;
        for (byte i = 9; i < 110; ++i)
        {
            if ((_dataBuff[i - 1] < triggerRange) &&
                (_dataBuff[i] >= triggerRange))
            {
                _triggerPoint = i;
                break;
            }
        }
    }

    void drawFrame()
    {
        _pU8g2->setFont(u8g2_font_5x8_tf);
        _pU8g2->drawVLine(_left + FRM_LFT, _top + FRM_TOP, FRM_LEN);
        _pU8g2->drawVLine(_left + FRM_LFT, _top + FRM_BTM - FRM_LEN, FRM_LEN);
        _pU8g2->drawVLine(_left + FRM_RGT, _top + FRM_TOP, FRM_LEN);
        _pU8g2->drawVLine(_left + FRM_RGT, _top + FRM_BTM - FRM_LEN, FRM_LEN);

        _pU8g2->drawHLine(_left + FRM_LFT + 1, _top + FRM_TOP, FRM_LEN);
        _pU8g2->drawHLine(_left + FRM_LFT + 1, _top + FRM_BTM - 1, FRM_LEN);
        _pU8g2->drawHLine(_left + FRM_RGT - FRM_LEN, _top + FRM_TOP, FRM_LEN);
        _pU8g2->drawHLine(_left + FRM_RGT - FRM_LEN, _top + FRM_BTM - 1, FRM_LEN);
        for (byte x = FRM_LFT; x <= FRM_RGT; x += 8)
        {
            _pU8g2->drawHLine(_left + x, _top + FRM_CTR, 2);
        }

        _pU8g2->drawVLine(_left + FRM_LFT + 10, _top + FRM_CTR - (2), 4);
    }

    void drawString()
    {
        static char chrBuff[10] = {0};
        float tmp = 0.0;

        sprintf(chrBuff, "%d", _delay);
        _pU8g2->drawStr(_left, _top, chrBuff);

        tmp = _dataAve * _convertVoltCoff;
        sprintf(chrBuff, "%4.2f", tmp);
        _pU8g2->drawStr(_left + 105, _top, chrBuff);

        tmp = _rangeMax * _convertVoltCoff;
        sprintf(chrBuff, "%4.2f", tmp);
        _pU8g2->drawStr(_left, _top + 9, chrBuff);

        tmp = ((_rangeMax + _rangeMin) >> 1) * _convertVoltCoff;
        sprintf(chrBuff, "%4.2f", tmp);
        _pU8g2->drawStr(_left, _top + FRM_CTR - 4, chrBuff);

        tmp = _rangeMin * _convertVoltCoff;
        sprintf(chrBuff, "%4.2f", tmp);
        _pU8g2->drawStr(_left, _top + FRM_BTM - 8, chrBuff);
    }

    void drawData(byte drawLastIndex = DATA_BUF_HALF)
    {
        long y, y2;
        for (byte x = 0; x < drawLastIndex; x += 2)
        {
            byte bufIndex = max(x + _triggerPoint - 10, 0);
            byte bufIndexMinusOne = max(((int16_t)bufIndex - 1), 0);
            y = map(_dataBuff[bufIndexMinusOne], _rangeMin, _rangeMax, FRM_BTM - 1, FRM_TOP + 1);
            y2 = map(_dataBuff[bufIndex], _rangeMin, _rangeMax, FRM_BTM - 1, FRM_TOP + 1);
            _pU8g2->drawLine(_left + x + 27, _top + y, _left + x + 28, _top + y2);
        }
    }
};
