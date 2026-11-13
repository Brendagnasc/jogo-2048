#pragma once
/*
 * Controller.h — GameController: o "C" do padrão MVC
 * ────────────────────────────────────────────────────
 * CONCEITO: MVC (Model-View-Controller)
 *   Model      = GameLogic   → estado e regras do jogo (sem I/O)
 *   View       = IRenderer   → como exibir o estado ao usuário
 *   Controller = GameController → lê input, chama Model, pede View para renderizar
 *
 *   Benefício: posso trocar a View (ex: terminal → janela SDL2) sem tocar no Model.
 *   Posso testar o Model sem UI nenhuma.
 *
 * CONCEITO: Forward declaration
 *   'class IRenderer;' diz ao compilador "existe uma classe IRenderer"
 *   sem incluir o header completo. Isso reduz dependências em cascata:
 *   Controller.h não "arrasta" todo o conteúdo de Renderer.h.
 *   Só funciona quando usamos ponteiros ou referências — não instâncias diretas.
 *
 * CONCEITO: unique_ptr vs shared_ptr vs raw pointer
 *   unique_ptr<T>:  um ÚNICO dono. Destruído quando o dono sai de escopo.
 *   shared_ptr<T>:  múltiplos donos. Destruído quando o ÚLTIMO dono sai.
 *   weak_ptr<T>:    observa sem ser dono. Não impede destruição.
 *   T*:             não gerencia nada — você cuida do delete.
 *
 *   Regra moderna: prefira unique_ptr. Use shared_ptr quando ownership compartilhada
 *   for realmente necessária. Evite raw pointers para ownership.
 */

#include "GameLogic.h"
#include "Observer.h"
#include "Renderer.h"
#include "Persistence.h"   // ← NOVO: módulo de save/load

#include <memory>   // unique_ptr, weak_ptr

class IRenderer;

class GameController {
public:
    // ── Construtor com injeção de dependência ────────────────────────────
    // Recebe um unique_ptr<IRenderer>: aceita QUALQUER implementação de renderer.
    // 'explicit': proíbe GameController c = renderer; (conversão implícita)
    // std::move(renderer): transfere a propriedade do unique_ptr para o membro.
    // Após o move, o parâmetro 'renderer' está em estado "válido mas vazio".
    explicit GameController(std::unique_ptr<IRenderer> renderer);

    // ── Loop principal ────────────────────────────────────────────────────
    void run();

private:
    std::unique_ptr<IRenderer>          rend_;
    std::unique_ptr<GameLogic<BOARD_N>> logic_;
    std::weak_ptr<StatsObserver>        statsWeak_;
    bool                                running_{true};

    // ── NOVO: objeto de persistência ─────────────────────────────────────
    // Persistence é um valor (não ponteiro) — não precisa de heap.
    // Criado com o caminho padrão do arquivo de save.
    // CONCEITO: member initializer na declaração (C++11)
    //   Inicializa persist_ com defaultSavePath() no momento da construção,
    //   sem precisar listá-lo na lista de inicialização do construtor.
    Persistence persist_{ defaultSavePath() };

    // ── NOVO: dados acumulados da sessão para salvar ──────────────────────
    // Mantemos um SaveData atualizado ao longo do jogo.
    // Ao encerrar, gravamos em disco.
    SaveData saveData_;

    void tick();
    void handleInput(char c);

    // ── NOVO: helpers de persistência ────────────────────────────────────
    void loadProgress();    // Chamado no início: carrega do disco
    void saveProgress();    // Chamado ao sair: grava no disco
    void updateSaveData();  // Atualiza saveData_ com o estado atual do jogo
};
