#pragma once
/*
 * Renderer.h — Contrato das interfaces gráficas (camada app, padrão Strategy)
 * ───────────────────────────────────────────────────────────────────────────
 * A GameSession produz um GameView (o que mostrar); cada interface decide
 * COMO mostrar. Terminal e SDL implementam IRenderer.
 */

#include "../core/Types.h"

#include <string>

// ── GameView: tudo o que a tela precisa, num pacote só ───────────────────────
// Antes render() recebia 4 parâmetros soltos; acrescentar "venceu" ou
// "fim de jogo" exigiria mudar a assinatura em todos os renderers.
struct GameView {
    const Grid<BOARD_N>* grid{nullptr};
    int  score{0};
    int  best{0};
    int  moves{0};
    bool won{false};
    bool over{false};
};

class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual void render(const GameView& view) = 0;

    // Mensagem de status (erro ou sucesso)
    virtual void showMsg(const std::string& msg, bool err = false) = 0;

    // Painel de ajuda — opcional (o SDL mostra a ajuda no próprio cabeçalho)
    virtual void showHelp() {}
};
