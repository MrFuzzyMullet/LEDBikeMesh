#ifndef PATTERNS_H
#define PATTERNS_H
//gets rid of random flickering on pixels, probably due to wifi interrupts
#define FASTLED_ESP32_I2S true
#define USE_GET_MILLISECOND_TIMER 
#include <FastLED.h>
#include <TaskScheduler.h>

//Setup of second strand of LEDs from Quin if needed
CRGB leds[NUM_LEDS];
#ifdef QuinLED
  CRGB leds2[NUM_LEDS2];
#endif

unsigned long nextChangeTime = 0;

static uint8_t patPos = 0;
static uint8_t patHue = 1;
static uint8_t brightness = 255;

enum class LEDPattern {
  RB_March, //synced position and hue
  Pride2015, //not synced hue or position
  RB_Phase, //not synced position
  RB_Cylon,  //synced hue and position
  Fireworks_1D, //not synced hue or position
  RB_DualSlide, //synced hue and position
  RB_Blend, //synced hue and postion
  ConfettiAndSinelon, //not synced position
  RB_Fill, //synced hue and postion
  RainbowRunnerPhase, //not synced postion
  RB_Fill2, //synced position and hue
  RB_Cylon2, //synced position and hue
  COUNT
};

struct PatternResult {
  LEDPattern pattern;
  unsigned long nextChange;
};

PatternResult rbMarch(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
    uint16_t EYE_SIZE = numLeds / 20;
    uint16_t SEPARATION = numLeds / 6;
    uint16_t TOTAL_SIZE = EYE_SIZE + SEPARATION;

    // Calculate the number of full cycles that have completed
    uint16_t completedCycles = currentTime / (TOTAL_SIZE * 50);

    // Define the hue shift across the entire LED strip
    uint8_t baseHue = (completedCycles + (currentTime / (numLeds))) % 255; // The hue shifts slightly each frame and significantly after every loop

    // Dimming between frames
    fadeToBlackBy(leds, numLeds, 128); 

    // Calculate position offset for movement
    uint16_t phaseOffset = (currentTime / 100) % TOTAL_SIZE; 

    for(uint16_t i = 0; i < numLeds; i++) {
        uint16_t relativePosition = (i + phaseOffset) % TOTAL_SIZE;

        if(relativePosition < EYE_SIZE) {
            uint8_t hue = (baseHue + (i * 255 / numLeds)) % 255;  // Hue cycles along the strip
            uint8_t brightness = sin8((relativePosition * 255 / EYE_SIZE)) / 2 + 128;  // Use sine wave for eye intensity
            leds[i] = CHSV(hue, 255, brightness);
        }
    }

    return { LEDPattern::RB_March, currentTime + 50 };  // Update every 50ms
}

PatternResult pride2015(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
  //adapted from here: https://github.com/FastLED/FastLED/blob/master/examples/Pride2015/Pride2015.ino
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;
 
  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);
  
  uint16_t ms = currentTime;
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;
  
  for( uint16_t i = 0 ; i < numLeds; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);
    
    CRGB newcolor = CHSV( hue8, sat8, bri8);
    
    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS-1) - pixelnumber;
    
    nblend( leds[pixelnumber], newcolor, 64);
  }
  return { LEDPattern::Pride2015, currentTime + 100 }; 
}

PatternResult rbPhase(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
  
  // Use currentTime to adjust the phase of beatsin16 in seconds
  uint16_t timeAdjusted = currentTime / 1000;

  // Adjust the beatsin16 functions with the phase offset based on currentTime
  uint16_t beatA = beatsin16(20, 0, 255, 0, timeAdjusted);
  uint16_t beatB = beatsin16(18, 0, 255, 0, timeAdjusted);

  // Fill the rainbow pattern
  fill_rainbow(leds, numLeds, (beatA + beatB) / 2, (512/numLeds));

  // Scale brightness of each LED
  uint8_t brightnessFactor = 128; // Adjust this value from 0 (off) to 255 (full brightness)
  for(uint8_t i = 0; i < numLeds; i++) {
    leds[i].nscale8(brightnessFactor);
  }
  return {LEDPattern::RB_Phase, currentTime + 50}; // Update every 100ms, or whatever rate you prefer
}

