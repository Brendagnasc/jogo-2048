/*
 * SDLController.cpp  —  Loop SDL2 não-bloqueante
 */
#include "SDLController.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

SDLController::SDLController()
    : logic_(std::make_unique<GameLogic<BOARD_N>>())
{
    auto ptr = std::make_unique<SDLRenderer>();
    sdl_  = ptr.get();
    rend_ = std::move(ptr);
    auto st = std::make_shared<StatsObserver>();
    stats_ = st;
    logic_->addObserver(std::make_shared<LogObserver>());
    logic_->addObserver(std::move(st));
    prev_ = logic_->grid();
}

void SDLController::run() {
    start();
    while (step()) {}
    finish();
}

void SDLController::start() {
    loadSave();
}

bool SDLController::step() {
    if (!running_) return false;
    char c = sdl_->pollInput();
    if (sdl_->closed()) return false;
    if (c != '\0') handleInput(c);
    if (!running_) return false;
    int best = std::max(save_.bestScore, logic_->best());
    rend_->render(logic_->grid(), logic_->score(), best, logic_->moves());
    if (logic_->hasWon() && !logic_->isOver())
        rend_->showMsg("Voce venceu! Continue jogando...");
    if (logic_->isOver())
        rend_->showMsg("Game Over!  Pressione R para reiniciar.", true);
    return true;
}

void SDLController::finish() {
    syncSave();
    writeSave();
    if (auto s = stats_.lock()) s->print();
    std::cout << "Recorde final: " << save_.bestScore << "\n";
}

void SDLController::handleInput(char c) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (auto dir = charToDir(c)) {
        prev_ = logic_->grid();
        if (logic_->applyMove(*dir)) {
            fireAnims(prev_, logic_->grid());
            saveBestIfImproved();
        } else
            rend_->showMsg("Nenhuma peca se moveu.", true);
        return;
    }
    switch (c) {
    case 'z':
        if (!logic_->undo()) rend_->showMsg("Nada para desfazer.", true);
        break;
    case 'r':
        syncSave();
        sdl_->pushHistory({logic_->score(), logic_->moves(), logic_->hasWon()});
        writeSave();
        logic_->reset();
        prev_ = logic_->grid();
        rend_->showMsg("Reiniciado!");
        break;
    case 'q':
        running_ = false;
        break;
    default: break;
    }
}

void SDLController::fireAnims(const Grid<BOARD_N>& before,
                               const Grid<BOARD_N>& after) {
    for (int r = 0; r < BOARD_N; r++) {
        for (int c = 0; c < BOARD_N; c++) {
            int vb = before[r][c];
            int va = after[r][c];
            if (!va) continue;
            if (!vb && (va == 2 || va == 4)) {
                sdl_->triggerPop(r, c);
            } else if (vb && va == vb * 2) {
                sdl_->triggerMerge(r, c);
            } else if (vb != va) {
                // Tile deslizou — procura célula de origem
                int fromCol = c, fromRow = r;
                for (int sc = 0; sc < BOARD_N; sc++)
                    if (sc != c && before[r][sc] == va) { fromCol = sc; break; }
                for (int sr = 0; sr < BOARD_N; sr++)
                    if (sr != r && before[sr][c] == va) { fromRow = sr; break; }
                sdl_->triggerSlide(r, c, sdl_->cx(fromCol), sdl_->cy(fromRow));
            }
        }
    }
}

void SDLController::loadSave() {
    if (auto d = persist_.load()) {
        save_ = *d;
        rend_->showMsg("Recorde anterior: " + std::to_string(save_.bestScore));
    }
}
void SDLController::writeSave() {
    persist_.save(save_);
#ifdef __EMSCRIPTEN__
    // copia o MEMFS para o IndexedDB do navegador
    EM_ASM( FS.syncfs(false, function (err) { if (err) console.warn("syncfs:", err); }); );
#endif
}

// No navegador a página pode ser fechada a qualquer momento, sem passar por
// finish(); então o recorde é gravado assim que ele é superado.
void SDLController::saveBestIfImproved() {
#ifdef __EMSCRIPTEN__
    if (logic_->best() > save_.bestScore) {
        save_.bestScore = logic_->best();
        writeSave();
    }
#endif
}
void SDLController::syncSave() {
    save_.bestScore = std::max(save_.bestScore, logic_->best());
    save_.totalGames++;
    if (auto s = stats_.lock()) save_.totalMerges += s->merges;
    if (logic_->hasWon()) save_.everReached2048 = true;
}
