#pragma once
/*
 * Command.h — Ações do jogador, independentes de teclado (camada app)
 * ────────────────────────────────────────────────────────────────────
 * Cada interface traduz a sua entrada (tecla no terminal, tecla/gesto no
 * SDL, botão na página web) para um Command. A GameSession só entende
 * Commands, então as regras de "o que cada ação faz" ficam num lugar só.
 */

#include "../core/Types.h"

#include <cctype>
#include <optional>

enum class Command { MoveUp, MoveDown, MoveLeft, MoveRight, Undo, Restart, Quit };

// Mapeamento padrão de teclas: WASD, Z, R, Q
inline std::optional<Command> commandFromKey(char key) {
    switch (std::tolower(static_cast<unsigned char>(key))) {
        case 'w': return Command::MoveUp;
        case 's': return Command::MoveDown;
        case 'a': return Command::MoveLeft;
        case 'd': return Command::MoveRight;
        case 'z': return Command::Undo;
        case 'r': return Command::Restart;
        case 'q': return Command::Quit;
        default:  return std::nullopt;
    }
}

inline std::optional<Direction> directionOf(Command c) {
    switch (c) {
        case Command::MoveUp:    return Direction::Up;
        case Command::MoveDown:  return Direction::Down;
        case Command::MoveLeft:  return Direction::Left;
        case Command::MoveRight: return Direction::Right;
        default:                 return std::nullopt;
    }
}
