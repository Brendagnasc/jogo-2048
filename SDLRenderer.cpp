/*
 * SDLRenderer.cpp  —  Implementação SDL2 — janela gráfica real
 * ═════════════════════════════════════════════════════════════
 * Abre uma janela no desktop do Ubuntu via SDL2.
 * Tema dark: fundo quase-preto, tiles com cores quentes, VSync 60fps.
 */
#include "SDLRenderer.h"
#include <stdexcept>
#include <iostream>
#include <cmath>
#include <algorithm>

// ── Paleta de cores ───────────────────────────────────────────────────────────
namespace P {
    constexpr SDL_Color BG     {  12,  12,  18, 255 };
    constexpr SDL_Color PANEL  {  24,  24,  36, 255 };
    constexpr SDL_Color EMPTY  {  38,  38,  54, 255 };
    constexpr SDL_Color WHITE  { 245, 238, 222, 255 };
    constexpr SDL_Color MUTED  { 105, 100, 120, 255 };
    constexpr SDL_Color GOLD   { 255, 190,  50, 255 };
    constexpr SDL_Color GREEN  {  65, 195, 110, 255 };
    constexpr SDL_Color RED    { 215,  65,  65, 255 };

    inline SDL_Color tile(int v) {
        switch(v){
        case    2: return {  52,  48,  70,255};
        case    4: return {  75,  70,  98,255};
        case    8: return { 192,  94,  38,255};
        case   16: return { 212,  68,  25,255};
        case   32: return { 230,  48,  28,255};
        case   64: return { 242,  32,  22,255};
        case  128: return { 235, 180,  28,255};
        case  256: return { 235, 168,  14,255};
        case  512: return { 235, 155,   4,255};
        case 1024: return { 235, 142,   0,255};
        case 2048: return { 255, 215,   0,255};
        default:   return {  88,  50, 178,255};
        }
    }
    inline SDL_Color tileFg(int v){
        if(v<=4)  return {135,130,158,255};
        if(v<8)   return {185,180,205,255};
        return {255,250,235,255};
    }
}

static constexpr float DUR_POP  = 180.f;
static constexpr float DUR_MRG  = 230.f;
static constexpr float DUR_SLD  = 120.f;
static constexpr float TICK     =  16.f; // ~60fps

