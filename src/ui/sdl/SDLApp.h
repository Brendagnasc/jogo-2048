#pragma once
/*
 * SDLApp.h — Versão gráfica SDL2 (camada ui)
 * ───────────────────────────────────────────
 * Igual à versão de terminal, só faz entrada/saída: pede um Command ao
 * SDLRenderer, entrega para a GameSession e anima/desenha o resultado.
 *
 * O loop é dividido em start/step/finish porque o navegador não permite um
 * while bloqueante: no desktop run() chama step() em loop; na versão web,
 * main.cpp registra step() no emscripten_set_main_loop.
 */

#include "../../app/GameSession.h"
#include "SDLRenderer.h"

class SDLApp {
public:
    explicit SDLApp(GameSession::Options opts = GameSession::Options{});

    void run();
    void start();    // carrega o save antes do primeiro quadro
    bool step();     // um quadro; false quando o jogo termina
    void finish();   // grava o save e mostra as estatísticas

    void handle(Command cmd);

private:
    SDLRenderer rend_;
    GameSession session_;
    bool        running_{true};
};
