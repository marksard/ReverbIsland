#include <hardware/pwm.h>
#include <U8g2lib.h>
#include "Button.hpp"
#include "SmoothAnalogRead.hpp"
#include "EzOscilloscope.hpp"
#include "Presets.hpp"
#include "Settings.hpp"
#include "GpioSet.h"

// 操作関係
static Button sw0;
static Button sw1;
static SmoothAnalogRead pots[POTS_MAX];
static uint potSlices[POTS_MAX] = {0};
static uint potChs[POTS_MAX] = {PWM_CHAN_A, PWM_CHAN_B, PWM_CHAN_A};
static uint pwmPotGpios[POTS_MAX] = {PWM_POT0, PWM_POT1, PWM_POT2};

static SmoothAnalogRead cv;
static EzOscilloscope ezOscillo;

// 表示関係
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, /* reset=*/U8X8_PIN_NONE);
static int8_t presetIndex = 0;
static uint16_t potValues[POTS_MAX] = {0};
static uint16_t potSettingValues[POTS_MAX] = {0};

void initOLED()
{
    u8g2.begin();
    u8g2.setContrast(40);
    u8g2.setFontPosTop();
    u8g2.setDrawColor(2);
#ifndef PROTO
    u8g2.setFlipMode(1);
#endif
    initPresets(&u8g2);
    initSettings(&u8g2);
}

void initRomBit()
{
    pinMode(T0, OUTPUT);
    pinMode(ROM1, OUTPUT);
    pinMode(ROM2, OUTPUT);
}

// プリセットの現在値から現在のROM/EEPROM切り替え設定を行う
void setRomBit(byte index)
{
    static byte mapIndexOld = 255;
    byte mapIndex = index / PRESET_SELECT_MAX;
    if (mapIndexOld == mapIndex)
    {
        return;
    }

    mapIndexOld = mapIndex;
    byte t0 = LOW;
    byte rom1 = LOW;
    byte rom2 = LOW;
    switch (mapIndex)
    {
    case 0:
        t0 = LOW;
        rom1 = HIGH;
        rom2 = HIGH;
        break;
    case 1:
        t0 = HIGH;
        rom1 = LOW;
        rom2 = HIGH;
        break;
    case 2:
        t0 = HIGH;
        rom1 = HIGH;
        rom2 = LOW;
        break;
    default:
        t0 = LOW;
        rom1 = HIGH;
        rom2 = HIGH;
        break;
    }

    digitalWrite(T0, t0);
    digitalWrite(ROM1, rom1);
    digitalWrite(ROM2, rom2);
}

void initPresetBit()
{
    pinMode(S0, OUTPUT);
    pinMode(S1, OUTPUT);
    pinMode(S2, OUTPUT);
}

// プリセットの現在値からプリセット設定を行う
void setPresetBit(byte index)
{
    static byte indexOld = 255;
    if (indexOld == index)
    {
        return;
    }

    indexOld = index;
    // presetIndexは8以上入るがretReadで3bitしか読んでないので問題なし
    digitalWrite(S0, bitRead(index, 0));
    digitalWrite(S1, bitRead(index, 1));
    digitalWrite(S2, bitRead(index, 2));
}

void initPWMPotsOut()
{
    for (byte i = 0; i < POTS_MAX; ++i)
    {
        // GP0-GP3のピン機能をPWMに設定
        gpio_set_function(pwmPotGpios[i], GPIO_FUNC_PWM);
        // GP0-GP3のPWMスライスを取得
        potSlices[i] = pwm_gpio_to_slice_num(pwmPotGpios[i]);
        // PWM周期
        // 31.25kHz ビットは設定による
        pwm_set_clkdiv(potSlices[i], 3.90625);
        pwm_set_wrap(potSlices[i], POTS_MAX_VALUE);
        // PWM出力イネーブル
        pwm_set_enabled(potSlices[i], true);
    }
}

template <typename su = uint8_t>
su constrainCyclic(su value, su min, su max)
{
    if (value > max)
        return min;
    if (value < min)
        return max;
    return value;
}

void initController()
{
    // internal regulator output mode -> PWM
    pinMode(23, OUTPUT);
    digitalWrite(23, HIGH);

    // ADC解像度とパルス出力解像度はあわせること
    analogReadResolution(POTS_BIT);
    sw0.init(SW0);
    sw1.init(SW1);
    sw0.setHoldTime(1000);
    sw1.setHoldTime(1000);
    pots[0].init(POT0);
    pots[1].init(POT1);
    pots[2].init(POT2);
    cv.init(CV);
    ezOscillo.init(&u8g2, &cv, POTS_ROW * 16);

    initRomBit();
    setRomBit(presetIndex);

    initPresetBit();
    setPresetBit(presetIndex);

    initPWMPotsOut();
}

extern byte assignCVMode;
extern byte assignCV2Pot;
extern byte assignCVDepth;

static byte unlock[3] = {0};
void resetUnlock()
{
    for (byte i = 0; i < 3; ++i)
    {
        unlock[i] = 0;
    }
}