PatternResult rainbowRunnerPhase(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
  
  // Adjust the divisor to slow down the hue change speed. Higher number is slower change
  uint8_t hue = currentTime / 10; 

  // Adjust the BPM of beatsin16 to be inversely proportional to numLeds for normalized speed
  int bpmFactorA = 10 * (100.0 / numLeds);
  int bpmFactorB = 8 * (100.0 / numLeds);

  // Use currentTime to adjust the phase of beatsin16.
  uint16_t timeAdjusted = currentTime / 1000;

  // Adjust the beatsin16 functions with the phase offset based on currentTime
  uint16_t beatA = beatsin16(bpmFactorA, 0, numLeds - 10, 0, timeAdjusted);
  uint16_t beatB = beatsin16(bpmFactorB, 0, numLeds - 10, 0, timeAdjusted);

  // Calculate the lead position of the runner
  uint16_t runnerPosition = (beatA + beatB) / 2;

  // Clear the leds for fresh drawing
  fadeToBlackBy(leds, numLeds, 60); 

  // Color the LED at runnerPosition and create a trailing fade for the subsequent 10 LEDs
  for(int offset = 0; offset < 10; offset++) {
    if(runnerPosition + offset < numLeds) {
        leds[runnerPosition + offset] += CHSV((hue + offset*10) % 256, 255, 255 - offset*25); 
    }
  }

  return {LEDPattern::RainbowRunnerPhase, currentTime + 50};  // Update every 100ms
}

PatternResult confettiAndSinelon(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
  
  // Use currentTime to adjust hue in a consistent manner
  uint8_t patHue = currentTime / 10;  // Adjust the divisor to control the speed of hue change. Larger values result in slower hue change
  
  // Fade the LEDs for trailing effect
  fadeToBlackBy(leds, numLeds, 45);
  
  // Sinelon effect
  // Using beatsin16 with a constant range (0-255) and then scaling it to LED count
  int pos1 = map(beatsin16(6, 0, 255, currentTime/1000), 0, 255, 0, numLeds - 1);
  leds[pos1] += CHSV(patHue, 255, 192);
  
  // Confetti Effect
  static unsigned long lastUpdateTime = 0;
  if(currentTime - lastUpdateTime >= 200) {
    int pos2 = random16(numLeds);
    int pos3 = random16(numLeds);
    int pos4 = random16(numLeds);

    leds[pos2] += CHSV(patHue + random8(64), 200, 128);
    leds[pos3] += CHSV(patHue + random8(64), 200, 128);
    leds[pos4] += CHSV(patHue + random8(64), 200, 128);

    lastUpdateTime = currentTime;
  }
  
  return {LEDPattern::ConfettiAndSinelon, currentTime + 50};
}

PatternResult rbBlend(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
  // Controls how fast the hue changes over time. Increase to slow down the hue change.
  uint8_t timeBasedPosition = (currentTime / 100) % 256;

  // Determines how much blending occurs between the two colors. The range is 0-28. Change the divisor to adjust blending granularity.
  uint8_t blendAmount = (timeBasedPosition % 256) / 12;

  // Blend between adjacent hues to achieve a smooth transition effect.
  for(int i = 0; i < numLeds; i++) {
    leds[i] = blend(
      CHSV(timeBasedPosition, 255, 128),         // Current hue
      CHSV(timeBasedPosition + 1, 255, 128),     // Next hue
      blendAmount                                // Amount of blending between the two hues
    );
  }

  // Controls the frequency of pattern updates. Decrease for more frequent updates.
  return { LEDPattern::RB_Blend, currentTime + 50 };
}

PatternResult rbCylon(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
    fadeToBlackBy(leds, numLeds, 64); // Fade all LEDs

    const unsigned long CYCLE_DURATION = 5000; // 5000 ms for one full back-and-forth cycle
    int halfCycle = CYCLE_DURATION / 2;
    
    // Calculate normalized position based on triangle wave 
    int elapsedInCycle = currentTime % CYCLE_DURATION;
    float normalizedPosition;
    
    if (elapsedInCycle < halfCycle) {
        normalizedPosition = static_cast<float>(elapsedInCycle) / halfCycle;
    } else {
        normalizedPosition = 2.0f - static_cast<float>(elapsedInCycle) / halfCycle;
    }

    // Scale normalized position by the number of LEDs in the strip
    int patPos = normalizedPosition * (numLeds - 1);  // -1 because LED positions range from 0 to numLeds-1

    uint8_t patHue = (currentTime / 256) % 256;  // Increase hue over time, adjust the 64 value for hue change speed
    
    int trailLength = numLeds / 8;

    // Loop over each LED in the trail
    for (int i = -trailLength; i <= trailLength; i++) {
        // Check that ledPosition is within bounds of the array
        int ledPosition = patPos + i;
        if (ledPosition >= 0 && ledPosition < numLeds) {
            uint8_t ledHue = patHue % 256; 
            uint8_t distanceToEye = abs(i);
            uint8_t brightness = 255 * (trailLength - distanceToEye) / trailLength;
            leds[ledPosition] = CHSV(ledHue, 255, brightness);
        }
    }

    return { LEDPattern::RB_Cylon, currentTime + 20 };
}

