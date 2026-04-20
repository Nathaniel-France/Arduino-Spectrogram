#pragma GCC optimize("O3")

#include "FFT.h"
#include <string.h>
#include <Arduino.h>
//#include <iostream>

#include <avr/pgmspace.h>

const q15 twiddle_r[] PROGMEM = {32767, 32728, 32609, 32412, 32137, 31785, 31356, 30852, 30273, 29621, 28898, 28105, 27245, 26319, 25329, 24279, 23170, 22005, 20787, 19519, 18204, 16846, 15446, 14010, 12539, 11039, 9512, 7962, 6393, 4808, 3212, 1608, 0, -1608, -3212, -4808, -6393, -7962, -9512, -11039, -12539, -14010, -15446, -16846, -18204, -19519, -20787, -22005, -23170, -24279, -25329, -26319, -27245, -28105, -28898, -29621, -30273, -30852, -31356, -31785, -32137, -32412, -32609, -32728};

const q15 twiddle_j[] PROGMEM = {0, -1608, -3212, -4808, -6393, -7962, -9512, -11039, -12539, -14010, -15446, -16846, -18204, -19519, -20787, -22005, -23170, -24279, -25329, -26319, -27245, -28105, -28898, -29621, -30273, -30852, -31356, -31785, -32137, -32412, -32609, -32728, -32767, -32728, -32609, -32412, -32137, -31785, -31356, -30852, -30273, -29621, -28898, -28105, -27245, -26319, -25329, -24279, -23170, -22005, -20787, -19519, -18204, -16846, -15446, -14010, -12539, -11039, -9512, -7962, -6393, -4808, -3212, -1608};

const q15 window[] PROGMEM = {0, 20, 80, 180, 320, 498, 716, 972, 1266, 1597, 1964, 2367, 2803, 3273, 3775, 4308, 4870, 5461, 6078, 6721, 7387, 8075, 8783, 9510, 10254, 11013, 11785, 12568, 13361, 14161, 14967, 15775, 16586, 17396, 18203, 19006, 19803, 20591, 21369, 22135, 22886, 23622, 24340, 25038, 25716, 26370, 27000, 27604, 28181, 28729, 29246, 29732, 30186, 30605, 30990, 31339, 31652, 31927, 32164, 32362, 32522, 32642, 32722, 32762, 32762, 32722, 32642, 32522, 32362, 32164, 31927, 31652, 31339, 30990, 30605, 30186, 29732, 29246, 28729, 28181, 27604, 27000, 26370, 25716, 25038, 24340, 23622, 22886, 22135, 21369, 20591, 19803, 19006, 18203, 17396, 16586, 15775, 14967, 14161, 13361, 12568, 11785, 11013, 10254, 9510, 8783, 8075, 7387, 6721, 6078, 5461, 4870, 4308, 3775, 3273, 2803, 2367, 1964, 1597, 1266, 972, 716, 498, 320, 180, 80, 20, 0};

const uint8_t bit_reversal[] PROGMEM = {0, 64, 32, 96, 16, 80, 48, 112, 8, 72, 40, 104, 24, 88, 56, 120, 4, 68, 36, 100, 20, 84, 52, 116, 12, 76, 44, 108, 28, 92, 60, 124, 2, 66, 34, 98, 18, 82, 50, 114, 10, 74, 42, 106, 26, 90, 58, 122, 6, 70, 38, 102, 22, 86, 54, 118, 14, 78, 46, 110, 30, 94, 62, 126, 1, 65, 33, 97, 17, 81, 49, 113, 9, 73, 41, 105, 25, 89, 57, 121, 5, 69, 37, 101, 21, 85, 53, 117, 13, 77, 45, 109, 29, 93, 61, 125, 3, 67, 35, 99, 19, 83, 51, 115, 11, 75, 43, 107, 27, 91, 59, 123, 7, 71, 39, 103, 23, 87, 55, 119, 15, 79, 47, 111, 31, 95, 63, 127};

// Constructor to set number of points
FFT::FFT(const int size) {
    N = size;
    numStages = 7;

    computeOutput = new complex_q15[N];
}

