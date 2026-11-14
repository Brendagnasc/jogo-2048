#pragma once
/*
 * SDLRenderer.h  —  Janela gráfica SDL2 para o 2048
 * ══════════════════════════════════════════════════
 * Implementa IRenderer abrindo uma janela real no desktop.
 * Tema: dark elegante, tiles coloridos, animações suaves.
 */
#include "../../app/Command.h"
#include "../../app/Renderer.h"
#include "../../core/Types.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// ── RAII deleters para recursos SDL ──────────────────────────────────────────
struct DelWin { void operator()(SDL_Window*   p) noexcept { SDL_DestroyWindow(p);   }};
struct DelRen { void operator()(SDL_Renderer* p) noexcept { SDL_DestroyRenderer(p); }};
struct DelTex { void operator()(SDL_Texture*  p) noexcept { SDL_DestroyTexture(p);  }};
struct DelSur { void operator()(SDL_Surface*  p) noexcept { SDL_FreeSurface(p);     }};
struct DelFnt { void operator()(TTF_Font*      p) noexcept { TTF_CloseFont(p);       }};

using WinPtr = std::unique_ptr<SDL_Window,   DelWin>;
using RenPtr = std::unique_ptr<SDL_Renderer, DelRen>;
using TexPtr = std::unique_ptr<SDL_Texture,  DelTex>;
using SurPtr = std::unique_ptr<SDL_Surface,  DelSur>;
using FntPtr = std::unique_ptr<TTF_Font,      DelFnt>;

// ── Animação por célula ───────────────────────────────────────────────────────
enum class AnimKind { Idle, Pop, Merge, Slide };
struct CellAnim {
    AnimKind kind   = AnimKind::Idle;
    float    t      = 1.f;   // 0 = início, 1 = fim
    float    fromX  = 0.f;   // canto superior esquerdo da célula de origem (Slide)
    float    fromY  = 0.f;
    bool     active = false;
};

// ── Entrada de histórico ──────────────────────────────────────────────────────
struct HistEntry { int score; int moves; bool won; };

// ═════════════════════════════════════════════════════════════════════════════
class SDLRenderer final : public IRenderer {
public:
    explicit SDLRenderer();
    ~SDLRenderer() override;

    void render(const GameView& view) override;
    void showMsg(const std::string& msg, bool err = false) override;

    // Anima um movimento a partir do caminho real de cada peça, informado
    // pela lógica (MoveResult). Antes as animações eram deduzidas comparando
    // a grade antes/depois, o que errava a origem quando havia valores iguais.
    void animate(const std::vector<TileMove>& tiles, const std::optional<Spawn>& spawn);
    void pushHistory(const HistEntry& e);

    // Lê teclado, mouse e toque e traduz para Command
    std::optional<Command> pollInput();
    bool closed() const { return closed_; }

private:
    // Canto superior esquerdo de uma célula, em pixels
    float cellX(int col) const;
    float cellY(int row) const;

    void triggerPop  (int r, int c);
    void triggerMerge(int r, int c);
    void triggerSlide(int r, int c, float fromX, float fromY);

    WinPtr win_;
    RenPtr ren_;
    FntPtr fBig_, fMid_, fSml_;

    // Layout
    static constexpr int WIN_W   = 860;
    static constexpr int WIN_H   = 680;
    static constexpr int GAP     = 10;
    static constexpr int RADIUS  = 8;
    static constexpr int HPANEL  = 210;  // largura painel histórico
    static constexpr int HEADER  = 115;  // altura header

    int boardSz_ = 0;  // lado do tabuleiro (quadrado)
    int boardX_  = 0;
    int boardY_  = HEADER + 8;
    int cellSz_  = 0;

    std::array<CellAnim, BOARD_N * BOARD_N> anims_{};
    std::vector<HistEntry> hist_;

    std::string msg_;
    bool        msgErr_   = false;
    Uint32      msgUntil_ = 0;
    bool        closed_   = false;

    // gesto de deslizar (mouse ou toque)
    static constexpr int SWIPE_MIN = 30;  // pixels mínimos para contar como movimento
    bool swiping_ = false;
    int  swipeX_  = 0;
    int  swipeY_  = 0;

    void layout();
    void stepAnims();
    void drawBg();
    void drawHeader(int score, int best, int moves);
    void drawBoard(const Grid<BOARD_N>& grid);
    void drawTile(int r, int c, int val, float scale, float ox, float oy);
    void drawHist();
    void drawToast();

    void roundRect(SDL_Rect rc, int rad, SDL_Color col);
    void txt(const std::string& s, FntPtr& f, SDL_Color col,
             SDL_Rect dst, bool cx = true, bool cy = true);

    static SDL_Color tileBg(int v);
    static SDL_Color tileFg(int v);
    static float easeOutCubic(float t);
    static float easeOutBack (float t);
    static int   idx(int r, int c) { return r * BOARD_N + c; }
};

std::unique_ptr<IRenderer> createSDLRenderer();
