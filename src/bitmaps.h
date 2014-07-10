#ifndef BITMAPS_H_INCLUDED
#define BITMAPS_H_INCLUDED

#include "types.h"

Themes    ThemeBB[EXT_THEME_NB];
Bonuses   BonusBB[BONUS_NB];
Penalties PenaltyBB[PENALTY_NB];

namespace Bitmaps {

    void init() {

        ThemeBB[THEME_NONE] = 0;
        for (Theme t = GRIMSHAW; t < EXT_THEME_NB; ++t)
            ThemeBB[t] = 1ULL << (t-1);

        BonusBB[NO_BONUS] = 0;
        for (Bonus b = BONUS_1; b < BONUS_NB; ++b)
            BonusBB[b] = 1ULL << (b-1);

        PenaltyBB[NO_PENALTY] = 0;
        for (Penalty p = PENALTY_1MIN; p < PENALTY_NB; ++p)
            PenaltyBB[p] = 1ULL << (p-1);
    }
}

#endif // BITMAPS_H_INCLUDED
