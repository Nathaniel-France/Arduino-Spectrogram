/*
 * Created and written by Nathaniel France
 * for ECE 2804 Home Audio Project
*/

#ifndef FFT_H
#define FFT_H

#include <stdint.h>
#include <math.h>
// #include <Arduino.h>

#define PI 3.1415926535897932384626433832795
#define SCALE 32767     // Scale factor for Q15

typedef int16_t q15;


struct complex_q15 {
    q15 r;
    q15 j;
};

class FFT {
public:
    explicit FFT(int size);
    ~FFT();

    // Math helper functions
    static q15 multQ15(q15 a, q15 b);
    static complex_q15 multComplex(complex_q15 a, complex_q15 b);
    static complex_q15 addComplex(complex_q15 a, complex_q15 b);
    static complex_q15 subComplex(complex_q15 a, complex_q15 b);
    
    void applyWindow(const int8_t* input);
    void applyBitRev();

    // Main computation function
    void compute(int8_t* input);

    uint16_t getMagnitude(int startBin, int endBin) const;


    int N;
    int numStages;

    complex_q15* computeOutput;
private:
};

#endif // FFT_H