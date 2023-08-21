#ifndef PATTERNS_H
#define PATTERNS_H

#include <FastLED.h>
#include <TaskScheduler.h>

CRGB leds[NUM_LEDS];
unsigned long nextChangeTime = 0;

static uint8_t patPos = 0;
static uint8_t patHue = 1;
static uint8_t brightness = 255;
static uint8_t wavePos = 0;
static int dir = 1;

painlessMesh mesh;

uint32_t get_millisecond_timer() {
   return mesh.getNodeTime()/1000 ;
}

enum class LEDPattern {
  RB_Cycle,
  RB_Wave,
  Fireworks_1D,
  RB_Slide,
  RB_Blend,
  RB_Cylon,  
  RB_Cylon2,
  RB_Breathe,
  RB_Fill,
  RB_Breathe_Solid,
  RB_Fill2,
  COUNT
};

struct PatternResult {
  LEDPattern pattern;
  unsigned long nextChange;
};

PatternResult rbCycle(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
  patPos = (patPos + 1) % 256;
  for(int i = 0; i < numLeds; i++) {
    leds[i] = CHSV((i * 256 / numLeds + patPos) % 256, 255, 255);
  }
  // currentTime = get_millisecond_timer();
  // nextChangeTime = currentTime + 100;
  //Serial.printf("Current: %u, Next: %u, patPos: %u, patHue: %u\n", currentTime, nextChangeTime, patPos, patHue);
  return { LEDPattern::RB_Cycle, currentTime + 100 };
}

PatternResult rbWave(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
  wavePos = 0;
    // Create a static rainbow along the LED strip
    for (int i = 0; i < numLeds; i++) {
        leds[i] = CHSV((i * 256 / numLeds + patPos) % 256, 255, 64); // The base color, half brightness
    }
    // Send a wave down the strip
    for (int i = 0; i < numLeds; i++) {
        // Use beat function to manipulate the brightness
        uint8_t wave = beatsin8(20, 64, 255, wavePos, i * 256 / numLeds); // Adjust these parameters to control the wave
        leds[i] += CHSV((i * 256 / numLeds + patPos) % 256, 255, wave); // The wave color, varying brightness
    }
    patPos = (patPos + 1) % 256;

    return { LEDPattern::RB_Wave, currentTime + 150 };
}

PatternResult rbBlend(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
  uint8_t speed = 1; // Adjust to control the speed
  uint8_t blendAmount;

  // Increment the position
  patPos += speed;
  blendAmount = (patPos % 256) / 9; // Adjusted speed

  for( int i = 0; i < numLeds; i++ ) {
    leds[i] = blend(CHSV(patPos, 255, 255), CHSV(patPos + 1, 255, 255), blendAmount);
  }

  return { LEDPattern::RB_Blend, currentTime + 125 };
}

PatternResult rbCylon(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
  static int dir = 1;
    if (patPos >= numLeds) {
    patPos = numLeds - 1;
    dir = -1;
  } else if (patPos < 0) {
    patPos = 0;
    dir = 1;
  }
  fadeToBlackBy(leds, numLeds, 64); // Fade all LEDs
  patPos += dir;
  patHue += dir;
  int trailLength = numLeds / 8; // This determines the length of the trail behind the "cylon" eye. Adjust as needed.
  // Loop over each LED in the trail
  for (int i = -trailLength; i <= trailLength; i++) {
    // Check that ledPosition is within bounds of the array
    int ledPosition = patPos + i;
    if (ledPosition >= 0 && ledPosition < numLeds) {
      uint8_t ledHue = patHue % 256; // Keep the hue the same for all LEDs
      uint8_t distanceToEye = abs(i);
      uint8_t brightness = 255 * (trailLength - distanceToEye) / trailLength; // Dim the LEDs the further they are from the "cylon" eye
      leds[ledPosition] = CHSV(ledHue, 255, brightness);
    }
  }
  // Change direction if the eye of the trail hits either end
  if (patPos == 0 || patPos == numLeds - 1) {
    dir *= -1;
  }
  return { LEDPattern::RB_Cylon, currentTime + 90 }; // Adjusted delay to make cycle close to 15 sec
}

