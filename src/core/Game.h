#pragma once
/*
 * Game.h — Estado de uma partida (camada core, o "M" do MVC)
 * ───────────────────────────────────────────────────────────
 * Game<N> guarda o tabuleiro, a pontuação e a pilha de undo, e aplica as
 * regras de board::slide. Continua sem nenhuma entrada/saída.
 *
 * DECISÕES DE ARQUITETURA
 *   • Gerador aleatório por instância, com semente no construtor.
 *     Antes era um 'static' global: impossível reproduzir uma partida.
 *     Com a semente, um teste cria Game<4>(42) e sabe exatamente onde as
 *     peças vão nascer.
 *   • move() devolve um Turn com tudo o que aconteceu (caminho de cada peça,
 *     peça nova, vitória, fim de jogo). A interface usa isso para animar e
 *     mostrar mensagens, sem comparar grades.
 *   • Observadores são registrados por referência (ponteiro não-dono).
 *     Quem cria o observador é dono dele e garante que ele vive mais que o
 *     Game (na prática, os dois são membros da mesma GameSession).
 *   • O recorde (best) não fica aqui: é um dado persistente da sessão
 *     (GameSession), não uma regra da partida.
 *
 * Por que tudo no .h? Game é template: o compilador precisa ver o corpo
 * completo no momento em que instancia Game<4>.
 */

#include "Board.h"
#include "Observer.h"

#include <cstdint>
#include <optional>
#include <random>
#include <vector>

template <int N>
class Game {
public:
    // Tudo o que um movimento produziu
    struct Turn {
        MoveResult<N>        slide;            // caminho de cada peça e pontos
        std::optional<Spawn> spawn;            // peça nova (se havia espaço)
        bool                 justWon{false};   // chegou a WIN_TILE agora
        bool                 gameOver{false};  // ficou sem jogadas
    };

    // Semente padrão aleatória; testes passam uma fixa
    explicit Game(std::uint32_t seed = std::random_device{}())
        : rng_(seed)
    {
        reset();
    }

    // ── Consultas ─────────────────────────────────────────────────────────
    const Grid<N>& grid()   const { return grid_;  }
    int            score()  const { return score_; }
    int            moves()  const { return moves_; }
    bool           hasWon() const { return won_;   }
    bool           isOver() const { return over_;  }
    bool           canUndo() const { return !history_.empty(); }

    void addObserver(IObserver& obs) { observers_.push_back(&obs); }

    // ── Comandos ──────────────────────────────────────────────────────────

    // Aplica um movimento. std::nullopt = nada se moveu (jogada inválida).
    std::optional<Turn> move(Direction dir) {
        if (over_) return std::nullopt;

        Turn turn;
        turn.slide = board::slide<N>(grid_, dir);
        if (!turn.slide.moved) return std::nullopt;

        // Padrão Command (memento): salva o estado ANTES de alterar
        history_.push_back({ grid_, score_, moves_, won_ });

        grid_   = turn.slide.grid;
        score_ += turn.slide.gained;
        moves_++;

        emit({ GameEvent::Type::Move, 0, score_ });
        for (const auto& t : turn.slide.tiles)
            if (t.merged) emit({ GameEvent::Type::Merge, t.value * 2, score_ });

        turn.spawn = spawnTile();

        if (!won_ && board::hasTile<N>(grid_, WIN_TILE)) {
            won_ = turn.justWon = true;
            emit({ GameEvent::Type::Win, WIN_TILE, score_ });
        }
        if (!board::canMove<N>(grid_)) {
            over_ = turn.gameOver = true;
            emit({ GameEvent::Type::GameOver, 0, score_ });
        }
        return turn;
    }

    // Volta um movimento. Restaura também o estado de vitória.
    bool undo() {
        if (history_.empty()) return false;
        const Snapshot snap = history_.back();
        history_.pop_back();
        grid_  = snap.grid;
        score_ = snap.score;
        moves_ = snap.moves;
        won_   = snap.won;
        over_  = false;   // se havia jogada antes, ainda há
        emit({ GameEvent::Type::Undo, 0, score_ });
        return true;
    }

    // Nova partida: tabuleiro vazio com duas peças
    void reset() {
        history_.clear();
        score_ = moves_ = 0;
        won_ = over_ = false;
        for (auto& row : grid_) row.fill(0);
        spawnTile();
        spawnTile();
        emit({ GameEvent::Type::Reset, 0, 0 });
    }

    // Define o tabuleiro diretamente (testes e, no futuro, carregar partida)
    void setState(const Grid<N>& g, int score = 0) {
        history_.clear();
        grid_  = g;
        score_ = score;
        moves_ = 0;
        won_   = board::hasTile<N>(g, WIN_TILE);
        over_  = !board::canMove<N>(g);
    }

private:
    struct Snapshot {
        Grid<N> grid;
        int     score;
        int     moves;
        bool    won;
    };

    Grid<N>                 grid_{};
    int                     score_{0};
    int                     moves_{0};
    bool                    won_{false};
    bool                    over_{false};
    std::vector<Snapshot>   history_;     // pilha de undo (back = topo)
    std::vector<IObserver*> observers_;   // não-donos
    std::mt19937            rng_;

    void emit(const GameEvent& e) {
        for (IObserver* obs : observers_) obs->onEvent(e);
    }

    // Coloca um 2 (90%) ou 4 (10%) numa célula vazia aleatória
    std::optional<Spawn> spawnTile() {
        const auto empty = board::emptyCells<N>(grid_);
        if (empty.empty()) return std::nullopt;

        std::uniform_int_distribution<std::size_t> pickCell(0, empty.size() - 1);
        std::uniform_int_distribution<int>         pickValue(0, 9);

        const Spawn s{ empty[pickCell(rng_)], pickValue(rng_) < 9 ? 2 : 4 };
        grid_[s.at.row][s.at.col] = s.value;
        emit({ GameEvent::Type::NewTile, s.value, score_ });
        return s;
    }
};
