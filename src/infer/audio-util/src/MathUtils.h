/// @file MathUtils.h
/// @brief Integer division with rounding utility.

#pragma once
/// @brief Integer division rounding to the closest integer without float conversion.
/// @tparam T Integer type (int, int64_t, etc.).
/// @param n Numerator.
/// @param d Denominator.
/// @return Rounded quotient.
template<class T>
inline T divIntRound(T n, T d);

template<class T>
inline T divIntRound(T n, T d) {
    return ((n < 0) ^ (d < 0)) ? \
                               ((n - (d / 2)) / d) : \
                               ((n + (d / 2)) / d);
}