PatternResult rbCylon2(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
    fadeToBlackBy(leds, numLeds, 64); // Fade all LEDs

    const unsigned long CYCLE_DURATION = 7500; // 2000 ms for one full back-and-forth cycle
    int halfCycle = CYCLE_DURATION / 2;
    
    // Calculate normalized position based on triangle wave 
    int elapsedInCycle = currentTime % CYCLE_DURATION;
    float normalizedPosition;
    
    if (elapsedInCycle < halfCycle) {
        normalizedPosition = static_cast<float>(elapsedInCycle) / halfCycle;
    } else {
        normalizedPosition = 2.0f - static_cast<float>(elapsedInCycle) / halfCycle;
    }

    // Scale normalized position by the number of LEDs in the strip
    int patPos = normalizedPosition * (numLeds - 1);  // -1 because LED positions range from 0 to numLeds-1

    uint8_t patHue = (currentTime / 16) % 256;  // Increase hue over time, adjust the 8 value for hue change speed
    
    int trailLength = numLeds / 8;

    // Loop over each LED in the trail
    for (int i = -trailLength; i <= trailLength; i++) {
        // Check that ledPosition is within bounds of the array
        int ledPosition = patPos + i;
        if (ledPosition >= 0 && ledPosition < numLeds) {
            uint8_t distanceToEye = abs(i);
            uint8_t brightness = 255 * (trailLength - distanceToEye) / trailLength;
            leds[ledPosition] = CHSV(patHue, 255, brightness);  // Using consistent hue for the entire eye
        }
    }

    return { LEDPattern::RB_Cylon2, currentTime + 20 };
}

PatternResult rbDualSlide(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
  static int dir1 = 1;      // Direction for the first runner (from the start)
  static int dir2 = -1;     // Direction for the second runner (from the end)
  static int patPos1 = 0;   // Start of the strip for the first runner
  static int patPos2 = numLeds - 1;  // End of the strip for the second runner
  
  for (int i = 0; i < numLeds; i++) {
    // Dim the leds from the previous frame
    leds[i].nscale8(random8(128, 255));
  }

  // Generate random hues for each runner (can be different)
  uint8_t randomHue1 = random8();
  //uint8_t randomHue2 = random8();
  
  leds[patPos1] = CHSV(randomHue1, 255, 255);
  leds[patPos2] = CHSV(randomHue1, 255, 255);

  if (patPos1 >= numLeds - 1) {
      dir1 = -1;  // Change direction for the first runner
  } else if (patPos1 <= 0) {
      dir1 = 1;   // Change direction for the first runner
  }

  if (patPos2 <= 0) {
      dir2 = 1;   // Change direction for the second runner
  } else if (patPos2 >= numLeds - 1) {
      dir2 = -1;  // Change direction for the second runner
  }

  patPos1 += dir1;
  patPos2 += dir2;

  return { LEDPattern::RB_DualSlide, currentTime + 75};
}

PatternResult rbFill(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
    // Total duration for one full fill-unfill cycle (this remains constant regardless of strip length)
    const unsigned long CYCLE_DURATION = 7500; // 7500 ms for one full fill-unfill cycle

    // Calculate the normalized progression in the cycle (from 0.0 to 1.0)
    float normalizedPosition = static_cast<float>(currentTime % CYCLE_DURATION) / CYCLE_DURATION;

    int fillPosition;
    if (normalizedPosition <= 0.5f) {
        // If in the first half of the cycle (fill phase)
        fillPosition = normalizedPosition * 2.0f * numLeds;
    } else {
        // If in the second half of the cycle (unfill phase)
        fillPosition = (2.0f - normalizedPosition * 2.0f) * numLeds;
    }

    int startIndex = (normalizedPosition > 0.5f) ? numLeds - fillPosition : 0;

    for (int i = 0; i < numLeds; i++) {
        int hue = (i * 256) / numLeds;  // Calculate hue based on position along the LED strip

        if (i >= startIndex && i < startIndex + fillPosition) {
            leds[i] = CHSV(hue, 255, 200);
        } else {
            leds[i] = CRGB::Black;
        }
    }

    return { LEDPattern::RB_Fill, currentTime + 50 };
}

