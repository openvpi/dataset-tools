#pragma once

#include <QString>
#include <dstools/DsTextTypes.h>

namespace HFA {
    class HFA;
}

namespace Rmvpe {
    class Rmvpe;
}

namespace Game {
    class Game;
}

namespace dstools {

    class PhNumCalculator;

    struct AutoCompleteResult {
        DsTextDocument doc;
        bool modified = false;
    };

    AutoCompleteResult autoCompleteSlice(DsTextDocument doc, const QString &audioPath, HFA::HFA *hfa,
                                         Rmvpe::Rmvpe *rmvpe, Game::Game *game, PhNumCalculator *phNumCalc);

} // namespace dstools