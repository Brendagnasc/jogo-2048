/*
 * main.cpp — Ponto de entrada do programa
 * ─────────────────────────────────────────
 * CONCEITO: Por que main.cpp é tão pequeno?
 *   Boas práticas C++ recomendam que main() faça o mínimo possível:
 *   criar os objetos de mais alto nível e iniciar o loop principal.
 *   Toda a lógica fica em classes e funções especializadas.
 *
 *   Isso facilita:
 *   - Testar partes do código sem executar o programa completo
 *   - Reutilizar GameLogic em outro projeto (ex: uma versão gráfica com SDL2)
 *   - Trocar o renderer sem tocar na lógica
 *
 * CONCEITO: Declaração externa (forward declaration de função)
 *   'createTerminalRenderer()' é definida em Renderer.cpp.
 *   Declaramos aqui apenas a assinatura para que o compilador saiba
 *   que ela existe. O linker encontrará a definição depois.
 *
 *   Alternativa (mais comum): colocar a declaração num header e incluir.
 *   Aqui usamos declaração inline para mostrar o conceito explicitamente.
 *
 * CONCEITO: try/catch e exceções
 *   Exceções são mecanismo para tratar erros em tempo de execução.
 *   'throw' lança, 'catch' captura. 'std::exception' é a base de todas
 *   as exceções padrão. 'e.what()' retorna a mensagem de erro.
 */

#include "Controller.h"

#include <memory>     // std::unique_ptr
#include <stdexcept>  // std::exception
#include <iostream>
#include <cstdlib>    // EXIT_SUCCESS, EXIT_FAILURE

// Declaração da factory function definida em Renderer.cpp
// Retorna um unique_ptr<IRenderer> — oculta qual implementação concreta usa
std::unique_ptr<IRenderer> createTerminalRenderer();

int main() {
    try {
        // 1. Criar o renderer via factory — nenhum 'new' explícito
        auto renderer = createTerminalRenderer();

        // 2. Injetar no controller — move semantics: sem cópia do unique_ptr
        GameController ctrl(std::move(renderer));

        // 3. Iniciar o loop principal — bloqueia até o usuário sair
        ctrl.run();
    }
    catch (const std::exception& e) {
        // Captura qualquer exceção derivada de std::exception (hierarquia padrão)
        // Exemplos: std::bad_alloc (falta memória), std::runtime_error, etc.
        std::cerr << "\n  Erro fatal: " << e.what() << "\n";
        return EXIT_FAILURE;   // Código de saída 1 = erro para o sistema operacional
    }

    return EXIT_SUCCESS;   // Código de saída 0 = sucesso
}