PatternResult rbFill2(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
  // Define a constant duration for the entire fill-unfill cycle.
  const unsigned long cycleDuration = 5000;  // 10 seconds for a full fill-unfill cycle

  // Calculate the position in the current fill-unfill cycle.
  unsigned long positionInCycle = currentTime % cycleDuration;

  // Calculate fillPosition based on the position in the cycle.
  int fillPosition;
  if (positionInCycle < cycleDuration / 2) {
    fillPosition = (positionInCycle * numLeds) / (cycleDuration / 2);
  } else {
    fillPosition = numLeds - 1 - ((positionInCycle - cycleDuration / 2) * numLeds) / (cycleDuration / 2);
  }

  // Calculate hue based on currentTime
  int hue = (currentTime / 100) % 256;

  for(int i = 0; i < numLeds; i++) {
      if(i <= fillPosition) {
          leds[i] = CHSV(hue, 255, 200);
      } else {
          leds[i] = CRGB::Black;
      }
  }

  return {LEDPattern::RB_Fill2, currentTime + 50}; // Update every 50ms
}

PatternResult fireworks1D(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
    static int state = 0;
    static int whiteLED = 0;
    static int explosionStart = -1;
    static int explosionSize = 0;
    static int explosionTrigger = random(numLeds); // choose a random position for explosion
    static const int MAX_EXPLOSION_SIZE = 10; // maximum size of the explosion
    static const int WHITE_SPEED = 2; // speed of the white LED
    static const int EXPLOSION_SPEED = 1; // Decreased explosion speed to make it last longer

    // Fading out the white LED trail and explosion
    // Adjust the fade rate based on whether an explosion is taking place
    fadeToBlackBy(leds, numLeds, state == 0 ? 128 : 20);

    switch (state) {
        case 0: // no explosion happening, move white LED
            leds[whiteLED] = CRGB::White;
            whiteLED += WHITE_SPEED;
            if (whiteLED >= explosionTrigger) { // start an explosion at the random place
                explosionStart = explosionTrigger;
                explosionSize = 1;
                whiteLED = 0;
                state = 1;
            }
            break;

        case 1: // explosion happening
            for (int i = max(0, explosionStart - explosionSize); i < min((int)numLeds, explosionStart + explosionSize); i++) {
                if(random(10) > 7){ // approximately 30% of the time, the LED will be black
                    leds[i] = CRGB::Black;
                } else {
                    leds[i] = CHSV(random(0, 255), 255, 255); // random color
                }
            }
            explosionSize += EXPLOSION_SPEED;
            if (explosionSize >= MAX_EXPLOSION_SIZE) { // end explosion
                explosionStart = -1;
                explosionSize = 0;
                explosionTrigger = random(numLeds); // select a new random position for next explosion
                state = 0;
            }
            break;
            return {LEDPattern::Fireworks_1D, currentTime + random(250,500)};
    }

  return {LEDPattern::Fireworks_1D, currentTime + 100}; // adjust for preferred animation speed
}

PatternResult setPattern(LEDPattern pattern, CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
  switch (pattern) {
    case LEDPattern::RB_March:
      return rbMarch(leds, numLeds, currentTime);
    case LEDPattern::Pride2015:
      return pride2015(leds, numLeds, currentTime);
    case LEDPattern::RB_Phase:
      return rbPhase(leds, numLeds, currentTime);
    case LEDPattern::RainbowRunnerPhase:
      return rainbowRunnerPhase(leds, numLeds, currentTime);
    case LEDPattern::ConfettiAndSinelon:
      return confettiAndSinelon(leds, numLeds, currentTime);
    case LEDPattern::RB_Blend:
      return rbBlend(leds, numLeds, currentTime);    
    case LEDPattern::RB_Cylon:
      return rbCylon(leds, numLeds, currentTime);    
    case LEDPattern::RB_Cylon2:
      return rbCylon2(leds, numLeds, currentTime);
    case LEDPattern::RB_DualSlide:
      return rbDualSlide(leds, numLeds, currentTime);
    // case LEDPattern::RB_Breathe_Solid:
    //   return rbBreatheSolid(leds, numLeds, currentTime);
    case LEDPattern::RB_Fill:
      return rbFill(leds, numLeds, currentTime);
    case LEDPattern::RB_Fill2:
      return rbFill2(leds, numLeds, currentTime);
    case LEDPattern::Fireworks_1D:
      return fireworks1D(leds, numLeds, currentTime);
    default:
      // Add error handling here
      Serial.println("Error: Unknown pattern, defaulting to RB_Cycle");
      return rbMarch(leds, numLeds, currentTime);
  }
}

LEDPattern getNextPattern(LEDPattern currentPattern){
  int nextPatternAsInt = static_cast<int>(currentPattern) + 1;
  if (nextPatternAsInt >= static_cast<int>(LEDPattern::COUNT)) {
    nextPatternAsInt = static_cast<int>(LEDPattern::RB_March);
  }
  return static_cast<LEDPattern>(nextPatternAsInt);
}

#endif