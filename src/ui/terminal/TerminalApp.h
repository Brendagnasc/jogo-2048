#pragma once
/*
 * TerminalApp.h — Versão de terminal (camada ui)
 * ───────────────────────────────────────────────
 * Só faz entrada e saída: lê uma tecla, traduz para Command, entrega para a
 * GameSession e desenha o resultado. Nenhuma regra de jogo mora aqui.
 *
 * Injeção de dependência: recebe o renderer pronto (unique_ptr<IRenderer>),
 * então qualquer implementação de IRenderer serve.
 */

#include "../../app/GameSession.h"
#include "../../app/Renderer.h"

#include <memory>

class TerminalApp {
public:
    explicit TerminalApp(std::unique_ptr<IRenderer> renderer);
    void run();

private:
    std::unique_ptr<IRenderer> rend_;
    GameSession                session_;
    bool                       running_{true};

    void tick();
    void printSummary() const;
};

// Factory definida em TerminalRenderer.cpp
std::unique_ptr<IRenderer> createTerminalRenderer();
