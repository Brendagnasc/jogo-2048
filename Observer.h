#pragma once
/*
 * Observer.h — Padrão de Projeto: Observer (Observador)
 * ──────────────────────────────────────────────────────
 * CONCEITO: Padrão Observer
 *   Define uma dependência 1-para-N: quando o objeto "sujeito" (GameLogic)
 *   muda de estado, todos os seus "observadores" são notificados e atualizados
 *   automaticamente. Isso DESACOPLA a lógica do jogo da apresentação/log/stats.
 *
 *   Sem Observer:  GameLogic chama diretamente renderer.render(), logger.log()...
 *   Com Observer:  GameLogic emite eventos; quem quiser reagir, se registra.
 *   Benefício: adicionar um novo comportamento (ex: som) não toca em GameLogic.
 *
 * CONCEITO: Classe abstrata / Interface pura
 *   Uma classe com apenas métodos virtuais puros (= 0) NÃO pode ser instanciada.
 *   Ela define um CONTRATO que subclasses DEVEM cumprir.
 *   Isso é polimorfismo em C++: código que usa IObserver* funciona com
 *   QUALQUER subclasse sem precisar saber qual é.
 *
 * CONCEITO: Destrutor virtual
 *   OBRIGATÓRIO em toda classe base com métodos virtuais.
 *   Sem ele: delete ptr_base não chama o destrutor da subclasse → vazamento.
 *   'virtual ~IObserver() = default' delega ao compilador uma implementação vazia.
 */

#include "Types.h"
#include <iostream>
#include <sstream>
#include <chrono>

// ── IObserver: interface pura ─────────────────────────────────────────────────
class IObserver {
public:
    virtual ~IObserver() = default;               // Destrutor virtual obrigatório
    virtual void onEvent(const GameEvent&) = 0;   // Método puramente virtual
    // 'const GameEvent&': recebe por referência constante — sem cópia, sem modificação
};

// ── LogObserver: imprime eventos no stderr ────────────────────────────────────
// 'final': palavra-chave que impede que LogObserver seja herdado.
//   - Semântico: "esta classe não deve ser estendida"
//   - Otimização: compilador pode fazer devirtualização de chamadas
//
// 'public IObserver': herança pública — LogObserver É-UM IObserver.
//   Herança pública: interface herdada permanece pública.
//   Herança privada (rara): herda implementação, não interface.
class LogObserver final : public IObserver {
public:
    // 'override': confirma explicitamente que sobrescreve um virtual da base.
    // O compilador gera erro se a assinatura não corresponder — evita bugs sutis.
    void onEvent(const GameEvent& e) override {
        using T = GameEvent::Type;

        // Filtra eventos pouco informativos para não poluir o log
        if (e.type == T::Move || e.type == T::NewTile) return;

        // std::chrono: mede tempo com alta precisão sem usar clock() antigo
        auto ts = std::chrono::steady_clock::now().time_since_epoch();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(ts).count() % 1'000'000;
        // '1'000'000': separador de milhar (C++14) — apenas visual, sem efeito no valor

        // ostringstream: monta string sem concatenar com '+' (eficiente)
        std::ostringstream oss;
        switch (e.type) {
            case T::Merge:    oss << "Merge +" << e.value << " (score=" << e.score << ")"; break;
            case T::Win:      oss << "WIN! Reached " << WIN_TILE;                          break;
            case T::GameOver: oss << "GAME OVER. Final: " << e.score;                      break;
            case T::Undo:     oss << "Undo performed";                                     break;
            default: return;
        }

        // Imprime em stderr para não misturar com a UI que vai para stdout
        std::cerr << "\033[2m[" << ms << "ms] " << oss.str() << "\033[0m\n";
    }
};

// ── StatsObserver: acumula estatísticas da sessão ─────────────────────────────
class StatsObserver final : public IObserver {
public:
    // Membros públicos com inicialização in-class (C++11)
    int merges{0};
    int undos{0};
    int pointsFromMerges{0};

    void onEvent(const GameEvent& e) override {
        using T = GameEvent::Type;
        if (e.type == T::Merge) { merges++; pointsFromMerges += e.value; }
        if (e.type == T::Undo)  { undos++; }
    }

    void print() const {
        // 'const' no método: garante que não modifica o objeto.
        // Pode ser chamado em instâncias const: const StatsObserver s; s.print();
        std::cout << "\n  \033[1m── Estatísticas ────────────────\033[0m\n"
                  << "  Fusões realizadas : " << merges             << "\n"
                  << "  Pontos por fusões : " << pointsFromMerges   << "\n"
                  << "  Undos usados      : " << undos              << "\n";
    }
};
