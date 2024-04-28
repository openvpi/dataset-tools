#ifndef LYRICFA_MATHUTILS_H
#define LYRICFA_MATHUTILS_H

template<class T>
inline T divIntRound(T n, T d);

template<class T>
inline T divIntRound(T n, T d) {
    /*
     * Integer division rounding to the closest integer, without converting to floating point numbers.
     */
    // T should be an integer type (int, int64_t, qint64, ...)
    return ((n < 0) ^ (d < 0)) ? \
                               ((n - (d / 2)) / d) : \
                               ((n + (d / 2)) / d);
}

#endif // LYRICFA_MATHUTILS_H
