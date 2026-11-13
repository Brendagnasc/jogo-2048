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

    // Um quadro do loop (entrada + desenho). Retorna false quando o jogo termina.
    // No desktop é chamado por run(); no navegador, pelo
    // emscripten_set_main_loop, já que o navegador não permite loop bloqueante.
    bool step();
    void start();   // carrega o save antes do primeiro quadro
    void finish();  // grava o save e mostra as estatísticas

    void handleInput(char c);
private:
    std::unique_ptr<GameLogic<BOARD_N>> logic_;
    SDLRenderer*                        sdl_{};
    std::unique_ptr<IRenderer>          rend_;
    std::weak_ptr<StatsObserver>        stats_;
    Persistence                         persist_{ defaultSavePath() };
    SaveData                            save_;
    bool                                running_{true};
    Grid<BOARD_N>                       prev_{};

    void fireAnims(const Grid<BOARD_N>& before, const Grid<BOARD_N>& after);
    void loadSave();
    void writeSave();
    void syncSave();
    void saveBestIfImproved();
};
