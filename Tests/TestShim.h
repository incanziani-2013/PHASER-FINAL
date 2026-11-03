#pragma once
// Shim mínimo de las pocas APIs de JUCE que usa el DSP, SOLO para probar el motor offline con g++.
// No forma parte del plugin. El comportamiento imita a juce::SmoothedValue / juce::Random.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <type_traits>
namespace juce
{
    template <class T> constexpr T jmax (T a, T b) { return a > b ? a : b; }
    template <class T> constexpr T jmin (T a, T b) { return a < b ? a : b; }
    template <class T> constexpr T jlimit (T lo, T hi, T v) { return v < lo ? lo : (v > hi ? hi : v); }
    inline int nextPowerOfTwo (int n) { int p = 1; while (p < n) p <<= 1; return p; }

    class Random
    {
    public:
        Random() : s (0x9E3779B97F4A7C15ull) {}
        explicit Random (int64_t seed) : s ((uint64_t) seed * 0x9E3779B97F4A7C15ull + 1) {}
        float nextFloat() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return (float) (s >> 40) / 16777216.0f; }
    private: uint64_t s;
    };

    struct ValueSmoothingTypes { struct Linear {}; struct Multiplicative {}; };

    template <typename F, typename T = ValueSmoothingTypes::Linear>
    class SmoothedValue
    {
    public:
        void reset (double sr, double sec) { steps = std::max (1, (int) std::floor (sec * sr)); setCurrentAndTargetValue (target); }
        void setCurrentAndTargetValue (F v) { cur = target = v; countdown = 0; }
        void setTargetValue (F v)
        {
            if (v == target) return;
            target = v; countdown = steps;
            if constexpr (std::is_same_v<T, ValueSmoothingTypes::Multiplicative>) step = std::exp ((std::log (std::max (target, (F) 1e-8)) - std::log (std::max (cur, (F) 1e-8))) / (F) countdown);
            else step = (target - cur) / (F) countdown;
        }
        F getNextValue() { if (countdown <= 0) return target; advance(); return cur; }
        F skip (int n) { if (n >= countdown) { cur = target; countdown = 0; return target; } for (int i = 0; i < n; ++i) advance(); return cur; }
        bool isSmoothing() const { return countdown > 0; }
        F getCurrentValue() const { return cur; }
        F getTargetValue() const { return target; }
    private:
        void advance() { --countdown; if (countdown == 0) cur = target; else if constexpr (std::is_same_v<T, ValueSmoothingTypes::Multiplicative>) cur *= step; else cur += step; }
        F cur {}, target {}, step {}; int steps = 1, countdown = 0;
    };
}
