#pragma once
/*
 * Renderer.h — Interface de renderização (padrão Strategy)
 * ──────────────────────────────────────────────────────────
 * CONCEITO: Padrão Strategy
 *   Define uma família de algoritmos (formas de renderizar), encapsula cada um
 *   e os torna intercambiáveis. O Controller usa IRenderer* sem saber qual
 *   implementação está por trás — TerminalRenderer, QtRenderer, WebRenderer, etc.
 *
 * CONCEITO: Interface pura vs Classe abstrata
 *   Interface pura: SOMENTE métodos virtuais puros (= 0). Não tem estado.
 *   Classe abstrata: mistura métodos virtuais puros com implementações.
 *   IRenderer é uma interface pura — apenas define o contrato.
 *
 * CONCEITO: Separar declaração de implementação
 *   Renderer.h:   declara IRenderer (interface) — todos que usam incluem este
 *   Renderer.cpp: define TerminalRenderer — apenas quem implementa inclui este
 *   main.cpp: inclui apenas Renderer.h — não precisa saber dos detalhes internos
 */

#include "Types.h"
#include <string>

// ── IRenderer: interface pura ─────────────────────────────────────────────────
class IRenderer {
public:
    virtual ~IRenderer() = default;

    // Renderiza o estado completo do jogo
    virtual void render(const Grid<BOARD_N>& grid,
                        int score, int best, int moves) = 0;

    // Exibe uma mensagem de status (erro ou sucesso)
    // 'bool err = false': parâmetro com valor padrão — caller pode omiti-lo
    virtual void showMsg(const std::string& msg, bool err = false) = 0;

    // Exibe o painel de ajuda/controles
    virtual void showHelp() = 0;
};
