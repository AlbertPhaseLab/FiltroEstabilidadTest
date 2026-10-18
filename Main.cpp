#include <JuceHeader.h>
#include <algorithm>
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

template <typename T>
struct BiquadCoefficients {
    T b0 = 0;
    T b1 = 0;
    T b2 = 0;
    T a1 = 0;
    T a2 = 0;
};

template <typename T>
class CustomBiquadDF1 {
public:
	CustomBiquadDF1() {
		reset();
	}
	void setCoefficients(T b0, T b1, T b2, T a1, T a2) {
		b0_ = b0;
		b1_ = b1;
		b2_ = b2;
        a1_ = a1;
        a2_ = a2;
		reset();
	}
	T process(T x) {
        T y = b0_ * x + b1_ * x1_ + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;
        // Check for numerical stability: if y is NaN or Inf, reset the filter state and return 0
		if (!std::isfinite(y)) {
			reset();
			return static_cast<T>(0.0);
		}
		x2_ = x1_; x1_ = x;
		y2_ = y1_; y1_ = y;
		return y;
	}
	void reset() {
		x1_ = x2_ = y1_ = y2_ = static_cast<T>(0.0);
	}
private:
	T b0_ = 0, b1_ = 0, b2_ = 0;
	T a1_ = 0, a2_ = 0;
	T x1_ = 0, x2_ = 0; // Previous input samples
	T y1_ = 0, y2_ = 0; // Previous output samples

};

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

static BiquadCoefficients<double> calculateDoubleCoefficients(double sampleRate,
                                                               double cutoffFreq)
{
    auto coefficients = juce::dsp::IIR::Coefficients<double>::makePeakFilter(
        sampleRate, cutoffFreq, 10.0, 15.84);
    const auto* rawCoefficients = coefficients->getRawCoefficients();

    return { rawCoefficients[0], rawCoefficients[1], rawCoefficients[2],
             rawCoefficients[3], rawCoefficients[4] };
}

