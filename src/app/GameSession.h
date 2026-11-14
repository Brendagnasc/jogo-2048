#pragma once
/*
 * GameSession.h — Casos de uso do jogo (camada app)
 * ──────────────────────────────────────────────────
 * Antes, GameController (terminal) e SDLController repetiam as mesmas regras:
 * o que cada tecla faz, quando salvar, como somar estatísticas ao reiniciar.
 * Qualquer correção precisava ser feita duas vezes (e as duas versões já
 * tinham divergido). Agora essas regras vivem só aqui.
 *
 *   ui/terminal ─┐
 *                ├─► GameSession ─► Game<N> (core)
 *   ui/sdl ──────┘        │
 *                         └─► Persistence (save em arquivo / IndexedDB)
 *
 * As interfaces ficam responsáveis apenas por: ler a entrada, traduzir para
 * Command, chamar handle() e desenhar o resultado.
 */

#include "../core/Game.h"
#include "Command.h"
#include "Observers.h"
#include "Persistence.h"
#include "Renderer.h"

#include <cstdint>
#include <optional>
#include <string>

class GameSession {
public:
    using GameT = Game<BOARD_N>;

    // Resumo de uma partida encerrada (para o histórico da interface)
    struct GameSummary {
        int  score{0};
        int  moves{0};
        bool won{false};
    };

    // O que a interface precisa saber depois de um comando
    struct Outcome {
        std::string                 message;        // vazio = nada a dizer
        bool                        isError{false};
        std::optional<GameT::Turn>  turn;           // movimento aplicado (para animar)
        std::optional<GameSummary>  finishedGame;   // partida encerrada por Restart
        bool                        quit{false};
    };

    struct Options {
        std::string   savePath = defaultSavePath();
        bool          logEvents = true;                 // LogObserver no stderr
        std::uint32_t seed = std::random_device{}();
    };

    GameSession();                          // opções padrão
    explicit GameSession(Options opts);

    // Não copiável: o Game guarda ponteiros para os observadores membros
    GameSession(const GameSession&)            = delete;
    GameSession& operator=(const GameSession&) = delete;

    // Carrega o save. Devolve uma mensagem de boas-vindas.
    std::string loadProgress();

    // Aplica uma ação do jogador
    Outcome handle(Command cmd);

    // Encerra a sessão: contabiliza a partida atual e grava o save
    void finish();

    GameView              view()     const;
    const SaveData&       saveData() const { return save_; }
    const StatsObserver&  stats()    const { return stats_; }
    const GameT&          game()     const { return game_; }

private:
    StatsObserver stats_;
    LogObserver   log_;
    GameT         game_;      // declarado depois dos observadores: eles vivem mais
    Persistence   persist_;
    SaveData      save_;

    int  best() const;
    void recordFinishedGame();   // soma a partida atual ao save, uma única vez
    void saveBestIfImproved();
};
