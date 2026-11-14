#pragma once
/*
 * Observers.h — Observadores concretos (camada app)
 * ──────────────────────────────────────────────────
 * Implementações de IObserver que não são regra de jogo: estatísticas e log.
 * Ficam fora do core para que a lógica não dependa de <iostream>.
 */

#include "../core/Observer.h"
#include "../core/Types.h"

#include <chrono>
#include <iostream>
#include <sstream>

// ── StatsObserver: conta fusões e undos ───────────────────────────────────────
// Mantém dois contadores: da partida atual (zerado a cada partida, usado para
// o save) e da sessão inteira (mostrado ao sair). Antes havia um só, e o save
// somava de novo as fusões das partidas anteriores a cada reinício.
class StatsObserver final : public IObserver {
public:
    struct Counters {
        int merges{0};
        int pointsFromMerges{0};
        int undos{0};
    };

    void onEvent(const GameEvent& e) override {
        using T = GameEvent::Type;
        if (e.type == T::Merge) { bump(&Counters::merges, 1); bump(&Counters::pointsFromMerges, e.value); }
        if (e.type == T::Undo)  { bump(&Counters::undos, 1); }
    }

    const Counters& game()    const { return game_; }
    const Counters& session() const { return session_; }
    void startNewGame() { game_ = {}; }

private:
    Counters game_, session_;

    // ponteiro para membro: soma 'n' no mesmo campo dos dois contadores
    void bump(int Counters::*field, int n) {
        game_.*field    += n;
        session_.*field += n;
    }
};

// ── LogObserver: escreve os eventos importantes no stderr ────────────────────
class LogObserver final : public IObserver {
public:
    void onEvent(const GameEvent& e) override {
        using T = GameEvent::Type;
        std::ostringstream oss;
        switch (e.type) {
            case T::Merge:    oss << "Merge +" << e.value << " (score=" << e.score << ")"; break;
            case T::Win:      oss << "WIN! Reached " << WIN_TILE;                          break;
            case T::GameOver: oss << "GAME OVER. Final: " << e.score;                      break;
            case T::Undo:     oss << "Undo performed";                                     break;
            default: return;   // Move, NewTile e Reset poluiriam o log
        }
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now().time_since_epoch()).count() % 1'000'000;
        std::cerr << "\033[2m[" << ms << "ms] " << oss.str() << "\033[0m\n";
    }
};
