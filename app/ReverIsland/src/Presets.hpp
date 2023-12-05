#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include "GpioSet.h"
#include "ParamGroup.hpp"

#define PRESET_SELECT_MAX 8
#define PRESET_MAP_MAX 3 // INTERNAL PRESETS + (EEPROM x 2)
// presetIndexの最大値はPRESET_SELECT_MAX * PRESET_MAP_MAXとなる
#define PRESET_TOTAL (PRESET_SELECT_MAX * PRESET_MAP_MAX)

// プリセット名やパラメタ名などは、EEPROMには入ってないしEEPROMを直接読まないのでここで都度定義する必要がある
const static char *names[PRESET_TOTAL][4] = {
    // INTERNAL PRESETS
    {"ChorusReverb", "Reverb Mix  ", "Chorus Rate ", "Chorus Mix  "},
    {"FlangrReverb", "Reverb Mix  ", "Flanger Rate", "Flanger Mix "},
    {"Tremolo-rev ", "Reverb Mix  ", "Tremolo Rate", "Tremolo Mix "},
    {"Pitch shift ", "Pitch Semi  ", "------------", "------------"},
    {"Pitch-echo  ", "Pitch Shift ", "Echo Delay  ", "Echo Mix    "},
    {"Test        ", "------------", "------------", "------------"},
    {"Reverb 1    ", "Reverb Time ", "HF Filter   ", "LF Filter   "},
    {"Reverb 2    ", "Reverb Time ", "HF Filter   ", "LF Filter   "},
    // EEPROM A（例）marksard selection vol.1 
    {"ShimmerRvOct", "Shimmer     ", "Time        ", "Damping     "}, // dattorro-shimmer_oct_var-lvl(Dattorro Mix Reverb)  dattorro-shimmer_oct_var-lvl.spn
    {"Plate Reverb", "Reverb level", "Reverb time ", "Damping     "}, // Plate Reverb - Dattorro(Dattorro)  dattorro.spn
    {"Echo Reverb ", "Delay       ", "Repeat      ", "Reverb      "}, // Echo Reverb(Spin Semi)  3K_V1_4_ECHO-REV.spn
    {"3TCascadeChr", "Time 1      ", "Time 2      ", "Time 3      "}, // Triple Tap Cascaded Delay - Stereo w/ Chorus(Graham Biswell)   tripple_echo_cascaded_stereo+chorus.spn
    {"SnglTapeEcRv", "Time        ", "Feedback    ", "Damping     "}, // Single Head Tape Echo + Reverb(No name)  dv103-1head-pp-2_1-4xreverb.spn
    {"Flanger     ", "Speed       ", "Depth       ", "Feedback    "}, // Flanger(Firesledge)  05_bass-fv1-p0-flanger.spn
    {"Rv+Flnge+LP ", "Reverb      ", "Flanger     ", "LPF         "}, // Reverb+Flange+LP(Dave Spinkler)  dance_ir_fla_l.spn
    {"Rv+Pitch+LP ", "Reverb      ", "Pitch       ", "Filter      "}, // Reverb+Pitch+LP(Dave Spinkler)  dance_ir_ptz_l.spn
    // EEPROM B（例）
    {"Phaser OD   ", "Speed       ", "Depth       ", "Feedback    "}, // Phaser OD(Firesledge)  bass-fv1-p1-phaser.spn
    {"Distortion  ", "Gain        ", "Tone        ", "Dry/Wet mix "}, // Distortion(Firesledge)  bass-fv1-p5-disto.spn
    {"Bit crusher ", "P1          ", "P2          ", "P3          "}, // Bit crusher(Frank Thomson)  crusher.spn
    {"Wah         ", "Reverb      ", "Sensitivity ", "FilterQLevel"}, // Wah(Spin Semi)  GA_DEMO_WAH.spn
    {"OilCan Delay", "Time & Rate ", "Chorus Width", "Feedback    "}, // Oil can delay(Digital Larry)  oil-can-delay.spn
    {"Soft Clip OD", "Gain Thresh ", "Volume      ", "Tone        "}, // Soft Clipping Overdrive(Jeroen Korterik)  softclipping_overdrive.spn
    {"St2FlngMTapD", "Feedback    ", "Reso & Time ", "Return Level"}, // Stereo Dual Flange Multi Tap Delay(Digital Larry)  stereo-dual-flange-multi-tap-delay.spn
    {"StRingModChr", "Blend       ", "CarrierOffst", "Chorus      "}, // Stereo Ring Modulators w/ Chorus(Digital Larry) stereo-ring-modulators-with-chorus.spn

    // // EEPROM A
    // {"EEPROM A    ", "P1          ", "P2          ", "P3          "}, //
    // {"EEPROM A    ", "P1          ", "P2          ", "P3          "}, //
    // {"EEPROM A    ", "P1          ", "P2          ", "P3          "}, //
    // {"EEPROM A    ", "P1          ", "P2          ", "P3          "}, //
    // {"EEPROM A    ", "P1          ", "P2          ", "P3          "}, //
    // {"EEPROM A    ", "P1          ", "P2          ", "P3          "}, //
    // {"EEPROM A    ", "P1          ", "P2          ", "P3          "}, //
    // {"EEPROM A    ", "P1          ", "P2          ", "P3          "}, //
    // EEPROM B
    // {"EEPROM B    ", "P1          ", "P2          ", "P3          "}, //
    // {"EEPROM B    ", "P1          ", "P2          ", "P3          "}, //
    // {"EEPROM B    ", "P1          ", "P2          ", "P3          "}, //
    // {"EEPROM B    ", "P1          ", "P2          ", "P3          "}, //
    // {"EEPROM B    ", "P1          ", "P2          ", "P3          "}, //
    // {"EEPROM B    ", "P1          ", "P2          ", "P3          "}, //
    // {"EEPROM B    ", "P1          ", "P2          ", "P3          "}, //
    // {"EEPROM B    ", "P1          ", "P2          ", "P3          "}, //
};

extern byte mode0;
extern byte mode1;
extern byte mode2;

static byte minValue = 0;
static byte maxValue = 127;
static byte lastPot[POTS_MAX] = {0};
static byte *values[PRESET_TOTAL][POTS_MAX][4] = {0};
static ParamGroup ps[PRESET_TOTAL];

void initPresets(U8G2 *pU8g2)
{
    for (byte i = 0; i < PRESET_TOTAL; ++i)
    {
        ps[i].init(pU8g2);
        for (byte j = 0; j < POTS_MAX; ++j)
        {
            values[i][j][0] = &lastPot[j];
            values[i][j][1] = &minValue;
            values[i][j][2] = &maxValue;
            values[i][j][3] = &mode1;
        }

        ps[i].attachNames(names[i]);
        ps[i].attachValues(values[i]);
    }
}

void dispPresets(U8G2 *pU8g2, byte index, uint16_t values[POTS_MAX])
{
    pU8g2->clearBuffer();

    ps[index].dispParamGroup(values);

    // ROM/EEPROPM1/2の表示、プリセット名表示
    static char mapName[2] = {0};
    byte mapIndex = index / PRESET_SELECT_MAX;
    sprintf(mapName, mapIndex == 0 ? "R" : mapIndex == 1 ? "A"
                                                         : "B");
    ps[index].dispTitle(index % 8, mapName);

    pU8g2->sendBuffer();
}