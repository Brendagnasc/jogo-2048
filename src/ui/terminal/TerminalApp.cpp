/*
 * TerminalApp.cpp — Loop da versão de terminal
 */
#include "TerminalApp.h"

#include <iostream>

TerminalApp::TerminalApp(std::unique_ptr<IRenderer> renderer)
    : rend_(std::move(renderer))
{}

void TerminalApp::run() {
    std::cout << "\n  " << session_.loadProgress() << "\n";
    rend_->showHelp();
    std::cout << "  Pressione Enter para iniciar...";
    std::cin.ignore();

    while (running_) tick();

    session_.finish();
    printSummary();
}

void TerminalApp::tick() {
    const GameView view = session_.view();
    rend_->render(view);
    if (view.won && !view.over)
        rend_->showMsg("\xe2\x98\x85  Voce venceu! Continue para mais pontos.");
    if (view.over)
        rend_->showMsg("Game Over! Pressione R para reiniciar.", true);

    std::cout << "  > " << std::flush;
    char key;
    if (!(std::cin >> key)) { running_ = false; return; }

    const auto cmd = commandFromKey(key);
    if (!cmd) {
        rend_->showMsg("Tecla invalida. Use WASD, Z, R ou Q.", true);
        return;
    }

    const auto out = session_.handle(*cmd);
    if (!out.message.empty()) rend_->showMsg(out.message, out.isError);
    if (out.quit) running_ = false;
}

void TerminalApp::printSummary() const {
    const auto& s = session_.stats().session();
    std::cout << "\n  \033[1m── Estatísticas ────────────────\033[0m\n"
              << "  Fusões realizadas : " << s.merges           << "\n"
              << "  Pontos por fusões : " << s.pointsFromMerges << "\n"
              << "  Undos usados      : " << s.undos            << "\n"
              << "\n  Recorde: " << session_.saveData().bestScore << "\n\n";
}
