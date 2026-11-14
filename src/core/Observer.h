#pragma once
/*
 * Observer.h — Eventos do jogo e interface de observador (camada core)
 * ─────────────────────────────────────────────────────────────────────
 * Padrão Observer: o jogo emite eventos; quem quiser reagir (estatísticas,
 * log, som...) se registra. Adicionar um comportamento novo não toca no Game.
 *
 * Aqui fica SÓ o contrato. Observadores concretos que escrevem no console
 * (LogObserver) ou acumulam números (StatsObserver) ficam na camada app,
 * para que o core não dependa de <iostream>.
 */

struct GameEvent {
    enum class Type { Move, Merge, NewTile, Win, GameOver, Undo, Reset };

    Type type;
    int  value{0};   // Merge: pontos da fusão · NewTile: valor da peça · Win: WIN_TILE
    int  score{0};   // pontuação depois do evento
};

class IObserver {
public:
    virtual ~IObserver() = default;
    virtual void onEvent(const GameEvent&) = 0;
};
