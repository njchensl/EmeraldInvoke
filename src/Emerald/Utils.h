#pragma once

// round up util func
template <typename T, T multiple>
static T RoundUp(T numToRound) {
    if (multiple == 0) {
        return numToRound;
    }

    int remainder = numToRound % multiple;
    if (remainder == 0) {
        return numToRound;
    }

    return numToRound + multiple - remainder;
}