// ═════════════════════════════════════════════════════════════════════════════
SDLRenderer::SDLRenderer() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        throw std::runtime_error(std::string("SDL_Init: ") + SDL_GetError());
    if (TTF_Init() != 0) {
        SDL_Quit();
        throw std::runtime_error(std::string("TTF_Init: ") + TTF_GetError());
    }

    win_.reset(SDL_CreateWindow(
        "2048  |  C++ Edition",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
    ));
    if (!win_) throw std::runtime_error(std::string("CreateWindow: ") + SDL_GetError());

    ren_.reset(SDL_CreateRenderer(win_.get(), -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
    if (!ren_) throw std::runtime_error(std::string("CreateRenderer: ") + SDL_GetError());

    SDL_SetRenderDrawBlendMode(ren_.get(), SDL_BLENDMODE_BLEND);

    // Tenta carregar fontes do sistema (Ubuntu tem DejaVu por padrão)
    auto tryF = [](std::initializer_list<const char*> paths, int sz) -> TTF_Font* {
        for (auto p : paths) if (auto f = TTF_OpenFont(p,sz)) return f;
        return nullptr;
    };

    fBig_.reset(tryF({
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Bold.ttf",
        "/usr/share/fonts/truetype/freefont/FreeMono.ttf",
        "/usr/share/fonts/truetype/ubuntu/UbuntuMono-B.ttf",
    }, 34));

    fMid_.reset(tryF({
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-B.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf",
    }, 17));

    fSml_.reset(tryF({
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
    }, 13));

    if (!fBig_ || !fMid_ || !fSml_) {
        std::cerr << "[2048] Fonte nao encontrada.\n"
                  << "       sudo apt install fonts-dejavu-core\n";
    }
    layout();
}

SDLRenderer::~SDLRenderer() { TTF_Quit(); SDL_Quit(); }

// ── layout ────────────────────────────────────────────────────────────────────
void SDLRenderer::layout() {
    int aw = WIN_W - HPANEL - 32;
    int ah = WIN_H - boardY_ - 16;
    boardSz_ = std::min(aw, ah);
    boardX_  = (WIN_W - HPANEL - boardSz_) / 2;
    cellSz_  = (boardSz_ - (BOARD_N + 1) * GAP) / BOARD_N;
}

float SDLRenderer::cx(int col) const {
    return static_cast<float>(boardX_ + GAP + col*(cellSz_+GAP) + cellSz_/2);
}
float SDLRenderer::cy(int row) const {
    return static_cast<float>(boardY_ + GAP + row*(cellSz_+GAP) + cellSz_/2);
}

// ── render ────────────────────────────────────────────────────────────────────
void SDLRenderer::render(const Grid<BOARD_N>& grid,
                          int score, int best, int moves) {
    stepAnims();
    SDL_SetRenderDrawColor(ren_.get(), P::BG.r, P::BG.g, P::BG.b, 255);
    SDL_RenderClear(ren_.get());
    drawBg();
    drawHeader(score, best, moves);
    drawBoard(grid);
    drawHist();
    drawToast();
    SDL_RenderPresent(ren_.get());
}

// ── drawBg ────────────────────────────────────────────────────────────────────
void SDLRenderer::drawBg() {
    roundRect({boardX_-14, boardY_-14, boardSz_+28, boardSz_+28}, 14, P::PANEL);
    roundRect({WIN_W-HPANEL+4, 8, HPANEL-12, WIN_H-16}, 12, P::PANEL);
}

// ── drawHeader ────────────────────────────────────────────────────────────────
void SDLRenderer::drawHeader(int score, int best, int moves) {
    txt("2048", fBig_, P::GOLD, {20, 14, 150, 52}, false);

    auto scoreBox = [&](const char* label, int val, int x){
        roundRect({x, 12, 124, 66}, 10, P::PANEL);
        txt(label, fSml_, P::MUTED, {x, 18, 124, 20});
        txt(std::to_string(val), fMid_, P::WHITE, {x, 40, 124, 30});
    };
    int bx = WIN_W - HPANEL;
    scoreBox("SCORE", score, bx - 268);
    scoreBox("BEST",  best,  bx - 132);

    txt("Movimentos: " + std::to_string(moves),
        fSml_, P::MUTED, {20, 74, 350, 18}, false);
    txt("WASD / setas  |  Z desfazer  |  R reiniciar  |  Q sair",
        fSml_, P::MUTED, {20, 92, 430, 16}, false);
}

// ── drawBoard ─────────────────────────────────────────────────────────────────
void SDLRenderer::drawBoard(const Grid<BOARD_N>& grid) {
    // Fundo das células vazias
    for (int r = 0; r < BOARD_N; r++)
        for (int c = 0; c < BOARD_N; c++)
            roundRect({boardX_+GAP+c*(cellSz_+GAP),
                       boardY_+GAP+r*(cellSz_+GAP),
                       cellSz_, cellSz_}, RADIUS, P::EMPTY);

    // Tiles com animação
    for (int r = 0; r < BOARD_N; r++)
        for (int c = 0; c < BOARD_N; c++) {
            int v = grid[r][c];
            if (!v) continue;
            auto& a = anims_[idx(r,c)];
            float scale=1.f, ox=0.f, oy=0.f;
            if (a.active) {
                switch(a.kind){
                case AnimKind::Pop:
                    scale = 0.45f + 0.55f * easeOutBack(a.t);
                    break;
                case AnimKind::Merge:
                    scale = 1.f + 0.24f * std::sin(a.t * 3.14159f);
                    break;
                case AnimKind::Slide: {
                    float destX = static_cast<float>(boardX_+GAP+c*(cellSz_+GAP));
                    float destY = static_cast<float>(boardY_+GAP+r*(cellSz_+GAP));
                    float et = easeOutCubic(a.t);
                    ox = a.fromX*(1.f-et) + destX*et - destX;
                    oy = a.fromY*(1.f-et) + destY*et - destY;
                    break;
                }
                default: break;
                }
            }
            drawTile(r, c, v, scale, ox, oy);
        }
}

// ── drawTile ──────────────────────────────────────────────────────────────────
void SDLRenderer::drawTile(int r, int c, int val,
                             float scale, float ox, float oy) {
    int bx = boardX_+GAP+c*(cellSz_+GAP);
    int by = boardY_+GAP+r*(cellSz_+GAP);
    int sz = static_cast<int>(cellSz_*scale);
    int x  = static_cast<int>(bx+ox+(cellSz_-sz)/2);
    int y  = static_cast<int>(by+oy+(cellSz_-sz)/2);

    // Sombra
    roundRect({x+3, y+5, sz, sz}, RADIUS, {0,0,0,80});
    // Tile
    roundRect({x, y, sz, sz}, RADIUS, P::tile(val));
    // Número
    if (fBig_)
        txt(std::to_string(val), fBig_, P::tileFg(val), {x, y, sz, sz});
}

// ── drawHist ──────────────────────────────────────────────────────────────────
void SDLRenderer::drawHist() {
    int px = WIN_W - HPANEL + 14;
    txt("HISTÓRICO", fSml_, P::GOLD, {px, 18, HPANEL-28, 20});

    if (hist_.empty()) {
        txt("Sem partidas ainda", fSml_, P::MUTED, {px, 50, HPANEL-28, 20});
        return;
    }
    int y = 48;
    for (int i=(int)hist_.size()-1; i>=0 && (int)hist_.size()-1-i<9; --i) {
        auto& e = hist_[i];
        int row = (int)hist_.size()-1 - i;
        SDL_Color rb = row%2==0 ? SDL_Color{32,32,46,255}
                                 : SDL_Color{26,26,38,255};
        roundRect({px-4, y, HPANEL-22, 48}, 6, rb);
        SDL_Color sc = e.won ? P::GOLD : P::WHITE;
        txt(std::to_string(e.score), fMid_, sc, {px, y+4, HPANEL-28, 22});
        std::string det = std::to_string(e.moves)+" mov";
        if (e.won) det += "  \xe2\x98\x85";  // ★ UTF-8
        txt(det, fSml_, P::MUTED, {px, y+28, HPANEL-28, 16});
        y += 52;
    }
}

// ── drawToast ────────────────────────────────────────────────────────────────
void SDLRenderer::drawToast() {
    if (msg_.empty()) return;
    if (SDL_GetTicks() > msgUntil_) { msg_.clear(); return; }
    SDL_Color bg = msgErr_ ? SDL_Color{165,42,42,225} : SDL_Color{35,145,80,225};
    int mw=400, mh=48;
    int mx=(WIN_W-HPANEL-mw)/2, my=WIN_H-68;
    roundRect({mx, my, mw, mh}, 10, bg);
    if (fMid_) txt(msg_, fMid_, P::WHITE, {mx, my, mw, mh});
}

// ── stepAnims ─────────────────────────────────────────────────────────────────
void SDLRenderer::stepAnims() {
    for (auto& a : anims_) {
        if (!a.active) continue;
        float dur = a.kind==AnimKind::Slide ? DUR_SLD
                  : a.kind==AnimKind::Merge  ? DUR_MRG : DUR_POP;
        a.t = std::clamp(a.t + TICK/dur, 0.f, 1.f);
        if (a.t >= 1.f) { a.active=false; a.kind=AnimKind::Idle; }
    }
}

// ── triggers ──────────────────────────────────────────────────────────────────
void SDLRenderer::triggerPop(int r, int c) {
    anims_[idx(r,c)] = {AnimKind::Pop, 0.f, 0.f, 0.f, true};
}
void SDLRenderer::triggerMerge(int r, int c) {
    anims_[idx(r,c)] = {AnimKind::Merge, 0.f, 0.f, 0.f, true};
}
void SDLRenderer::triggerSlide(int r, int c, float fx, float fy) {
    anims_[idx(r,c)] = {AnimKind::Slide, 0.f, fx, fy, true};
}
void SDLRenderer::pushHistory(const HistEntry& e) {
    hist_.push_back(e);
    if (hist_.size()>20) hist_.erase(hist_.begin());
}

// ── pollInput ────────────────────────────────────────────────────────────────
char SDLRenderer::pollInput() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) { closed_=true; return 'q'; }
        if (ev.type == SDL_KEYDOWN) {
            switch(ev.key.keysym.sym) {
            case SDLK_w: case SDLK_UP:     return 'w';
            case SDLK_s: case SDLK_DOWN:   return 's';
            case SDLK_a: case SDLK_LEFT:   return 'a';
            case SDLK_d: case SDLK_RIGHT:  return 'd';
            case SDLK_z:                   return 'z';
            case SDLK_r:                   return 'r';
            case SDLK_q: case SDLK_ESCAPE: closed_=true; return 'q';
            default: break;
            }
        }
    }
    return '\0';
}

