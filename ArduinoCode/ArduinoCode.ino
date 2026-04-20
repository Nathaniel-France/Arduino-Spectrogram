/*
 * Written by Nathaniel France & Brandon Pak
 * for ECE 2804 Home Audio Project
 * Pin out:         Breadboard
 * A0 = Audio In    pin 11
 * A4 = SDA         pin 3
 * A5 = SCL         pin 4
 * D9 = PWM out     pin 20
 * + 3.3v           pin 5
 * + 5v             plus rail
 * GND              minus rail
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "FFT.h"

// Constructs SSD1306 class with width, height, i2c connections from Wire.h, and reset pin (-1 uses reset on arduino)
Adafruit_SSD1306 display(128, 32, &Wire, -1);

// Volatile needed bc these are modified by the ISR and main loop
volatile int8_t bufferA[128];
volatile int8_t bufferB[128];
volatile bool collectingA = true;   // which buffer the ISR is writing to
volatile int collectIndex = 0;
volatile bool bufferReady = false;

volatile int8_t* collectBuffer = bufferA;       // ISR writes here
volatile int8_t* processBuffer = bufferB;       // FFT reads from here

volatile int8_t prevSample = 0;
volatile int16_t prevFiltered = 0;

const q15 alpha = 32700; // ~0.99

uint8_t peakVol[5] = {0,0,0,0,0};
uint8_t peakTimer[5] = {0,0,0,0,0};
const uint8_t PEAKHOLDTIME = 20;
const uint8_t PEAKDECAY = 1;
const int MAXMAGNITUDE = 1000;

uint8_t smoothedHeights[5] = {0,0,0,0,0};
uint8_t heights[5] = {0,0,0,0,0};
uint16_t bands[5] = {0,0,0,0,0};

FFT fft(128);

// When Timer 2 overflows send an ISR, reload the timer, and collect 1 sample
// This gives the 10 kHz sampling rate by aligning the sampling with Timer 2
ISR(TIMER2_OVF_vect) {
    TCNT2 = 55;

    // Reads sample and converts to q15
    int8_t sample = ((analogRead(A0) - 512) >> 2);

    // High pass filter to remove DC: y[n] = x[n] - x[n-1] + alpha * y[n-1]
    int16_t filtered = (int16_t)sample - prevSample + (int16_t)(((int32_t)alpha * prevFiltered) >> 15);

    prevSample = sample;
    prevFiltered = filtered;

    if (filtered > 127) filtered = 127;
    else if (filtered < -128) filtered = -128;

    collectBuffer[collectIndex++] = (int8_t)(filtered);

    if(collectIndex >= 128) {
        collectIndex = 0;
        bufferReady = true;
    }
}

int freeRam() {
    extern int __heap_start, *__brkval;
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void updateDisplay() {
    display.clearDisplay();
    display.setCursor(10, 0);
    display.print("L");
    display.setCursor(31, 0);
    display.print("LM");
    display.setCursor(60, 0);
    display.print("M");
    display.setCursor(81, 0);
    display.print("HM");
    display.setCursor(110, 0);
    display.print("H");

    for (int i = 0; i < 5; ++i) {
        heights[i] = map(bands[i], 0, MAXMAGNITUDE, 0, 22);
        heights[i] = constrain(heights[i], 0, 22);
        smoothedHeights[i] = (smoothedHeights[i] * 3 + heights[i]) / 4;
        display.fillRect(i * 26, 32 - smoothedHeights[i], 25, smoothedHeights[i], SSD1306_WHITE);

        if (smoothedHeights[i] >= peakVol[i]) {
            peakVol[i] = smoothedHeights[i];
            peakTimer[i] = PEAKHOLDTIME;
        }
        else if (peakTimer[i] > 0) {
            --peakTimer[i];
        }
        else {
            peakVol[i] -= PEAKDECAY;
        }

        if (peakVol[i] != smoothedHeights[i]) display.drawFastHLine(i * 26 + 4, 32 - peakVol[i], 17, SSD1306_WHITE);
    }


    display.display();
}

void setup() {
    // put your setup code here, to run once:
    Serial.begin(9600);
    Serial.println(analogRead(A0));

    // PWM output for class D amp
    pinMode(9, OUTPUT);

    // Sets Timer 1 for PWM use
    // Sets fast PWM mode, no prescaler
    TCCR1A = _BV(COM1A1) | _BV(WGM11);
    TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10);

    ICR1 = 199; // TOP: 16 MHz / (199 + 1) = 80 kHz
    OCR1A = 99; // 50% duty cycle: (99 + 1) / (199 + 1) = 50%

    // Set up Timer 2 for ADC sampling, set to overflow 10000 times per second
    TCCR2A = 0;
    TCCR2B = _BV(CS21);
    TIMSK2 = _BV(TOIE2);
    TCNT2 = 55;

    // Sets ADC prescaler to 16 (faster samples)
    ADCSRA = (ADCSRA & 0xF8) | 0x04;

    // Disables pull-up resistors
    digitalWrite(A4, LOW);
    digitalWrite(A5, LOW);

    Serial.print("Free RAM before display: ");
    Serial.println(freeRam());

    // Ensures the correct i2c device address and sets voltage to 3.3v
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        while(1); // loops forever to prevent the rest of set up
    }

    display.clearDisplay(); // clear display ram
    display.setTextSize(1); // set text size to normal
    display.setTextColor(SSD1306_WHITE);  // sets text color to white
    display.setCursor(10, 0); // puts display cursor on (10, 0) for proper text placement
    display.println(F("Welcome, Home Audio by Nathaniel France & Brandon Pak"));  // creates data to display text
    display.display();  // puts data from above line into display ram

    delay(3000);  // sits for 3 seconds before moving to loop
}

void loop() {
    // put your main code here, to run repeatedly:

    if (bufferReady) {
        noInterrupts();
        int8_t* temp = collectBuffer;
        collectBuffer = processBuffer;
        processBuffer = temp;
        bufferReady = false;
        interrupts();

        fft.compute(processBuffer);
        fft.computeOutput[0].r = 0;
        fft.computeOutput[0].j = 0;
        fft.computeOutput[1].r = 0;
        fft.computeOutput[1].j = 0;
    }

    bands[0] = fft.getMagnitude(2, 3);
    bands[1] = fft.getMagnitude(4, 6);
    bands[2] = fft.getMagnitude(7, 25);
    bands[3] = fft.getMagnitude(26, 51);
    bands[4] = fft.getMagnitude(52, 64);

    for (int i = 0; i < 5; ++i) {
        if (bands[i] < 200) bands[i] = 0;
    }

    updateDisplay();

    for (int i = 0; i < 65; ++i) {
        if (fft.getMagnitude(i, i) > 1000) {
            Serial.print("Bin "); Serial.print(i);
            Serial.print(" mag "); Serial.println(fft.getMagnitude(i, i));
        }
    }
}