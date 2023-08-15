/*!
 * AnalogRead class
 * Copyright 2023 marksard
 * This software is released under the MIT license.
 * see https://opensource.org/licenses/MIT
 */ 

#pragma once

#include <Arduino.h>

class SmoothAnalogRead
{
public:
    SmoothAnalogRead() {}
    SmoothAnalogRead(byte pin)
    {
        init(pin);
    }
    
    /// @brief ピン設定
    /// @param pin
    void init(byte pin)
    {
        _pin = pin;
        _value = 0;
        pinMode(pin, INPUT_PULLUP);
    }

    uint16_t analogReadDirect()
    {
        return readPin();
    }

    uint16_t analogRead(bool smooth = true)
    {
        // アナログ入力。平均＋ローパスフィルタ仕様
        // フィルタは係数0.05096が半端なのは誤差で目減りしたぶんの帳尻合わせるため
        int aval = 0;
        for (byte j = 0; j < 16; ++j)
        {
            aval += readPin() + 1;
        }
        aval = aval >> 4;
        _value = (_value * 0.95) + (aval * 0.05096);
        return smooth ? _value : aval;
    }

protected:
    byte _pin;
    uint16_t _value;

    /// @brief ピン値読込
    /// @return
    virtual uint16_t readPin()
    {
        return ::analogRead(_pin);
    }
};