void updatePresetsValues()
{
    // ポット処理更新
    for (byte i = 0; i < POTS_MAX; ++i)
    {
        uint16_t readValue = pots[i].analogRead();
        byte value = (*(byte *)(values[presetIndex][i][0]));
        byte min = (*(byte *)values[presetIndex][i][1]);
        byte max = (*(byte *)values[presetIndex][i][2]);
        byte newValue = constrain(map(readValue, 0, POTS_MAX_VALUE, min, max), min, max);
        if (newValue == value)
        {
            unlock[i] = 1;
        }

        if (unlock[i])
        {
            (*(byte *)(values[presetIndex][i][0])) = newValue;       
        }

        if (assignCV2Pot == i)
        {
            uint16_t cvValue = cv.analogRead() * (0.01 * assignCVDepth);
            byte newCVValue = constrain(map(cvValue, 0, POTS_MAX_VALUE, min, max), min, max);

            uint16_t cvUni = (uint16_t)(POTS_MAX_VALUE * (0.01 * assignCVDepth)) >> 1;
            byte newCVUni = constrain(map(cvUni, 0, POTS_MAX_VALUE, min, max), min, max);

            if (assignCVMode == 0)
            {
                newValue = constrain(newValue + newCVValue, min, max);
            }
            else if (assignCVMode == 1)
            {
                int16_t unipoleCVValue = constrain((int16_t)(newCVValue - newCVUni), -max, max);
                newValue = constrain(newValue + unipoleCVValue, min, max);
            }
            (*(byte *)(values[presetIndex][i][0])) = newValue;       
        }

        potValues[i] = readValue;
        // FV-1へポットの値をパルス出力
        pwm_set_chan_level(potSlices[i], potChs[i], potValues[i]);
    }
}

void updateSettings()
{
    byte settingIndex = 0;
    // ポット処理更新
    for (byte i = 0; i < POTS_MAX; ++i)
    {
        uint16_t readValue = pots[i].analogRead();
        byte value = (*(byte *)(settingValues[settingIndex][i][0]));
        byte min = (*(byte *)settingValues[settingIndex][i][1]);
        byte max = (*(byte *)settingValues[settingIndex][i][2]);
        byte newValue = constrain(map(readValue, 0, POTS_MAX_VALUE, min, max), min, max);
        if (newValue == value)
        {
            unlock[i] = 1;
        }

        if (unlock[i])
        {
            (*(byte *)(settingValues[settingIndex][i][0])) = newValue;       
        }
        potSettingValues[i] = readValue;
    }
}

static byte dispMode = 0;
void updateController()
{
    static byte lastPresetIndex = presetIndex;
    byte stateSw0 = sw0.getState();
    byte stateSw1 = sw1.getState();
    if (dispMode == 0)
    {
        updatePresetsValues();
        // ボタン処理：プリセット変更
        if (stateSw0 == 2)
        {
            presetIndex = constrainCyclic(presetIndex + 1, 0, PRESET_TOTAL - 1);
            setRomBit(presetIndex);
            setPresetBit(presetIndex);
        }
        else if (stateSw0 == 3)
        {
            dispMode = 1;
            resetUnlock();
        }
        else if (stateSw1 == 2)
        {
            presetIndex = constrainCyclic(presetIndex - 1, 0, PRESET_TOTAL - 1);
            setRomBit(presetIndex);
            setPresetBit(presetIndex);
        }
        else if (stateSw1 == 3)
        {
            dispMode = 2;
            resetUnlock();
        }
    }
    else if (dispMode == 1)
    {
        if (stateSw0 == 2)
        {
            ezOscillo.incDelay();
        }
        else if (stateSw0 == 3)
        {
            dispMode = 0;
            resetUnlock();
        }
        else if (stateSw1 == 2)
        {
            ezOscillo.decDelay();
        }
        else if (stateSw1 == 3)
        {
            dispMode = 2;
            resetUnlock();
        }
    }
    else if (dispMode == 2)
    {
        updateSettings();
        if (stateSw0 == 2)
        {
        }
        else if (stateSw0 == 3)
        {
            dispMode = 1;
            resetUnlock();
        }
        else if (stateSw1 == 2)
        {
        }
        else if (stateSw1 == 3)
        {
            dispMode = 0;
            resetUnlock();
        }
    }
    
}

// CPU 1は操作系専用
void setup()
{
    Serial.begin(9600);
    initController();
}

void loop()
{
    updateController();
    delay(1);
}

// CPU 2は表示専用
void setup1()
{
    initOLED();
    dispPresets(&u8g2, presetIndex, potValues);
}

void loop1()
{
    switch (dispMode)
    {
    case 0:
        dispPresets(&u8g2, presetIndex, potValues);
        break;
    case 1:
        ezOscillo.play();
        break;
    case 2:
        dispSettings(&u8g2, potSettingValues);
        break;
    }
    // 30fpsで更新
    delay(33);
}