template <typename T>
BiquadCoefficients<T> convertCoefficients(const BiquadCoefficients<double>& coefficients)
{
    return { static_cast<T>(coefficients.b0), static_cast<T>(coefficients.b1),
             static_cast<T>(coefficients.b2), static_cast<T>(coefficients.a1),
             static_cast<T>(coefficients.a2) };
}

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

    const auto doubleCoefficients = calculateDoubleCoefficients(sampleRate, cutoffFreq);
    const auto coefficients = convertCoefficients<T>(doubleCoefficients);

    double real_a1 = static_cast<double>(coefficients.a1);
    double real_a2 = static_cast<double>(coefficients.a2);
    // 3. Validate Stability
    bool stable = (std::abs(real_a2) < 1.0)
               && (std::abs(real_a1) < (1.0 + real_a2));
    
    const int numSamples = 500000;
    
	T customMaxAbsYDF1 = 0.0;
	T customFinalYDF1 = 0.0;
	CustomBiquadDF1<T> customFilterDF1;
    customFilterDF1.setCoefficients(coefficients.b0, coefficients.b1, coefficients.b2,
                                    coefficients.a1, coefficients.a2);

    T customMaxAbsYTDF2 = 0.0;
    T customFinalYTDF2 = 0.0;
    CustomBiquadTDF2<T> customFilterTDF2;
    customFilterTDF2.setCoefficients(coefficients.b0, coefficients.b1, coefficients.b2,
                                      coefficients.a1, coefficients.a2);

    auto juceCoefficients = new juce::dsp::IIR::Coefficients<T>(
        coefficients.b0, coefficients.b1, coefficients.b2,
        static_cast<T>(1.0), coefficients.a1, coefficients.a2);
    juce::dsp::IIR::Filter<T> juceFilter(juceCoefficients);
    T juceMaxAbsY = 0.0;
    T juceFinalY = 0.0;
    T maxDifferenceDF1 = 0.0;
    T maxDifferenceTDF2 = 0.0;

	// 4. Process the unit impulse through both filters and track the maximum absolute output
    for (int n = 0; n < numSamples; n++)
    {
        // Generate the unit impulse input (calculated once for both filters)
		T x = (n == 0) ? static_cast<T>(1.0) : static_cast<T>(0.0);
		
        // Process the input through the custom Direct Form 1 filter
		T yDF1 = customFilterDF1.process(x);
        if (std::abs(yDF1) > customMaxAbsYDF1)
        {
            customMaxAbsYDF1 = std::abs(yDF1);
        }

		// Process the input through the custom Transposed Direct Form 2 filter
		T yTDF2 = customFilterTDF2.process(x);
        if (std::abs(yTDF2) > customMaxAbsYTDF2)
        {
            customMaxAbsYTDF2 = std::abs(yTDF2);
        }
        
		// Process the input through the JUCE filter
		T yJuce = juceFilter.processSample(x);
		if (std::abs(yJuce) > juceMaxAbsY)
		{
			juceMaxAbsY = std::abs(yJuce);
		}

        const T differenceDF1 = (!std::isfinite(yDF1) || !std::isfinite(yJuce))
                              ? std::numeric_limits<T>::infinity()
                              : std::abs(yDF1 - yJuce);
        const T differenceTDF2 = (!std::isfinite(yTDF2) || !std::isfinite(yJuce))
                               ? std::numeric_limits<T>::infinity()
                               : std::abs(yTDF2 - yJuce);
        maxDifferenceDF1 = std::max(maxDifferenceDF1, differenceDF1);
        maxDifferenceTDF2 = std::max(maxDifferenceTDF2, differenceTDF2);

		// Store the final output values after processing all samples
		if (n == numSamples - 1)
		{
            customFinalYDF1 = yDF1;
			customFinalYTDF2 = yTDF2;
			juceFinalY = yJuce;
		}
    }

	// 5. Output the results

    std::cout << "  Frequency: " << std::setw(7) << cutoffFreq << " Hz" << std::endl;
    const char* precisionName = std::is_same_v<T, float> ? "float" : "double";

    std::cout << "    Custom DF1 " << precisionName
        << " | Mathematical stability: " << (stable ? "YES" : "NO")
		<< " | Max |y|: " << std::scientific << std::setprecision(4) << customMaxAbsYDF1
        << " | Final y: " << customFinalYDF1
        << " | Max diff vs JUCE: " << maxDifferenceDF1 << std::endl;

    std::cout << "    Custom TDF2 " << precisionName
              << " | Mathematical stability: " << (stable ? "YES" : "NO")
              << " | Max |y|: " << std::scientific << std::setprecision(4) << customMaxAbsYTDF2
              << " | Final y: " << customFinalYTDF2
              << " | Max diff vs JUCE: " << maxDifferenceTDF2 << std::endl;

    std::cout << "    JUCE   " << precisionName
              << " | Mathematical stability: " << (stable ? "YES" : "NO")
              << " | Max |y|: " << juceMaxAbsY
              << " | Final y: " << juceFinalY
              << std::defaultfloat << std::endl;

    if constexpr (std::is_same_v<T, double>)
    {
        std::cout << "  Coefficients calculated in double:" << std::endl;
        std::cout << std::setprecision(16)
            << "    b0=" << doubleCoefficients.b0 << " b1=" << doubleCoefficients.b1
            << " b2=" << doubleCoefficients.b2 << " a1=" << doubleCoefficients.a1
            << " a2=" << doubleCoefficients.a2 << std::endl;
        const auto floatCoefficients = convertCoefficients<float>(doubleCoefficients);
        std::cout << "  Same coefficients converted to float:" << std::endl;
        std::cout << std::setprecision(9)
            << "    b0=" << floatCoefficients.b0 << " b1=" << floatCoefficients.b1
            << " b2=" << floatCoefficients.b2 << " a1=" << floatCoefficients.a1
            << " a2=" << floatCoefficients.a2 << std::endl;
    }
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