// ── showMsg ───────────────────────────────────────────────────────────────────
void SDLRenderer::showMsg(const std::string& msg, bool err) {
    msg_=msg; msgErr_=err; msgUntil_=SDL_GetTicks()+2500;
}

// ── roundRect ─────────────────────────────────────────────────────────────────
void SDLRenderer::roundRect(SDL_Rect rc, int rad, SDL_Color col) {
    SDL_SetRenderDrawColor(ren_.get(), col.r, col.g, col.b, col.a);
    SDL_Rect m={rc.x,        rc.y+rad, rc.w,          rc.h-2*rad};
    SDL_Rect t={rc.x+rad,    rc.y,     rc.w-2*rad,    rad        };
    SDL_Rect b={rc.x+rad,    rc.y+rc.h-rad, rc.w-2*rad, rad      };
    SDL_RenderFillRect(ren_.get(), &m);
    SDL_RenderFillRect(ren_.get(), &t);
    SDL_RenderFillRect(ren_.get(), &b);
    // 4 cantos arredondados
    int corners[4][2] = {
        {rc.x+rad-1, rc.y+rad-1},
        {rc.x+rc.w-rad, rc.y+rad-1},
        {rc.x+rad-1, rc.y+rc.h-rad},
        {rc.x+rc.w-rad, rc.y+rc.h-rad}
    };
    for (int q=0; q<4; q++)
        for (int dy=-rad; dy<=0; dy++)
            for (int dx=-rad; dx<=0; dx++)
                if (dx*dx+dy*dy <= rad*rad) {
                    int sx=(q==0||q==2)?dx:-dx-1;
                    int sy=(q==0||q==1)?dy:-dy-1;
                    SDL_RenderDrawPoint(ren_.get(), corners[q][0]+sx, corners[q][1]+sy);
                }
}

