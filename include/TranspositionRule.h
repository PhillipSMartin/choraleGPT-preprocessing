#pragma once
#include <map>

// rule for transposing current key to a new key in the circle of fifths 
// for example, to transpose B to a key with one extra sharp, we apply the rule {'F', 0, 1},
//   meaning, we change B to F, keep the same octave, and add an alteration (e.g., flat to natural or
//   natural to sharp)
struct TranspositionRule {
    char newPitch;      // the pitch we change to
    int octaveChange;   // the increment in the octave
    int accidentalChange;    // the increment in the accidental
};

static const std::map<char, TranspositionRule> transposeDownAFourthRules = {
    {'C', {'G', -1, 0}},
    {'D', {'A', -1, 0}},
    {'E', {'B', -1, 0}},
    {'F', {'C', 0, 0}},
    {'G', {'D', 0, 0}},
    {'A', {'E', 0, 0}},
    {'B', {'F', 0, 1}}
};

static const std::map<char, TranspositionRule> transposeUpAFifthRules = {
    {'C', {'G', 0, 0}},
    {'D', {'A', 0, 0}},
    {'E', {'B', 0, 0}},
    {'F', {'C', 1, 0}},
    {'G', {'D', 1, 0}},
    {'A', {'E', 1, 0}},
    {'B', {'F', 1, 1}}
};

static const std::map<char, TranspositionRule> transposeUpAFourthRules = {
    {'C', {'F', 0, 0}},
    {'D', {'G', 0, 0}},
    {'E', {'A', 0, 0}},
    {'F', {'B', 0, -1}},
    {'G', {'C', 1, 0}},
    {'A', {'D', 1, 0}},
    {'B', {'E', 1, 0}}
};

static const std::map<char, TranspositionRule> transposeDownAFifthRules = {
    {'C', {'F', -1, 0}},
    {'D', {'G', -1, 0}},
    {'E', {'A', -1, 0}},
    {'F', {'B', -1, -1}},
    {'G', {'C', 0, 0}},
    {'A', {'D', 0, 0}},
    {'B', {'E', 0, 0}}
};