PatternResult rbCylon2(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
  static int dir = 1;
  if (patPos >= numLeds) {
    patPos = numLeds - 1;
    dir = -1;
  } else if (patPos < 0) {
    patPos = 0;
    dir = 1;
  }
  fadeToBlackBy(leds, numLeds, 64); // Fade all LEDs
  patPos += dir;
  if(patPos >= numLeds - 1 || patPos <= 0) {
    dir *= -1;
  }
  patHue += dir;
  int trailLength = numLeds / 8; // This determines the length of the trail behind the "cylon" eye. Adjust as needed.
  // Calculate the hue range to be displayed across the LEDs
  uint8_t hueRange = 256 / trailLength; // This will create a full spectrum across the trail
  // Loop over each LED in the trail
  for (int i = -trailLength; i <= trailLength; i++) {
    int ledPosition = patPos + i;
    if (ledPosition < 0 || ledPosition >= numLeds) {
      continue;
    }
    uint8_t ledHue = (patHue + i * hueRange) % 256; // Calculate the hue of this LED in the trail
    uint8_t distanceToEye = abs(i);
    uint8_t brightness = 255 * (trailLength - distanceToEye) / trailLength; // Dim the LEDs the further they are from the "cylon" eye
    leds[ledPosition] = CHSV(ledHue, 255, brightness);
  }

  return { LEDPattern::RB_Cylon2, currentTime + 90 }; // Adjusted delay to make cycle close to 15 sec
}

PatternResult rbSlide(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
  static int dir = 1;
  static int patPos = 0;
  
  for (int i = 0; i < numLeds; i++) {
    // Dim the leds from the previous frame
    leds[i].nscale8(random8(200, 255));
  }
  
  // Generate a random hue
  uint8_t randomHue = random8();
  leds[patPos] = CHSV(randomHue, 255, 255);

  if (patPos >= numLeds - 1) {
      dir = -1;
  } else if (patPos <= 0) {
      dir = 1;
  }
  patPos += dir;

  return { LEDPattern::RB_Slide, currentTime + 90};
}

PatternResult rbBreathe(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
  static bool increasing = true;
  // Make sure patPos is within the valid hue range
  uint8_t huePosition = patPos % 256;
  for (int i = 0; i < numLeds; i++) {
      leds[i] = CHSV((i * 256 / numLeds + huePosition) % 256, 255, brightness);
  }

  if (increasing) {
      if (brightness <= 250 ) { // Ensure there's room to increase brightness
          brightness += 3;
      } else {
          brightness = 255;
          patPos = (patPos + 1) % numLeds;; // Update hue
          increasing = false;
      }
  } else {
      if (brightness >= 30) { // Ensure there's room to decrease brightness without going below 30
          brightness -= 3;
      } else {
          brightness = 30;
          patPos = (patPos + 1) % numLeds; // Update hue
          increasing = true;
      }
  }
  
  return {LEDPattern::RB_Breathe, currentTime + 100}; 
}

PatternResult rbBreatheSolid(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
  static bool increasing = true;

  // Increment patPos with every call, ensuring it's within the valid hue range
  patHue = (patHue + 1) % 256;

  for (int i = 0; i < numLeds; i++) {
      leds[i] = CHSV(patHue, 255, brightness);
  }

  if (increasing) {
      brightness += 3;
      if (brightness >= 250) {
          brightness = 255;
          increasing = false;
      }
  } else {
      brightness -= 3;
      if (brightness <= (10 + 30)) {
          brightness = 10;
          increasing = true;
      }
  }

  return {LEDPattern::RB_Breathe_Solid, currentTime + 100};
}

