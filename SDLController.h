#pragma once
#include "GameLogic.h"
#include "Observer.h"
#include "SDLRenderer.h"
#include "Persistence.h"
#include <memory>

class SDLController {
public:
    explicit SDLController();
    void run();
private:
    std::unique_ptr<GameLogic<BOARD_N>> logic_;
    SDLRenderer*                        sdl_{};
    std::unique_ptr<IRenderer>          rend_;
    std::weak_ptr<StatsObserver>        stats_;
    Persistence                         persist_{ defaultSavePath() };
    SaveData                            save_;
    bool                                running_{true};
    Grid<BOARD_N>                       prev_{};

    void handleInput(char c);
    void fireAnims(const Grid<BOARD_N>& before, const Grid<BOARD_N>& after);
    void loadSave();
    void writeSave();
    void syncSave();
};