// Destructor, prevents memory leaks from the lookup tables
FFT::~FFT() {
    delete[] computeOutput;
}

/**
 * 
 * 
 * 
 * @param   a Multiplicand
 * @param   b Multiplier
 * @return    The result of the multiplication
*/
q15 FFT::multQ15(const q15 a, const q15 b) {
    int32_t result = (int32_t)a * b;
    return (q15)(result >> 15);
}

// Complex addition function
complex_q15 FFT::addComplex(const complex_q15 a, const complex_q15 b) {
    complex_q15 ret = {0,0};
    ret.r = static_cast<q15>((static_cast<int32_t>(a.r) + b.r));
    ret.j = static_cast<q15>((static_cast<int32_t>(a.j) + b.j));
    return ret;
}

// Complex subtraction function
complex_q15 FFT::subComplex(const complex_q15 a, const complex_q15 b) {
    complex_q15 ret = {0,0};
    ret.r = static_cast<q15>((static_cast<int32_t>(a.r) - b.r));
    ret.j = static_cast<q15>((static_cast<int32_t>(a.j) - b.j));
    return ret;
}

// Complex multiplication function
complex_q15 FFT::multComplex(const complex_q15 a, const complex_q15 b) {
    complex_q15 ret{};
    ret.r = static_cast<q15>(((int32_t)a.r * b.r - (int32_t)a.j * b.j) >> 15);
    ret.j = static_cast<q15>(((int32_t)a.r * b.j + (int32_t)a.j * b.r) >> 15);
    return ret;
}

/**
 * Takes a regular q15 array and turns it into an array
 * of complex_q15 type with j = 0 for all entries.
 *
 * @param input input q15 array
 */
void FFT::applyWindow(const int8_t* input) {
    for (int i = 0; i < N; ++i) {
        computeOutput[i].r =  multQ15((int16_t)input[i] * 256, pgm_read_word(&window[i])) >> 7;
        computeOutput[i].j = 0;
    }
}

/**
 * 
*/
void FFT::applyBitRev() {
    for (int i = 0; i < N; ++i) {
    uint8_t j = pgm_read_byte(&bit_reversal[i]);
    if (j > i) {
        complex_q15 temp = computeOutput[i];
        computeOutput[i] = computeOutput[j];
        computeOutput[j] = temp;
    }
}
}

/**
 * Main compute function.  Starts by applying the Hann window
 * to set the input correctly.  Then bit reverses the input.
 * Then puts input through butterfly compute stages.
 *
 * @param input Input samples from ADC
 * @param computeOutput Pointer to output array
 */
void FFT::compute(int8_t* input) {
    memset(computeOutput, 0, sizeof(complex_q15) * N);

    // Apply Hann window
    applyWindow(input);

    // Bit Reversal
    applyBitRev();

    // Butterfly stages
    for (int stage = 0; stage < numStages; ++stage) {
        int groupSize = 1 << (stage + 1);
        int half = groupSize / 2;
        int twiddleStep = N / groupSize;

        for (int group = 0; group < N; group += groupSize) {
            for (int i = 0; i < half; ++i) {
                int angleIndex = i * twiddleStep;
                //Serial.print("Angle Index: "); Serial.println(angleIndex);

                complex_q15 top = computeOutput[group + i];
                complex_q15 bottom = computeOutput[group + i + half];

                complex_q15 w;
                w.r = pgm_read_word(&twiddle_r[angleIndex]);
                w.j = pgm_read_word(&twiddle_j[angleIndex]);;
                complex_q15 temp = multComplex(w, bottom);

                computeOutput[group + i] = addComplex(top, temp);
                computeOutput[group + i + half] = subComplex(top, temp);
            }
        }
    }
}

/**
 *
 */
uint16_t FFT::getMagnitude(int startBin, int endBin) const {
    long sum = 0;
    for (int i = startBin; i <= endBin; ++i) {
        long real = (long)computeOutput[i].r * computeOutput[i].r;
        long imag = (long)computeOutput[i].j * computeOutput[i].j;
        sum += (long)sqrt((double)(real + imag));
    }
    return (uint16_t)(sum / (endBin - startBin + 1));
}