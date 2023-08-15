/*!
 * Button class
 * Copyright 2023 marksard
 * This software is released under the MIT license.
 * see https://opensource.org/licenses/MIT
 */ 

#pragma once

#include <Arduino.h>

class Button
{
public:
    Button() {}
    Button(byte pin)
    {
        init(pin);
    }
    
    /// @brief ピン設定
    /// @param pin
    void init(byte pin)
    {
        _pin = pin;
        _pinState = 0;
        _holdStage = 0;
        _holdTime = 500;

        pinMode(pin, INPUT_PULLUP);

        // 空読み
        for(int i = 0; i < 8; ++i)
        {
            getState();
        }
    }

    /// @brief ボタン状態を取得
    /// @return 0:None 1:Button down 2:Button up 3:Holding 4:Holded
    inline byte getState()
    {
        byte result = 0;
        byte value = readPin();
        // 簡単チャタ取り
        _pinState = (_pinState << 1) | value;

        // Holding
        if (_holdStage == 2)
        {
            // Holded
            if (_pinState == 0x0F)
            {
                _holdStage = 0;
                result = 4;
            }
            else if (_pinState == 0x00)
            {
                result = 3;
            }

            return result;
        }

        // Button down
        if (_pinState == 0xF0)
        {
            result = 1;
            _holdStage = 0;
        }
        // Button up
        else if (_pinState == 0x0F)
        {
            result = 2;
            _holdStage = 0;
        }
        // Hold check
        else if (_pinState == 0x00)
        {
            // Start hold check
            if (_holdStage == 0)
            {
                _holdStage = 1;
                _lastMillis = millis();
            }
            // Hold confirm (1sec)
            else if (millis() >= _lastMillis + _holdTime)
            {
                _holdStage = 2;
            }
        }

        return result;
    }

    void setHoldTime(int16_t mills)
    {
        _holdTime = mills;
    }

protected:
    byte _pin;
    byte _pinState;
    byte _holdStage;
    unsigned long _lastMillis;
    int16_t _holdTime;

    /// @brief ピン値読込
    /// @return
    virtual byte readPin()
    {
        return digitalRead(_pin);
    }
};
