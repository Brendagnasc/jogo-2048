/*
 * SDLApp.cpp — Loop SDL2 não-bloqueante
 */
#include "SDLApp.h"

#include <iostream>

SDLApp::SDLApp(GameSession::Options opts)
    : session_(std::move(opts))
{}

void SDLApp::run() {
    start();
    while (step()) {}
    finish();
}

void SDLApp::start() {
    rend_.showMsg(session_.loadProgress());
}

bool SDLApp::step() {
    if (!running_) return false;

    if (auto cmd = rend_.pollInput()) handle(*cmd);
    if (rend_.closed() || !running_) return false;

    const GameView view = session_.view();
    rend_.render(view);
    if (view.won && !view.over) rend_.showMsg("Voce venceu! Continue jogando...");
    if (view.over)              rend_.showMsg("Game Over!  Pressione R para reiniciar.", true);
    return true;
}

void SDLApp::finish() {
    session_.finish();
    const auto& s = session_.stats().session();
    std::cout << "Fusoes: " << s.merges << "  Undos: " << s.undos
              << "  Recorde: " << session_.saveData().bestScore << "\n";
}

void SDLApp::handle(Command cmd) {
    const auto out = session_.handle(cmd);

    if (out.turn) rend_.animate(out.turn->slide.tiles, out.turn->spawn);
    if (out.finishedGame)
        rend_.pushHistory({ out.finishedGame->score, out.finishedGame->moves, out.finishedGame->won });
    if (!out.message.empty()) rend_.showMsg(out.message, out.isError);
    if (out.quit) running_ = false;
}