// ── txt ───────────────────────────────────────────────────────────────────────
void SDLRenderer::txt(const std::string& s, FntPtr& f, SDL_Color col,
                       SDL_Rect dst, bool ccx, bool ccy) {
    if (!f || s.empty()) return;
    SurPtr sur{TTF_RenderUTF8_Blended(f.get(), s.c_str(), col)};
    if (!sur) return;
    TexPtr tex{SDL_CreateTextureFromSurface(ren_.get(), sur.get())};
    if (!tex) return;
    int tw,th; SDL_QueryTexture(tex.get(), nullptr, nullptr, &tw, &th);
    SDL_Rect dr{
        ccx ? dst.x+(dst.w-tw)/2 : dst.x,
        ccy ? dst.y+(dst.h-th)/2 : dst.y,
        tw, th
    };
    SDL_RenderCopy(ren_.get(), tex.get(), nullptr, &dr);
}

// ── easing ───────────────────────────────────────────────────────────────────
float SDLRenderer::easeOutCubic(float t) {
    float i=1.f-t; return 1.f-i*i*i;
}
float SDLRenderer::easeOutBack(float t) {
    constexpr float c1=1.70158f, c3=c1+1.f;
    float i=t-1.f; return 1.f+c3*i*i*i+c1*i*i;
}

SDL_Color SDLRenderer::tileBg(int v) { return P::tile(v); }
SDL_Color SDLRenderer::tileFg(int v) { return P::tileFg(v); }

std::unique_ptr<IRenderer> createSDLRenderer() {
    return std::make_unique<SDLRenderer>();
}
