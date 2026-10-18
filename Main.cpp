#include <JuceHeader.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>

// Function to verify the mathematical stability of a Direct Form 1 / 2 Biquad filter
// Based on the denominator: 1 + a1*z^-1 + a2*z^-2
// Note: JUCE defines the denominator coefficients with a positive or negative sign depending on the structure.
// For the standard stability rule: |a2| < 1 and |a1| < 1 + a2
template <typename T>
bool isStable(T a1, T a2) {
    return (std::abs(a2) < 1.0) && (std::abs(a1) < (1.0 + a2));
}

// Function that runs the unit impulse test for a given frequency
template <typename T>
void runImpulseTest(double cutoffFreq, double sampleRate) {
    // 1. Calculate coefficients using JUCE's DSP tools (e.g., 2nd-order Low-Pass Filter)
    // We use juce::dsp::IIR::Coefficients to obtain the corresponding values
    auto coefficients = juce::dsp::IIR::Coefficients<T>::makeLowPass(sampleRate, cutoffFreq);
    
    // JUCE stores the coefficients as b0, b1, b2, a0, a1, a2.
    const auto* rawCoefficients = coefficients->getRawCoefficients();
    T a1 = rawCoefficients[4];
    T a2 = rawCoefficients[5];
    T b0 = rawCoefficients[0];
    T b1 = rawCoefficients[1];
    T b2 = rawCoefficients[2];

    // 2. Validate Stability
    bool stable = isStable(a1, a2);

    // 3 and 4. Inject a Unit Impulse and Monitor the Output
    T x1 = 0, x2 = 0; // Past states of the entry
    T y1 = 0, y2 = 0; // Past states of the output
    
    T maxAbsY = 0.0;
    T finalY = 0.0;
    const int numSamples = 10000;

    for (int n = 0; n < numSamples; ++n) {
        // Unit impulse: 1.0 in the first sample, 0.0 otherwise
        T x = (n == 0) ? 1.0f : 0.0f; 

        // Difference equation (Standard Direct Form 1)
        // y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
        T y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

        // Register the maximum absolute value
        if (std::abs(y) > maxAbsY) {
            maxAbsY = std::abs(y);
        }

        // Update states
        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = y;

        // Register the final value in the last sample
        if (n == numSamples - 1) {
            finalY = y;
        }
    }

    // Print the formatted results
    std::cout << "  Frequency: " << std::setw(7) << cutoffFreq << " Hz | "
              << "Stable: " << (stable ? "YES" : "NO") << " | "
              << "Max |y|: " << std::scientific << std::setprecision(4) << maxAbsY << " | "
              << "and final: " << finalY << std::defaultfloat << std::endl;
}

int main (int argc, char* argv[]) {
    const double sampleRate = 44100.0;
    
    // List of frequencies for the sweep from both ends
    std::vector<double> frequencies = {
        20.0, 50.0, 100.0, 1000.0, 10000.0, 20000.0, 21000.0, 22000.0, 22050.0, 24000.0, 30000.0, 44100.0
    };

    std::cout << "========================================" << std::endl;
    std::cout << "SINGLE-PRECISION TESTS (float)" << std::endl;
    std::cout << "========================================" << std::endl;
    for (double freq : frequencies) {
        runImpulseTest<float>(freq, sampleRate);
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "DOUBLE-PRECISION TESTS (double)" << std::endl;
    std::cout << "========================================" << std::endl;
    for (double freq : frequencies) {
        runImpulseTest<double>(freq, sampleRate);
    }

    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();

    return 0;
}