PatternResult rbFill(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
  static int state = 0;
  static int patPos = 0;
  // currentTime = get_millisecond_timer();
  switch (state) {
    case 0: 
        leds[patPos] = CHSV(patPos * 256 / numLeds + patPos, 255, 255);
        patPos = (patPos + 1) % numLeds;
        if (patPos == 0) {
            state = 1;
            return {LEDPattern::RB_Fill, currentTime + 1000};
        }
        break;
    case 1: 
        leds[patPos] = CRGB::Black;
        patPos = (patPos + 1) % numLeds;
        if (patPos == 0) {
            state = 0;
            return {LEDPattern::RB_Fill, currentTime + 250};
        }
        break;
  }

  return {LEDPattern::RB_Fill, currentTime + 100};
} 

PatternResult rbFill2(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
  static int state = 0;
  static int patPos = 0;
  static bool direction = true;
  // currentTime = get_millisecond_timer();
  switch (state) {
    case 0: 
      leds[patPos] = CHSV(patPos * 256 / numLeds + patPos, 255, 255);

      if (direction) {
        patPos = (patPos + 1) % numLeds;
        if (patPos == 0) {
            direction = false;
        }
      } else {
        patPos = (patPos - 1 + numLeds) % numLeds;
        if (patPos == numLeds - 1) {
          state = 1;
          return {LEDPattern::RB_Fill2, currentTime + 1000};
        }
      }
      break;

    case 1: 
      leds[patPos] = CRGB::Black;

        if (!direction) {
          patPos = (patPos - 1 + numLeds) % numLeds;
          if (patPos == numLeds - 1) {
            state = 0;
            direction = true;
            patPos = 0;
            return {LEDPattern::RB_Fill2, currentTime + 250}; 
          }
        }
        break;
  }

  return {LEDPattern::RB_Fill2, currentTime + 100}; 
}

PatternResult fireworks1D(CRGB* leds, uint8_t numLeds, unsigned long currentTime) {
    static int state = 0;
    static int whiteLED = 0;
    static int explosionStart = -1;
    static int explosionSize = 0;
    static int explosionTrigger = random(numLeds); // choose a random position for explosion
    static const int MAX_EXPLOSION_SIZE = 10; // maximum size of the explosion
    static const int WHITE_SPEED = 1; // speed of the white LED
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
    case LEDPattern::RB_Cycle:
      return rbCycle(leds, numLeds, currentTime);
    case LEDPattern::RB_Wave:
      return rbWave(leds, numLeds, currentTime);
    case LEDPattern::RB_Blend:
      return rbBlend(leds, numLeds, currentTime);    
    case LEDPattern::RB_Cylon:
      return rbCylon(leds, numLeds, currentTime);    
      case LEDPattern::RB_Cylon2:
      return rbCylon2(leds, numLeds, currentTime);
    case LEDPattern::RB_Slide:
      return rbSlide(leds, numLeds, currentTime);
    case LEDPattern::RB_Breathe:
      return rbBreathe(leds, numLeds, currentTime);
    case LEDPattern::RB_Breathe_Solid:
      return rbBreatheSolid(leds, numLeds, currentTime);
    case LEDPattern::RB_Fill:
      return rbFill(leds, numLeds, currentTime);
    case LEDPattern::RB_Fill2:
      return rbFill2(leds, numLeds, currentTime);
    case LEDPattern::Fireworks_1D:
      return fireworks1D(leds, numLeds, currentTime);
    default:
      // Add error handling here
      Serial.println("Error: Unknown pattern, defaulting to RB_Cycle");
      return rbCycle(leds, numLeds, currentTime);
  }
}

LEDPattern getNextPattern(LEDPattern currentPattern){
  int nextPatternAsInt = static_cast<int>(currentPattern) + 1;
  if (nextPatternAsInt >= static_cast<int>(LEDPattern::COUNT)) {
    nextPatternAsInt = static_cast<int>(LEDPattern::RB_Cycle);
  }
  return static_cast<LEDPattern>(nextPatternAsInt);
}

#endif