#include <JuceHeader.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <limits>
#include <type_traits>

// Function to verify the mathematical stability of a Direct Form 1 / 2 Biquad filter
// Based on the denominator: 1 + a1*z^-1 + a2*z^-2
// Note: JUCE defines the denominator coefficients with a positive or negative sign depending on the structure.
// For the standard stability rule: |a2| < 1 and |a1| < 1 + a2
template <typename T>
bool isStable(T a1, T a2) {
    return (std::abs(a2) < 1.0) && (std::abs(a1) < (1.0 + a2));
}
// Custom Biquad filter implementation using Transposed Direct Form 2 (TDF2)
template <typename T>
class CustomBiquadTDF2 {
public:
    CustomBiquadTDF2() {
		// Initialize the filter state variables
        reset();
    }
	// Set the filter coefficients
    void setCoefficients(T b0, T b1, T b2, T a1, T a2) {
        b0_ = b0;
        b1_ = b1;
        b2_ = b2;
        a1_ = a1;
        a2_ = a2;
        reset();
    }
	// Process a single sample through the filter
    T process(T x) {
        T y = b0_ * x + s1_;
		// Check for numerical stability: if y is NaN or Inf, reset the filter state and return 0
        if (!std::isfinite(y)) {
            reset();
            return static_cast<T>(0.0);
        }
		// Update the state variables for the next sample
        s1_ = b1_ * x - a1_ * y + s2_;
        s2_ = b2_ * x - a2_ * y;
        return y;
    }
	// Reset the filter state variables
    void reset() {
        s1_ = s2_ = static_cast<T>(0.0);
    }
// Get the current state variables (for debugging or analysis)
private:
    T b0_ = 0, b1_ = 0, b2_ = 0;
    T a1_ = 0, a2_ = 0;
    T s1_ = 0, s2_ = 0;
};

// Function that runs the unit impulse test for a given frequency
template <typename T>
void runImpulseTest(double cutoffFreq, double sampleRate) {
	// 1. Check if the cutoff frequency is above the Nyquist frequency
    if (cutoffFreq <= 0.0 || cutoffFreq >= sampleRate / 2.0)
    {
        std::cout << "  Frequency: " << std::setw(7) << cutoffFreq
                  << " Hz | Skipped (outside valid range)" << std::endl;
        return;
    }
    // 2. Calculate coefficients using JUCE's DSP tools (e.g., 2nd-order Low-Pass Filter)
    // We use juce::dsp::IIR::Coefficients to obtain the corresponding values
	// makeLowPass, makeHighPass, makeBandPass, etc., can be used to generate the coefficients for different filter types.
    //auto coefficients = juce::dsp::IIR::Coefficients<T>::makeLowPass(sampleRate, cutoffFreq);
    auto coefficients = juce::dsp::IIR::Coefficients<T>::makePeakFilter(sampleRate, cutoffFreq, 10.0, 15.84);
    // JUCE normalises a0 and stores the coefficients as b0, b1, b2, a1, a2.
    const auto* rawCoefficients = coefficients->getRawCoefficients();
    T a1 = rawCoefficients[3];
    T a2 = rawCoefficients[4];
    T b0 = rawCoefficients[0];
    T b1 = rawCoefficients[1];
    T b2 = rawCoefficients[2];

	double real_a1 = static_cast<double>(a1);
	double real_a2 = static_cast<double>(a2);
    // 3. Validate Stability
    bool stable = (std::abs(real_a2) < 1.0)
               && (std::abs(real_a1) < (1.0 + real_a2));

    const int numSamples = 500000;

    T customMaxAbsY = 0.0;
    T customFinalY = 0.0;
    CustomBiquadTDF2<T> customFilter;
    customFilter.setCoefficients(b0, b1, b2, a1, a2);

    for (int n = 0; n < numSamples; ++n) {
        T x = (n == 0) ? static_cast<T>(1.0) : static_cast<T>(0.0);

        T y = customFilter.process(x);
        if (std::abs(y) > customMaxAbsY) {
            customMaxAbsY = std::abs(y);
        }

        if (n == numSamples - 1) {
            customFinalY = y;
        }
    }

    juce::dsp::IIR::Filter<T> juceFilter;
    juceFilter.coefficients = coefficients;
    T juceMaxAbsY = 0.0;
    T juceFinalY = 0.0;

    for (int n = 0; n < numSamples; ++n) {
        T x = (n == 0) ? static_cast<T>(1.0) : static_cast<T>(0.0);
        T y = juceFilter.processSample(x);

        if (std::abs(y) > juceMaxAbsY) {
            juceMaxAbsY = std::abs(y);
        }

        if (n == numSamples - 1) {
            juceFinalY = y;
        }
    }

    std::cout << "  Frequency: " << std::setw(7) << cutoffFreq << " Hz" << std::endl;
    const char* precisionName = std::is_same_v<T, float> ? "float" : "double";
    std::cout << "    Custom " << precisionName
              << " | Mathematical stability: " << (stable ? "YES" : "NO")
              << " | Max |y|: " << std::scientific << std::setprecision(4) << customMaxAbsY
              << " | Final y: " << customFinalY << std::endl;
    std::cout << "    JUCE   " << precisionName
              << " | Mathematical stability: " << (stable ? "YES" : "NO")
              << " | Max |y|: " << juceMaxAbsY
              << " | Final y: " << juceFinalY
              << std::defaultfloat << std::endl;
}

int main (int argc, char* argv[]) {
    const double sampleRate = 44100.0;
    
    // List of frequencies for the sweep from both ends
    std::vector<double> frequencies = {
        0.5, 1.0, 1.3, 1.5, 3.0, 20.0, 50.0, 100.0, 1000.0, 10000.0, 20000.0, 21000.0, 22000.0, 22050.0, 24000.0, 30000.0, 44100.0
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
