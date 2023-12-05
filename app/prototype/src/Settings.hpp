#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include "GpioSet.h"
#include "ParamGroup.hpp"

#define EXSETMENU_MAX 1

// プリセット名やパラメタ名などは、EEPROMには入ってないしEEPROMを直接読まないのでここで都度定義する必要がある
const static char *settingNames[EXSETMENU_MAX][4] = {
    // INTERNAL PRESETS
    {"CV Assig Setting", "Mode       ", "Dest Pot No", "Depth      "},
};

extern byte mode0;
extern byte mode1;
extern byte mode2;

extern byte minValue;
static byte maxCVMode = 2;
static byte maxCV2Pot = POTS_MAX - 1;
static byte maxCVDepth = 100;
static ParamGroup settingGroup[EXSETMENU_MAX];

byte assignCVMode = 0;
byte assignCV2Pot = 2;
byte assignCVDepth = 50;

static byte *settingValues[EXSETMENU_MAX][POTS_MAX][4] =
{
    {
        {&assignCVMode, &minValue, &maxCVMode, &mode2},
        {&assignCV2Pot, &minValue, &maxCV2Pot, &mode1},
        {&assignCVDepth, &minValue, &maxCVDepth, &mode1}, 
    },
};

void initSettings(U8G2 *pU8g2)
{
    for (byte i = 0; i < EXSETMENU_MAX; ++i)
    {
        settingGroup[i].init(pU8g2);
        settingGroup[i].attachNames(settingNames[i]);
        settingGroup[i].attachValues(settingValues[i]);
    }
}

void dispSettings(U8G2 *pU8g2, uint16_t values[POTS_MAX])
{
    pU8g2->clearBuffer();

    settingGroup[0].dispParamGroup(values);
    settingGroup[0].dispTitle();

    pU8g2->sendBuffer();
}