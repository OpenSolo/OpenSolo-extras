/**********************************************************************

  pattern_generator.h - a set of utilities to generate parametrically
    driven patterns. The time domain for the pattern must be updated
    via the theta parameter. In addition, phase and speed can be 
    specified, which will be used to adjust the specified theta value.
    Computation of patterns generally follows the convention: 

        value = bias + amplitude * f(x)

    ...where x is some speed and phase adjusted time domain 
    ...where f() is a carrier function determined by the pattern 



  Authors: 
    Nate Fisher

  Created: 
    Wed Oct 1, 2014

**********************************************************************/

#ifndef  PATTERN_GENERATOR_H
#define  PATTERN_GENERATOR_H

#include "utilities.h"

static const int CYCLES_INFINITE = -2;
static const int CYCLES_STOP = -1;

typedef enum _Pattern_Enum {
    PATTERN_OFF,                // 0
    PATTERN_BREATHE,            // 1
    PATTERN_SOLID,              // 2
    PATTERN_SIREN,              // 3
    PATTERN_STROBE,             // 4
	PATTERN_AVIATION_STROBE,	// 5
    PATTERN_FADEIN,             // 6
    PATTERN_FADEOUT,            // 7
    PATTERN_PARAMUPDATE,        // 8
	PATTERN_FWUPDATE,           // 9
    PATTERN_ENUM_COUNT,         // 10
	PATTERN_PING = 0xAA			// Special byte sent by the oreoled master startup sequence
} PatternEnum;

typedef struct _Pattern_Generator_State {
    
    int8_t cyclesRemaining; 
    PatternEnum pattern;
    double theta;
    uint8_t speed;
    double phase;
    double amplitude;
    uint8_t bias;
    uint8_t value;
    uint8_t isNewCycle;

} PatternGenerator;

// create pattern generators for all
//  three LED channels
PatternGenerator pgRed;
PatternGenerator pgGreen;
PatternGenerator pgBlue;

void PG_init(PatternGenerator*);
void PG_calc(PatternGenerator*, double);
void _PG_calcTheta(PatternGenerator*, double);
void _PG_patternOff(PatternGenerator*);
void _PG_patternSolid(PatternGenerator*);
void _PG_patternStrobe(PatternGenerator*);
void _PG_patternSine(PatternGenerator*);
void _PG_patternSiren(PatternGenerator*);
void _PG_patternFadeIn(PatternGenerator*);
void _PG_patternFadeOut(PatternGenerator*);

#endif
