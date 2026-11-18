// ============================================================================
//  maze_ui.hpp  -  Interfaz pixel art "RUNNER.EXE" (inspirada en Maze Runner)
//
//  Libreria de un solo archivo (header-only) hecha con raylib 5.x
//  Requiere C++17.
//
//  USO (en tu main):
//
//      #include "maze_ui.hpp"
//      ...
//      MazeUIResult solve_Maze(const std::string& archivo) { ... }  // tu logica
//      ...
//      run_maze_ui(solve_Maze, save_File);
//
//  La UI pide el nombre del laberinto dentro de la ventana, llama a tu funcion
//  "solve_Maze" para resolverlo y guarda los archivos con tu "save_File".
//  La funcion abre la ventana y bloquea hasta que el usuario la cierre.
//  Fuentes opcionales: si existe "PressStart2P-Regular.ttf" junto al .exe se
//  usa automaticamente; si no, se usa la fuente pixelada de raylib.
// ============================================================================
#pragma once

#include "C:/raylib/raylib/src/raylib.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <climits>
#include <cmath>
#include <cstdio>
#include <queue>
#include <string>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
//  Tipos publicos: lo que la UI recibe de tu Main
// ---------------------------------------------------------------------------
struct MazeUIResult {
    bool ok = false;      // false = hubo un error (se muestra "error" en la UI)
    std::string error;
    std::vector<std::string> matrix;
    std::pair<int, int> start_A{-1, -1}, exit_B{-1, -1};
    std::vector<std::string> dfs_pre, dfs_post, dfs_solution, bfs_route, bfs_solution;
};
using MazeSolver = std::function<MazeUIResult(const std::string&)>;
using FileSaver = std::function<void(const std::string&, const std::vector<std::string>&)>;

namespace maze_ui_detail {

constexpr int WIN_W = 1120;
constexpr int WIN_H = 660;

// ---------------------------------------------------------------------------
//  Paleta
// ---------------------------------------------------------------------------
inline Color Hex(unsigned int rgb, unsigned char a = 255) {
    return Color{(unsigned char)(rgb >> 16), (unsigned char)(rgb >> 8), (unsigned char)rgb, a};
}

struct Palette {
    Color night, panel, panel2, stone, stoneHi, stoneDark, ivy, ivyDark, sand, gray,
        amber, amberHi, dfs, bfs, path, floorA, floorB, red, console, green;
};

inline const Palette& P() {
    static const Palette p{
        Hex(0x0e1213), Hex(0x1a2022), Hex(0x232b2d), Hex(0x4a5557), Hex(0x6f7d7b),
        Hex(0x2b3234), Hex(0x4f8a43), Hex(0x2f5a2b), Hex(0xe8d9ae), Hex(0xa9b4ae),
        Hex(0xf2b53b), Hex(0xffd97a), Hex(0xe0763a), Hex(0x5fd0e0), Hex(0xe9f07a),
        Hex(0x26302c), Hex(0x222b28), Hex(0xa8452a), Hex(0x0a0d0e), Hex(0x9be36a)};
    return p;
}

inline Color Mix(Color a, Color b, float t) {
    t = std::max(0.f, std::min(1.f, t));
    return Color{(unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t),
                 (unsigned char)(a.b + (b.b - a.b) * t), 255};
}

// ---------------------------------------------------------------------------
//  Texto
// ---------------------------------------------------------------------------
inline Font g_font;
inline bool g_custom = false;

inline float FontPx(int size) { return g_custom ? size * 0.7f : (float)size; }
inline float FontSpacing(int size) { return g_custom ? 0.f : size / 10.f; }

inline void Txt(const std::string& s, float x, float y, int size, Color c) {
    DrawTextEx(g_font, s.c_str(), Vector2{x, y}, FontPx(size), FontSpacing(size), c);
}
inline int Wid(const std::string& s, int size) {
    return (int)MeasureTextEx(g_font, s.c_str(), FontPx(size), FontSpacing(size)).x;
}
inline void TxtC(const std::string& s, Rectangle r, int size, Color c) {
    Txt(s, r.x + (r.width - Wid(s, size)) / 2.f, r.y + (r.height - FontPx(size)) / 2.f, size, c);
}
inline void TxtR(const std::string& s, float rightX, float y, int size, Color c) {
    Txt(s, rightX - Wid(s, size), y, size, c);
}

// ---------------------------------------------------------------------------
//  Piezas de UI pixel art
// ---------------------------------------------------------------------------
inline void Panel(Rectangle r, Color fill, Color border) {
    DrawRectangleRec(r, border);
    DrawRectangle((int)r.x + 4, (int)r.y + 4, (int)r.width - 8, (int)r.height - 8, fill);
    Color bg = P().night;  // esquinas cortadas
    DrawRectangle((int)r.x, (int)r.y, 4, 4, bg);
    DrawRectangle((int)(r.x + r.width - 4), (int)r.y, 4, 4, bg);
    DrawRectangle((int)r.x, (int)(r.y + r.height - 4), 4, 4, bg);
    DrawRectangle((int)(r.x + r.width - 4), (int)(r.y + r.height - 4), 4, 4, bg);
}

inline bool Hover(Rectangle r) { return CheckCollisionPointRec(GetMousePosition(), r); }
inline bool Clicked(Rectangle r) { return Hover(r) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT); }

inline bool Button(Rectangle r, const std::string& label, int size, Color fill, Color border,
                   Color fg) {
    bool hov = Hover(r);
    bool down = hov && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    Rectangle rr = r;
    if (down) rr.y += 2;
    Panel(rr, hov ? Mix(fill, WHITE, 0.12f) : fill, border);
    DrawRectangle((int)rr.x + 4, (int)(rr.y + rr.height - 10), (int)rr.width - 8, 6, Fade(BLACK, 0.3f));
    TxtC(label, rr, size, fg);
    return hov && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

inline void Section(Rectangle r, const std::string& title) {
    Panel(r, P().panel, P().stone);
    DrawRectangle((int)r.x + 16, (int)r.y + 18, 12, 12, P().amber);
    Txt(title, r.x + 36, r.y + 14, 20, P().amber);
}

inline bool g_claimed = false;  // true si un cuadro de texto recibio el clic este frame

// Recorta por la izquierda con "..." para que el texto quepa en maxW
inline std::string Fit(const std::string& s, int maxW, int size) {
    if (Wid(s, size) <= maxW) return s;
    std::string t = s;
    while (!t.empty() && Wid("..." + t, size) > maxW) t.erase(0, 1);
    return "..." + t;
}

// Cuadro de texto: clic para enfocar, escribe, Backspace borra, Supr limpia, Ctrl+V pega
inline void TextBox(Rectangle r, std::string& s, int id, int& focus, size_t maxLen = 150) {
    const Palette& p = P();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && Hover(r)) {
        focus = id;
        g_claimed = true;
    }
    bool foc = focus == id;
    Panel(r, p.night, foc ? p.amber : Hex(0x3b4547));
    if (foc) {
        int ch;
        while ((ch = GetCharPressed()) > 0)
            if (ch >= 32 && ch < 127 && s.size() < maxLen) s.push_back((char)ch);
        if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && !s.empty()) s.pop_back();
        if (IsKeyPressed(KEY_DELETE)) s.clear();
        if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_V)) {
            const char* t = GetClipboardText();
            if (t)
                for (const char* q = t; *q; ++q)
                    if (*q >= 32 && *q < 127 && s.size() < maxLen) s.push_back(*q);
        }
    }
    std::string vis = s;
    while (!vis.empty() && Wid(vis, 20) > r.width - 32) vis.erase(0, 1);
    float ty = r.y + (r.height - 20) / 2.f;
    Txt(vis, r.x + 14, ty, 20, p.sand);
    if (foc && std::fmod(GetTime(), 1.0) < 0.55)
        DrawRectangle((int)(r.x + 14 + Wid(vis, 20) + 3), (int)ty, 10, 20, p.amber);
}

inline void Header(const std::string& sub);

// ---------------------------------------------------------------------------
//  Sprites (rectangulos, sin imagenes externas)
// ---------------------------------------------------------------------------
struct SprRect { int x, y, w, h; unsigned int c; };

inline void DrawSprite(const SprRect* s, int n, float x, float y, float u) {
    for (int i = 0; i < n; i++)
        DrawRectangleRec(Rectangle{x + s[i].x * u, y + s[i].y * u, s[i].w * u, s[i].h * u},
                         Hex(s[i].c));
}

inline void DrawRunner(float cx, float cy, float h) {  // centrado en (cx,cy), alto h
    static const SprRect spr[] = {
        {2, 0, 4, 1, 0x4a2f1b}, {2, 1, 4, 2, 0xe6b88a}, {3, 1, 1, 1, 0x1b1b1b},
        {5, 1, 1, 1, 0x1b1b1b}, {1, 3, 6, 2, 0xf2b53b}, {0, 4, 1, 1, 0xe6b88a},
        {7, 4, 1, 1, 0xe6b88a}, {2, 5, 4, 1, 0x6b4423}, {2, 6, 2, 3, 0x2c3a4a},
        {4, 6, 2, 3, 0x2c3a4a}, {2, 9, 2, 1, 0x14100a}, {4, 9, 2, 1, 0x14100a}};
    float u = h / 10.f;
    DrawSprite(spr, 12, cx - 4 * u, cy - 5 * u, u);
}

inline void DrawDoor(float cx, float cy, float h) {
    static const SprRect spr[] = {{0, 0, 8, 8, 0x6f7d7b}, {1, 1, 6, 7, 0x4a5557},
                                  {2, 2, 4, 6, 0xf2b53b}, {3, 3, 2, 2, 0xfff0b0},
                                  {2, 2, 1, 1, 0x4a5557}, {5, 2, 1, 1, 0x4a5557}};
    float u = h / 8.f;
    DrawSprite(spr, 6, cx - 4 * u, cy - 4 * u, u);
}

// ---------------------------------------------------------------------------
//  Datos del laberinto + salidas del Main
// ---------------------------------------------------------------------------
struct Data {
    std::vector<std::string> g;
    int R = 0, C = 0, A = -1, B = -1;
    int opens = 0, walls = 0;
    std::vector<int> pre, post, dfsSol, bfsRoute, bfsSol;
    std::vector<int> rankPre, rankPost, rankBfs;
    std::vector<int> dist;       // distancia BFS desde A (-1 = inalcanzable)
    std::vector<int> bfsReveal;  // para el radar: rango BFS que revela cada muro
    std::vector<float> pingR;    // radio del ping (en celdas) tras explorar k nodos del BFS

    bool open(int r, int c) const {
        if (r < 0 || r >= R || c < 0 || c >= C) return false;
        if (c >= (int)g[r].size()) return false;
        return g[r][c] != '0';
    }
};

inline bool ParseCoord(const std::string& s, const Data& d, int& idx) {
    int r, c;
    if (std::sscanf(s.c_str(), "(%d,%d)", &r, &c) != 2) return false;
    if (r < 0 || r >= d.R || c < 0 || c >= d.C) return false;
    idx = r * d.C + c;
    return true;
}

inline std::vector<int> ParseList(const std::vector<std::string>& in, const Data& d) {
    std::vector<int> out;
    for (const auto& s : in) {
        int idx;
        if (ParseCoord(s, d, idx)) out.push_back(idx);
    }
    return out;
}

inline std::vector<int> RankOf(const std::vector<int>& order, int cells) {
    std::vector<int> rk(cells, -1);
    for (size_t k = 0; k < order.size(); k++) rk[order[k]] = (int)k;
    return rk;
}

inline int ExitCount(const std::vector<int>& order, int exitIdx) {
    for (size_t k = 0; k < order.size(); k++)
        if (order[k] == exitIdx) return (int)k + 1;
    return (int)order.size();
}

inline Data BuildData(const std::vector<std::string>& matrix, std::pair<int, int> A,
                      std::pair<int, int> B, const std::vector<std::string>& pre,
                      const std::vector<std::string>& post, const std::vector<std::string>& dsol,
                      const std::vector<std::string>& broute, const std::vector<std::string>& bsol) {
    Data d;
    d.g = matrix;
    d.R = (int)matrix.size();
    for (const auto& row : matrix) d.C = std::max(d.C, (int)row.size());
    if (d.R == 0 || d.C == 0) return d;
    d.A = A.first * d.C + A.second;
    d.B = B.first * d.C + B.second;
    for (int r = 0; r < d.R; r++)
        for (int c = 0; c < d.C; c++) (d.open(r, c) ? d.opens : d.walls)++;

    d.pre = ParseList(pre, d);
    d.post = ParseList(post, d);
    d.dfsSol = ParseList(dsol, d);
    d.bfsRoute = ParseList(broute, d);
    d.bfsSol = ParseList(bsol, d);
    int cells = d.R * d.C;
    d.rankPre = RankOf(d.pre, cells);
    d.rankPost = RankOf(d.post, cells);
    d.rankBfs = RankOf(d.bfsRoute, cells);

    // distancias BFS (mismas reglas que el Main: 4 vecinos, '0' = pared)
    d.dist.assign(cells, -1);
    std::queue<int> q;
    d.dist[d.A] = 0;
    q.push(d.A);
    const int dr[4] = {-1, 1, 0, 0}, dc[4] = {0, 0, -1, 1};
    while (!q.empty()) {
        int i = q.front();
        q.pop();
        int r = i / d.C, c = i % d.C;
        for (int k = 0; k < 4; k++) {
            int nr = r + dr[k], nc = c + dc[k];
            if (!d.open(nr, nc)) continue;
            int j = nr * d.C + nc;
            if (d.dist[j] == -1) {
                d.dist[j] = d.dist[i] + 1;
                q.push(j);
            }
        }
    }

    // radar: radio del ping y revelado de muros
    d.pingR.clear();
    float mx = 0;
    for (int i : d.bfsRoute) {
        float dy = (float)(i / d.C - d.A / d.C), dx = (float)(i % d.C - d.A % d.C);
        mx = std::max(mx, std::sqrt(dx * dx + dy * dy) + 0.5f);
        d.pingR.push_back(mx);
    }
    d.bfsReveal.assign(cells, INT_MAX);
    for (int r = 0; r < d.R; r++)
        for (int c = 0; c < d.C; c++) {
            if (d.open(r, c)) continue;
            int best = INT_MAX;
            for (int a = -1; a <= 1; a++)
                for (int b = -1; b <= 1; b++) {
                    int nr = r + a, nc = c + b;
                    if (!d.open(nr, nc)) continue;
                    int rk = d.rankBfs[nr * d.C + nc];
                    if (rk >= 0) best = std::min(best, rk);
                }
            d.bfsReveal[r * d.C + c] = best;
        }
    return d;
}

// ---------------------------------------------------------------------------
//  Vista de un tablero
// ---------------------------------------------------------------------------
struct View {
    const std::vector<int>* rank = nullptr;
    const std::vector<int>* path = nullptr;
    int explored = 0, n = 1, pathShown = 0, runner = -1;
    bool radar = false, dim = false;
    float pingCells = 0, sweepDeg = 0, pingAlpha = 1;
    Color lo{}, hi{}, dimc{};
};

inline void Bevel(float x, float y, float s, float a) {
    float t = std::max(1.f, std::floor(s / 6.f));
    DrawRectangle((int)x, (int)y, (int)s, (int)t, Fade(WHITE, a));
    DrawRectangle((int)x, (int)y, (int)t, (int)s, Fade(WHITE, a));
    DrawRectangle((int)x, (int)(y + s - t), (int)s, (int)t, Fade(BLACK, 0.35f));
    DrawRectangle((int)(x + s - t), (int)y, (int)t, (int)s, Fade(BLACK, 0.35f));
}

inline void DrawBoard(const Data& d, Rectangle area, const View& v) {
    const Palette& p = P();
    DrawRectangleRec(area, v.radar ? Hex(0x0a1113) : p.stoneDark);
    if (d.R == 0 || d.C == 0) return;
    int cs = (int)std::floor(std::min((area.width - 8) / d.C, (area.height - 8) / d.R));
    if (cs < 3) cs = 3;
    float ox = std::floor(area.x + (area.width - cs * d.C) / 2.f);
    float oy = std::floor(area.y + (area.height - cs * d.R) / 2.f);

    std::vector<char> onPath(d.R * d.C, 0);
    if (v.path)
        for (int k = 0; k < v.pathShown && k < (int)v.path->size(); k++) onPath[(*v.path)[k]] = 1;

    static const Color wallVar[4] = {Hex(0x4a5557), Hex(0x52605f), Hex(0x434d4f), Hex(0x566362)};
    static const Color radarWall[4] = {Hex(0x1d3a40), Hex(0x234650), Hex(0x1a3137), Hex(0x285058)};

    for (int r = 0; r < d.R; r++)
        for (int c = 0; c < d.C; c++) {
            int i = r * d.C + c;
            float x = ox + c * cs, y = oy + r * cs;
            if (!d.open(r, c)) {
                int h = (r * 31 + c * 17 + r * c * 3) % 7;
                if (v.radar) {
                    if (d.bfsReveal[i] < v.explored) {
                        DrawRectangle((int)x, (int)y, cs, cs, radarWall[h % 4]);
                        Bevel(x, y, (float)cs, 0.18f);
                    }
                } else {
                    bool border = r == 0 || c == 0 || r == d.R - 1 || c == d.C - 1;
                    Color col = border ? p.stoneDark
                                       : (h < 3 ? (h == 0 ? Hex(0x3f7a3a) : p.ivy)
                                                : wallVar[h - 3]);
                    DrawRectangle((int)x, (int)y, cs, cs, col);
                    Bevel(x, y, (float)cs, 0.12f);
                }
                continue;
            }
            int rk = v.rank ? (*v.rank)[i] : -1;
            if (onPath[i]) {
                DrawRectangle((int)x, (int)y, cs, cs, p.path);
                Bevel(x, y, (float)cs, 0.35f);
            } else if (rk >= 0 && rk < v.explored) {
                Color col;
                bool fresh = false;
                if (v.radar) {
                    int age = v.explored - 1 - rk;
                    float f = v.dim ? 1.f : std::min(1.f, age / 16.f);
                    col = Mix(Color{200, 250, 255, 255}, Color{30, 84, 98, 255}, f);
                    fresh = !v.dim && age < 2;
                } else {
                    float t = v.n > 1 ? (float)rk / (v.n - 1) : 0.f;
                    col = v.dim ? Mix(v.lo, v.dimc, t) : Mix(v.lo, v.hi, t);
                }
                DrawRectangle((int)x, (int)y, cs, cs, col);
                if (fresh)
                    DrawRectangleLinesEx(Rectangle{x, y, (float)cs, (float)cs}, 2, Hex(0xeffcff));
                else
                    DrawRectangleLines((int)x, (int)y, cs, cs, Fade(BLACK, 0.28f));
            } else {
                DrawRectangle((int)x, (int)y, cs, cs, v.radar ? Hex(0x0c1517) : ((r + c) % 2 ? p.floorB : p.floorA));
            }
        }

    // radar: anillos, ping y haz giratorio
    if (v.radar) {
        float ax = ox + (d.A % d.C) * cs + cs / 2.f, ay = oy + (d.A / d.C) * cs + cs / 2.f;
        Vector2 center{ax, ay};
        BeginScissorMode((int)area.x, (int)area.y, (int)area.width, (int)area.height);
        Color ringC = Fade(p.bfs, 0.30f);
        float maxR = std::sqrt(area.width * area.width + area.height * area.height);
        for (float rr = 3.f * cs; rr < maxR; rr += 3.f * cs)
            for (int a = 0; a < 360; a += 12)
                DrawRing(center, rr - 1, rr + 1, (float)a, (float)(a + 6), 3, ringC);
        DrawLineEx(Vector2{area.x, ay}, Vector2{area.x + area.width, ay}, 1, ringC);
        DrawLineEx(Vector2{ax, area.y}, Vector2{ax, area.y + area.height}, 1, ringC);
        if (v.pingCells > 0)
            DrawRing(center, v.pingCells * cs - 2, v.pingCells * cs + 2, 0, 360, 72,
                     Fade(p.bfs, v.pingAlpha));
        const int slices = 16;
        for (int s = 0; s < slices; s++) {
            float a1 = v.sweepDeg - (slices - s) * 5.f, a2 = a1 + 5.f;
            DrawCircleSector(center, maxR, a1, a2, 2, Fade(p.bfs, 0.015f + 0.22f * s / slices));
        }
        float la = v.sweepDeg * DEG2RAD;
        DrawLineEx(center, Vector2{ax + std::cos(la) * maxR, ay + std::sin(la) * maxR}, 2,
                   Fade(Hex(0xbefaff), 0.85f));
        EndScissorMode();
    }

    // salida, entrada y corredor
    float ex = ox + (d.B % d.C) * cs + cs / 2.f, ey = oy + (d.B / d.C) * cs + cs / 2.f;
    DrawDoor(ex, ey, cs * 0.9f);
    int ri = v.runner >= 0 ? v.runner : d.A;
    float rx = ox + (ri % d.C) * cs + cs / 2.f, ry = oy + (ri / d.C) * cs + cs / 2.f;
    DrawRectangleLinesEx(Rectangle{ox + (d.A % d.C) * cs, oy + (d.A / d.C) * cs, (float)cs, (float)cs},
                         2, p.amber);
    DrawRunner(rx, ry, cs * 0.9f);
}

// ---------------------------------------------------------------------------
//  Aplicacion
// ---------------------------------------------------------------------------
enum class Screen { Title, Load, Main, Anim, Save };
enum Mode { M_DFS_PRE = 0, M_DFS_POST, M_DFS_SOL, M_BFS_ROUTE, M_BFS_SOL };

struct FileEntry { std::string name; long long size = 0; };

struct App {
    MazeSolver solver;
    FileSaver saver;
    MazeUIResult res;      // ultimo laberinto resuelto
    Data d;
    bool loaded = false;
    std::string loadedName;
    Screen screen = Screen::Title;
    int mode = M_DFS_SOL;
    int step = 0, speed = 1, nD = 1, nB = 1, maxStep = 1;
    bool playing = false;
    float acc = 0.f;

    // pantalla "cargar"
    std::string fileName = "archivo.in";
    int focus = 1;
    std::vector<FileEntry> files;
    int scroll = 0, lastClickIdx = -1;
    double lastClickTime = 0;
    std::string msg;
    bool msgErr = false;

    // pantalla "descargar"
    std::string outDir = "salidas", prefix;
    std::string savedFor[5];
    std::string saveMsg;
    bool saveErr = false;
};

inline void Header(const std::string& sub) {
    const Palette& p = P();
    DrawRectangle(0, 0, WIN_W, 40, p.panel2);
    DrawRectangle(0, 40, WIN_W, 4, p.stone);
    DrawRunner(30, 20, 26);
    Txt("RUNNER.EXE", 52, 11, 20, p.amber);
    Txt(sub, 52 + Wid("RUNNER.EXE", 20) + 16, 11, 20, p.gray);
    for (int x = 7; x < WIN_W; x += 43 + (x % 5) * 7) {  // hiedra colgando
        DrawRectangle(x, 44, 4, 4 + (x * 7) % 9, p.ivyDark);
        DrawRectangle(x + 4, 44, 4, 4 + (x * 3) % 6, p.ivy);
    }
}

struct Stat { std::string label, value; Color color; };

inline void StatCards(float x, float y, float w, const std::vector<Stat>& st) {
    float gap = 8, cw = (w - gap * (st.size() - 1)) / st.size();
    for (size_t i = 0; i < st.size(); i++) {
        Rectangle r{x + i * (cw + gap), y, cw, 46};
        DrawRectangleRec(r, P().night);
        Txt(st[i].label, r.x + 8, r.y + 4, 10, P().gray);
        int size = Wid(st[i].value, 20) <= cw - 12 ? 20 : 10;
        Txt(st[i].value, r.x + 8, r.y + (size == 20 ? 20 : 26), size, st[i].color);
    }
}

inline const char* kLabel[5] = {"DFS PREORDEN", "DFS POSTORDEN", "SOLUCION DFS", "RECORRIDO BFS",
                                "SOLUCION BFS"};
inline const char* kFile[5] = {"dfs_preorder_route.txt", "dfs_postorder_route.txt",
                               "dfs_solution.txt", "bfs_route.txt", "bfs_solution.txt"};

inline View ViewForMode(const App& a, int m) {
    const Data& d = a.d;
    const Palette& p = P();
    View v;
    v.lo = Color{110, 40, 22, 255};
    v.hi = Color{240, 140, 60, 255};
    v.dimc = Color{150, 64, 34, 255};
    v.runner = d.A;
    switch (m) {
        case M_DFS_PRE:
            v.rank = &d.rankPre; v.path = &d.dfsSol;
            v.explored = (int)d.pre.size(); v.n = v.explored;
            break;
        case M_DFS_POST:
            v.rank = &d.rankPost; v.path = &d.dfsSol;
            v.explored = (int)d.post.size(); v.n = v.explored;
            break;
        case M_DFS_SOL:
            v.rank = &d.rankPre; v.path = &d.dfsSol;
            v.explored = a.nD; v.n = a.nD; v.pathShown = (int)d.dfsSol.size(); v.dim = true;
            break;
        case M_BFS_ROUTE:
            v.rank = &d.rankBfs; v.path = &d.bfsSol; v.radar = true;
            v.explored = (int)d.bfsRoute.size(); v.n = v.explored;
            break;
        default:
            v.rank = &d.rankBfs; v.path = &d.bfsSol; v.radar = true;
            v.explored = (int)d.bfsRoute.size(); v.n = v.explored;
            v.pathShown = (int)d.bfsSol.size(); v.dim = true;
            break;
    }
    if (v.radar) {
        v.pingCells = v.explored > 0 ? d.pingR[v.explored - 1] : 0;
        v.pingAlpha = 0.3f;
        v.sweepDeg = std::fmod((float)GetTime() * 90.f, 360.f);
    }
    (void)p;
    return v;
}

// ---------------------------------------------------------------------------
//  Pantalla: portada
// ---------------------------------------------------------------------------
inline void DrawTitleScreen(App& a) {
    const Palette& p = P();
    ClearBackground(p.night);
    const int floorY = WIN_H - 80, wallW = 260;
    // haz de luz del pasillo
    DrawRectangleGradientH(wallW, 0, (WIN_W - 2 * wallW) / 2, floorY, p.night, Hex(0xf2b53b, 255));
    DrawRectangleGradientH(WIN_W / 2, 0, (WIN_W - 2 * wallW) / 2, floorY, Hex(0xf2b53b, 255), p.night);
    DrawRectangleGradientV(wallW, 0, WIN_W - 2 * wallW, 330, p.night, Fade(p.night, 0.f));
    // muros de piedra con hiedra
    for (int side = 0; side < 2; side++) {
        int x0 = side == 0 ? 0 : WIN_W - wallW;
        DrawRectangle(x0, 0, wallW, floorY, p.stone);
        for (int y = 0; y < floorY; y += 40) {
            DrawRectangle(x0, y, wallW, 4, p.stoneDark);
            for (int x = ((y / 40) % 2) * 34; x < wallW; x += 68) DrawRectangle(x0 + x, y, 4, 40, p.stoneDark);
        }
        DrawRectangle(side == 0 ? wallW - 8 : x0, 0, 8, floorY, p.stoneHi);
        for (int k = 0; k < 6; k++) {
            int ix = side == 0 ? wallW - 30 - k * 36 : x0 + 12 + k * 36;
            DrawRectangle(ix, 0, 12, 60 + (k * 53) % 140, p.ivyDark);
            DrawRectangle(ix + 12, 0, 8, 30 + (k * 37) % 90, p.ivy);
        }
    }
    DrawRectangle(0, floorY, WIN_W, 80, p.stoneDark);
    DrawRectangle(0, floorY, WIN_W, 8, p.stoneHi);

    // titulo
    Rectangle tp{(WIN_W - 560) / 2.f, 40, 560, 220};
    Panel(tp, p.night, p.stone);
    std::string t0 = "ESTRUCTURA DE DATOS";
    Txt(t0, tp.x + (tp.width - Wid(t0, 20)) / 2, tp.y + 22, 20, p.gray);
    Txt("RUNNER", tp.x + 56 + 5, tp.y + 62 + 5, 60, Hex(0x7a4a10));
    Txt("RUNNER", tp.x + 56, tp.y + 62, 60, p.amber);
    Txt(".EXE", tp.x + 56 + Wid("RUNNER", 60) + 6, tp.y + 100, 30, p.sand);
    std::string t1 = "Solucionador de laberintos";
    Txt(t1, tp.x + (tp.width - Wid(t1, 30)) / 2, tp.y + 138, 30, p.sand);
    float cx = tp.x + tp.width / 2;
    Rectangle chip1{cx - 150, tp.y + 180, 60, 28}, chip2{cx - 20, tp.y + 180, 60, 28};
    DrawRectangleRec(chip1, p.dfs);
    TxtC("DFS", chip1, 20, Hex(0x14100a));
    Txt("vs", cx - 78, tp.y + 184, 20, p.gray);
    DrawRectangleRec(chip2, p.bfs);
    TxtC("BFS", chip2, 20, Hex(0x14100a));
    Txt("arboles", cx + 54, tp.y + 184, 20, p.gray);

    // boton START
    Rectangle sb{WIN_W / 2.f - 100, 290, 200, 60};
    if (Button(sb, "START", 30, p.amber, p.amberHi, Hex(0x1b1406)) || IsKeyPressed(KEY_ENTER))
        a.screen = Screen::Load;

    std::string info = "Elige tu laberinto dentro de la app";
    Txt(info, (WIN_W - Wid(info, 20)) / 2.f, 366, 20, p.sand);

    // puerta + corredor
    Rectangle door{WIN_W / 2.f - 70, floorY - 190, 140, 190};
    DrawRectangleRec(door, Hex(0x14100a));
    DrawRectangleLinesEx(door, 4, p.stoneDark);
    DrawRunner(WIN_W / 2.f, floorY - 50, 80);

    DrawRectangle(0, WIN_H - 36, WIN_W, 36, p.console);
    std::string foot = "Estructura de datos  -  Runner.exe v1.0   |   ENTER = empezar";
    Txt(foot, (WIN_W - Wid(foot, 20)) / 2.f, WIN_H - 28, 20, p.gray);
}

// ---------------------------------------------------------------------------
//  Pantalla: app principal (5 salidas del Main)
// ---------------------------------------------------------------------------
inline void DrawMainScreen(App& a) {
    const Palette& p = P();
    const Data& d = a.d;
    ClearBackground(p.night);
    Header(Fit(a.loadedName, 420, 20) + "  |  DFS vs BFS");

    bool bfs = a.mode >= M_BFS_ROUTE;
    Color accent = bfs ? p.bfs : p.dfs;
    View v = ViewForMode(a, a.mode);

    // columna izquierda
    char buf[160];
    std::snprintf(buf, sizeof buf, "MAPA   %d x %d", d.R, d.C);
    Txt(buf, 16, 62, 20, p.amber);
    Rectangle tag{720.f - 20 - Wid(kLabel[a.mode], 20), 56, (float)Wid(kLabel[a.mode], 20) + 20, 30};
    DrawRectangleRec(tag, accent);
    TxtC(kLabel[a.mode], tag, 20, Hex(0x14100a));

    Rectangle mapPanel{16, 94, 704, 396};
    Panel(mapPanel, p.stoneDark, p.stone);
    DrawBoard(d, Rectangle{mapPanel.x + 10, mapPanel.y + 10, mapPanel.width - 20, mapPanel.height - 20}, v);

    // leyenda
    struct Leg { const char* t; Color c; };
    Leg leg[] = {{"Muro", p.stone}, {"Hiedra", p.ivy}, {"Camino", p.floorA},
                 {"Explorado", accent}, {"Ruta", p.path}, {"Salida", p.amber}};
    float lx = 16;
    for (auto& l : leg) {
        DrawRectangle((int)lx, 500, 14, 14, l.c);
        Txt(l.t, lx + 20, 498, 20, p.gray);
        lx += 20 + Wid(l.t, 20) + 18;
    }

    if (Button(Rectangle{16, 528, 210, 44}, "ANIMAR", 20, p.amber, p.amberHi, Hex(0x1b1406))) {
        a.screen = Screen::Anim;
        a.step = 0; a.playing = false; a.acc = 0;
    }
    if (Button(Rectangle{238, 528, 210, 44}, "DESCARGAR", 20, p.amber, p.amberHi, Hex(0x1b1406))) {
        a.screen = Screen::Save;
        a.focus = 2; a.saveMsg.clear(); a.saveErr = false;
    }
    if (Button(Rectangle{460, 528, 200, 44}, "LABERINTO", 20, Hex(0x3b4547), Hex(0x566362), p.sand)) {
        a.screen = Screen::Load;
        a.focus = 1; a.msg.clear(); a.msgErr = false;
    }

    // consola
    Rectangle con{16, 584, 704, 60};
    DrawRectangleRec(con, p.console);
    DrawRectangleLinesEx(con, 4, Hex(0x2f3638));
    char l1[200];
    int steps = v.pathShown > 0 ? v.pathShown - 1 : -1;
    switch (a.mode) {
        case M_DFS_PRE: std::snprintf(l1, sizeof l1, "> DFS preorden: %d nodos numerados al entrar", v.explored); break;
        case M_DFS_POST: std::snprintf(l1, sizeof l1, "> DFS postorden: %d nodos numerados al salir", v.explored); break;
        case M_DFS_SOL: std::snprintf(l1, sizeof l1, "> DFS: %d nodos hasta B - ruta de %d pasos", v.explored, steps); break;
        case M_BFS_ROUTE: std::snprintf(l1, sizeof l1, "> BFS: %d nodos por niveles - %d niveles", v.explored, d.dist[d.B]); break;
        default: std::snprintf(l1, sizeof l1, "> BFS: %d nodos - ruta mas corta de %d pasos", v.explored, steps); break;
    }
    if ((v.pathShown == 0 && (a.mode == M_DFS_SOL || a.mode == M_BFS_SOL)))
        std::snprintf(l1, sizeof l1, "> No hay camino de A a B en este laberinto");
    Txt(l1, 28, 592, 20, p.green);
    std::string l2 = std::string("> Archivo: ") + kFile[a.mode];
    Txt(l2, 28, 616, 20, p.green);

    // columna derecha: salidas
    Rectangle sp{736, 56, 368, 292};
    Section(sp, "SALIDAS");
    for (int i = 0; i < 5; i++) {
        Rectangle br{sp.x + 16, sp.y + 44 + i * 48.f, sp.width - 32, 42};
        bool sel = a.mode == i;
        Color acc = i >= M_BFS_ROUTE ? p.bfs : p.dfs;
        if (Hover(br) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) a.mode = i;
        Panel(br, sel ? Hex(0x2a3335) : p.panel, sel ? acc : Hex(0x3b4547));
        Rectangle nb{br.x + 4, br.y + 4, 34, br.height - 8};
        DrawRectangleRec(nb, sel ? acc : Hex(0x3b4547));
        TxtC(std::to_string(i + 1), nb, 20, sel ? Hex(0x14100a) : p.sand);
        Txt(kLabel[i], br.x + 48, br.y + 6, 20, p.sand);
        Txt(kFile[i], br.x + 48, br.y + 27, 10, p.gray);
    }
    for (int i = 0; i < 5; i++)
        if (IsKeyPressed(KEY_ONE + i)) a.mode = i;

    // columna derecha: estadisticas
    Rectangle st{736, 364, 368, 280};
    Section(st, "ESTADISTICAS");
    Txt("Nodos explorados", st.x + 16, st.y + 46, 20, p.gray);
    snprintf(buf, sizeof buf, "%d / %d", v.explored, d.opens);
    TxtR(buf, st.x + st.width - 16, st.y + 46, 20, p.sand);
    int filled = d.opens > 0 ? (int)std::round(24.0 * v.explored / d.opens) : 0;
    float bw = (st.width - 32 - 23 * 3) / 24.f;
    for (int k = 0; k < 24; k++)
        DrawRectangle((int)(st.x + 16 + k * (bw + 3)), (int)st.y + 74, (int)bw, 16, k < filled ? accent : Hex(0x2b3436));

    int dsteps = d.dfsSol.empty() ? -1 : (int)d.dfsSol.size() - 1;
    int bsteps = d.bfsSol.empty() ? -1 : (int)d.bfsSol.size() - 1;
    auto num = [](int n) { return n < 0 ? std::string("--") : std::to_string(n) + " pasos"; };
    Txt("Ruta DFS", st.x + 16, st.y + 108, 20, p.dfs);
    TxtR(num(dsteps), st.x + st.width - 16, st.y + 108, 20, p.sand);
    Txt("Ruta BFS", st.x + 16, st.y + 136, 20, p.bfs);
    TxtR(num(bsteps), st.x + st.width - 16, st.y + 136, 20, p.sand);
    Txt("Niveles BFS hasta B", st.x + 16, st.y + 164, 20, p.gray);
    TxtR(d.dist[d.B] < 0 ? "--" : std::to_string(d.dist[d.B]), st.x + st.width - 16, st.y + 164, 20, p.sand);
    std::string verdict;
    if (dsteps >= 0 && bsteps >= 0) {
        int diff = dsteps - bsteps;
        verdict = diff > 0 ? "BFS acorta " + std::to_string(diff) + " pasos" : "Ambas rutas miden igual";
    } else verdict = "Sin ruta de A a B";
    DrawRectangle((int)st.x + 16, (int)st.y + 198, (int)st.width - 32, 4, Hex(0x2b3436));
    Txt(verdict, st.x + 16, st.y + 216, 20, p.path);
    Txt("Teclas 1-5 cambian de salida", st.x + 16, st.y + 250, 10, p.gray);
}

// ---------------------------------------------------------------------------
//  Pantalla: animacion DFS vs BFS (radar)
// ---------------------------------------------------------------------------
struct AnimInfo {
    View v;
    std::string status, nodes, route, level;
    Color statusColor{};
};

inline AnimInfo MakeAnim(const App& a, bool bfs) {
    const Data& d = a.d;
    const Palette& p = P();
    AnimInfo o;
    View& v = o.v;
    const std::vector<int>& order = bfs ? d.bfsRoute : d.pre;
    const std::vector<int>& path = bfs ? d.bfsSol : d.dfsSol;
    int n = bfs ? a.nB : a.nD;
    int L = (int)path.size();
    v.rank = bfs ? &d.rankBfs : &d.rankPre;
    v.path = &path;
    v.n = n;
    v.radar = bfs;
    v.lo = bfs ? Color{28, 72, 84, 255} : Color{110, 40, 22, 255};
    v.hi = bfs ? Color{110, 215, 232, 255} : Color{240, 140, 60, 255};
    v.dimc = bfs ? Color{40, 110, 124, 255} : Color{150, 64, 34, 255};
    int step = a.step, phase = 0;
    v.explored = std::min(step, n);
    v.runner = d.A;
    if (step < n) {
        if (!bfs && step > 0) v.runner = order[step - 1];
    } else if (L > 0) {
        int k = std::min(step - n, L - 1);
        v.pathShown = k + 1;
        v.runner = path[k];
        phase = k >= L - 1 ? 2 : 1;
    } else {
        phase = 3;
    }
    v.dim = phase >= 1;
    v.pingCells = (bfs && v.explored > 0) ? d.pingR[v.explored - 1] : 0;
    v.pingAlpha = phase == 0 ? 1.f : 0.3f;
    v.sweepDeg = std::fmod(step * 24.f, 360.f);

    o.status = phase == 0 ? (bfs ? "ESCANEANDO" : "EXPLORANDO")
               : phase == 1 ? "RECORRIDO" : phase == 2 ? "SALIDA!" : "SIN RUTA";
    o.statusColor = phase == 2 ? p.path : phase == 3 ? p.red : (bfs ? p.bfs : p.dfs);
    o.nodes = std::to_string(v.explored) + " / " + std::to_string(d.opens);
    o.route = phase >= 1 && L > 0 ? std::to_string(v.pathShown) + " / " + std::to_string(L) : "--";
    int lvl = (bfs && v.explored > 0) ? d.dist[order[v.explored - 1]] : 0;
    o.level = std::to_string(lvl) + " / " + (d.dist[d.B] < 0 ? std::string("--") : std::to_string(d.dist[d.B]));
    return o;
}

inline void DrawPane(const App& a, bool bfs, Rectangle r) {
    const Palette& p = P();
    AnimInfo o = MakeAnim(a, bfs);
    Color acc = bfs ? p.bfs : p.dfs;
    Panel(r, p.panel, p.stone);
    DrawRectangle((int)r.x + 16, (int)r.y + 16, 12, 12, acc);
    Txt(bfs ? "BFS - RADAR" : "DFS - PROFUNDIDAD", r.x + 36, r.y + 12, 20, acc);
    TxtR(bfs ? "cola, ping por niveles" : "recursivo, pila", r.x + r.width - 16, r.y + 14, 10, p.gray);
    Rectangle board{r.x + 12, r.y + 40, r.width - 24, 280};
    DrawBoard(a.d, board, o.v);
    std::vector<Stat> st = {{"NODOS", o.nodes, p.sand}};
    if (bfs) st.push_back({"NIVEL", o.level, p.sand});
    st.push_back({"RUTA", o.route, p.sand});
    st.push_back({"ESTADO", o.status, o.statusColor});
    StatCards(r.x + 12, r.y + 328, r.width - 24, st);
}

inline void DrawAnimScreen(App& a) {
    const Palette& p = P();
    ClearBackground(p.night);
    Header("animacion  |  DFS vs BFS");

    // avance del tiempo
    if (a.playing) {
        a.acc += GetFrameTime();
        while (a.acc >= 0.14f) {
            a.acc -= 0.14f;
            a.step += a.speed;
            if (a.step >= a.maxStep) { a.step = a.maxStep; a.playing = false; break; }
        }
    }

    if (Button(Rectangle{16, 54, 140, 40}, "VOLVER", 20, Hex(0x3b4547), Hex(0x566362), p.sand) ||
        IsKeyPressed(KEY_ESCAPE)) {
        a.screen = Screen::Main; a.playing = false;
    }
    Txt("ANIMACION", 176, 64, 20, p.amber);
    Txt("mismo laberinto, dos estrategias", 176 + Wid("ANIMACION", 20) + 16, 64, 20, p.gray);

    DrawPane(a, false, Rectangle{16, 104, 536, 380});
    DrawPane(a, true, Rectangle{568, 104, 536, 380});

    // controles
    Rectangle cp{16, 496, 1088, 148};
    Panel(cp, p.panel, p.stone);
    float y = cp.y + 14;
    Color sf = Hex(0x3b4547), sb = Hex(0x566362);
    if (Button(Rectangle{32, y, 48, 44}, "|<", 20, sf, sb, p.sand)) { a.step = 0; a.playing = false; }
    if (Button(Rectangle{88, y, 48, 44}, "<", 20, sf, sb, p.sand) || IsKeyPressed(KEY_LEFT)) {
        a.step = std::max(0, a.step - 1); a.playing = false;
    }
    if (Button(Rectangle{144, y, 96, 44}, a.playing ? "PAUSA" : "PLAY", 20, p.amber, p.amberHi, Hex(0x1b1406)) ||
        IsKeyPressed(KEY_SPACE)) {
        if (a.playing) a.playing = false;
        else { if (a.step >= a.maxStep) a.step = 0; a.playing = true; a.acc = 0; }
    }
    if (Button(Rectangle{248, y, 48, 44}, ">", 20, sf, sb, p.sand) || IsKeyPressed(KEY_RIGHT)) {
        a.step = std::min(a.maxStep, a.step + 1); a.playing = false;
    }
    Txt("Velocidad", 330, y + 12, 20, p.gray);
    const int speeds[3] = {1, 2, 4};
    for (int i = 0; i < 3; i++) {
        bool sel = a.speed == speeds[i];
        if (Button(Rectangle{450.f + i * 64, y, 56, 44}, "x" + std::to_string(speeds[i]), 20,
                   sel ? p.amber : sf, sel ? p.amberHi : sb, sel ? Hex(0x1b1406) : p.sand))
            a.speed = speeds[i];
    }
    std::string stepTxt = "PASO " + std::to_string(a.step) + " / " + std::to_string(a.maxStep);
    TxtR(stepTxt, cp.x + cp.width - 20, y + 12, 20, p.sand);

    // barra de tiempo
    Rectangle sl{32, cp.y + 76, cp.width - 32, 22};
    Rectangle hit{sl.x, sl.y - 8, sl.width, sl.height + 16};
    if (Hover(hit) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float t = (GetMouseX() - sl.x) / sl.width;
        a.step = (int)std::round(std::max(0.f, std::min(1.f, t)) * a.maxStep);
        a.playing = false;
    }
    DrawRectangleRec(sl, p.night);
    DrawRectangleLinesEx(sl, 2, Hex(0x3b4547));
    float ratio = a.maxStep > 0 ? (float)a.step / a.maxStep : 0;
    DrawRectangle((int)sl.x + 2, (int)sl.y + 2, (int)((sl.width - 4) * ratio), (int)sl.height - 4, p.amber);
    DrawRectangle((int)(sl.x + (sl.width - 8) * ratio), (int)sl.y - 4, 8, (int)sl.height + 8, p.amberHi);

    Txt("> Fase 1: cada paso expande un nodo  |  Fase 2: el corredor sigue los ancestros hasta B",
        cp.x + 16, cp.y + 112, 20, p.green);
}

// ---------------------------------------------------------------------------
//  Pantalla: cargar laberinto (entrada de texto dentro de la UI)
// ---------------------------------------------------------------------------
inline std::string Lower(std::string t) {
    for (auto& ch : t) ch = (char)std::tolower((unsigned char)ch);
    return t;
}

inline bool EndsWith(const std::string& s, const std::string& suf) {
    return s.size() >= suf.size() && s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
}

// Busca archivos .in / .txt en la carpeta de trabajo (ignora los archivos de salida)
inline void ScanFiles(App& a) {
    namespace fs = std::filesystem;
    a.files.clear();
    std::error_code ec;
    fs::path dir = fs::current_path(ec);
    for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec)) {
        std::error_code e2;
        if (!it->is_regular_file(e2)) continue;
        std::string name = it->path().filename().string();
        std::string ext = Lower(it->path().extension().string());
        if (ext != ".in" && ext != ".txt") continue;
        bool isOut = false;
        for (int i = 0; i < 5; i++)
            if (EndsWith(name, kFile[i])) isOut = true;
        if (isOut) continue;
        FileEntry fe;
        fe.name = name;
        fe.size = (long long)it->file_size(e2);
        a.files.push_back(fe);
    }
    std::sort(a.files.begin(), a.files.end(), [](const FileEntry& x, const FileEntry& y) {
        bool xi = Lower(x.name).size() >= 3 && EndsWith(Lower(x.name), ".in");
        bool yi = Lower(y.name).size() >= 3 && EndsWith(Lower(y.name), ".in");
        if (xi != yi) return xi;
        return Lower(x.name) < Lower(y.name);
    });
    a.scroll = 0;
}

inline bool ValidAB(const MazeUIResult& r) {
    int R = (int)r.matrix.size();
    if (R == 0) return false;
    auto okp = [&](std::pair<int, int> p) {
        return p.first >= 0 && p.first < R && p.second >= 0 && p.second < (int)r.matrix[p.first].size();
    };
    return okp(r.start_A) && okp(r.exit_B);
}

inline void DoLoad(App& a) {
    std::string name = a.fileName;
    while (!name.empty() && name.back() == ' ') name.pop_back();
    while (!name.empty() && name.front() == ' ') name.erase(0, 1);
    if (name.empty()) name = "archivo.in";
    a.msgErr = true;
    if (!a.solver) { a.msg = "ERROR: la UI no recibio la funcion que resuelve el laberinto."; return; }
    MazeUIResult res;
    try {
        res = a.solver(name);
    } catch (...) {
        a.msg = "ERROR: fallo inesperado al resolver el laberinto.";
        return;
    }
    if (!res.ok) { a.msg = "ERROR: " + (res.error.empty() ? std::string("no se pudo cargar el laberinto.") : res.error); return; }
    if (!ValidAB(res)) { a.msg = "ERROR: las posiciones de A y B no son validas."; return; }
    a.d = BuildData(res.matrix, res.start_A, res.exit_B, res.dfs_pre, res.dfs_post, res.dfs_solution,
                    res.bfs_route, res.bfs_solution);
    a.res = res;
    a.loaded = true;
    a.loadedName = name;
    a.nD = ExitCount(a.d.pre, a.d.B);
    a.nB = ExitCount(a.d.bfsRoute, a.d.B);
    int lD = std::max(0, (int)a.d.dfsSol.size() - 1), lB = std::max(0, (int)a.d.bfsSol.size() - 1);
    a.maxStep = std::max(1, std::max(a.nD + lD, a.nB + lB));
    a.step = 0; a.playing = false; a.acc = 0;
    a.mode = M_DFS_SOL;
    for (auto& sf : a.savedFor) sf.clear();
    a.saveMsg.clear();
    a.msg.clear();
    a.msgErr = false;
    a.screen = Screen::Main;
}

inline void DrawLoadScreen(App& a) {
    const Palette& p = P();
    ClearBackground(p.night);
    Header("cargar laberinto");
    if (IsKeyPressed(KEY_ESCAPE)) a.screen = a.loaded ? Screen::Main : Screen::Title;
    bool doLoad = false;

    Rectangle in{16, 60, 1088, 150};
    Section(in, "ARCHIVO DE ENTRADA");
    Txt("Nombre del archivo (puedes escribir una ruta)", 32, 98, 20, p.gray);
    TextBox(Rectangle{32, 124, 760, 48}, a.fileName, 1, a.focus);
    if (Button(Rectangle{808, 124, 150, 48}, "CARGAR", 20, p.amber, p.amberHi, Hex(0x1b1406))) doLoad = true;
    if (Button(Rectangle{970, 124, 118, 48}, a.loaded ? "VOLVER" : "ATRAS", 20, Hex(0x3b4547), Hex(0x566362), p.sand))
        a.screen = a.loaded ? Screen::Main : Screen::Title;
    Txt("ENTER = cargar   |   Ctrl+V pega   |   Supr borra todo   |   ESC = volver", 32, 180, 20, p.gray);

    Rectangle lp{16, 226, 1088, 300};
    Section(lp, "ARCHIVOS EN LA CARPETA");
    std::error_code ec;
    std::string cwd = std::filesystem::current_path(ec).string();
    Txt(Fit("Carpeta: " + cwd, 1040, 10), 32, 262, 10, p.gray);
    if (Button(Rectangle{968, 236, 120, 34}, "RECARGAR", 20, Hex(0x3b4547), Hex(0x566362), p.sand)) ScanFiles(a);

    const int visible = 6, rowH = 36;
    Rectangle listR{32, 282, 1056, visible * (float)rowH};
    int n = (int)a.files.size();
    if (Hover(listR)) a.scroll -= (int)GetMouseWheelMove();
    a.scroll = std::max(0, std::min(a.scroll, std::max(0, n - visible)));
    if (n == 0) Txt("No hay archivos .in ni .txt en esta carpeta. Escribe una ruta arriba.", 32, 296, 20, p.gray);
    for (int k = 0; k < visible; k++) {
        int i = a.scroll + k;
        if (i >= n) break;
        Rectangle rr{32, 282.f + k * rowH, 1040, 32};
        bool sel = a.files[i].name == a.fileName;
        Panel(rr, sel ? Hex(0x2a3335) : p.panel, sel ? p.amber : Hex(0x3b4547));
        if (Hover(rr)) DrawRectangle((int)rr.x + 4, (int)rr.y + 4, (int)rr.width - 8, (int)rr.height - 8, Fade(WHITE, 0.05f));
        Txt(Fit(a.files[i].name, 800, 20), rr.x + 14, rr.y + 6, 20, p.sand);
        char kb[40];
        std::snprintf(kb, sizeof kb, "%.1f KB", a.files[i].size / 1024.0);
        TxtR(kb, rr.x + rr.width - 14, rr.y + 6, 20, p.gray);
        if (Clicked(rr)) {
            a.fileName = a.files[i].name;
            a.focus = 1;
            double now = GetTime();
            if (a.lastClickIdx == i && now - a.lastClickTime < 0.45) doLoad = true;
            a.lastClickIdx = i;
            a.lastClickTime = now;
        }
    }
    if (n > visible) {  // barra de desplazamiento
        float th = listR.height * visible / n, ty = listR.y + (listR.height - th) * a.scroll / std::max(1, n - visible);
        DrawRectangle(1076, (int)listR.y, 8, (int)listR.height, p.night);
        DrawRectangle(1076, (int)ty, 8, (int)th, p.amber);
    }

    Rectangle con{16, 542, 1088, 100};
    DrawRectangleRec(con, p.console);
    DrawRectangleLinesEx(con, 4, Hex(0x2f3638));
    if (a.msg.empty()) {
        Txt("> Escribe el nombre del laberinto o elige uno de la lista, luego pulsa CARGAR", 28, 556, 20, p.green);
        Txt("> Doble clic en un archivo de la lista = cargarlo directamente", 28, 584, 20, p.green);
    } else {
        Txt(Fit("> " + a.msg, 1050, 20), 28, 556, 20, a.msgErr ? Hex(0xff7a5c) : p.green);
        Txt("> Revisa el nombre (la carpeta de arriba es donde se busca el archivo)", 28, 584, 20, p.gray);
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) doLoad = true;
    if (doLoad) DoLoad(a);
}

// ---------------------------------------------------------------------------
//  Pantalla: descargar (guardar) los 5 archivos
// ---------------------------------------------------------------------------
inline const std::vector<std::string>& OutData(const App& a, int i) {
    switch (i) {
        case 0: return a.res.dfs_pre;
        case 1: return a.res.dfs_post;
        case 2: return a.res.dfs_solution;
        case 3: return a.res.bfs_route;
        default: return a.res.bfs_solution;
    }
}

inline std::filesystem::path OutPath(const App& a, int i) {
    std::filesystem::path dir = a.outDir.empty() ? std::filesystem::path(".") : std::filesystem::path(a.outDir);
    return dir / (a.prefix + kFile[i]);
}

inline bool SaveOne(App& a, int i, std::string& err) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path full = OutPath(a, i);
    if (full.has_parent_path()) fs::create_directories(full.parent_path(), ec);
    if (ec) { err = "No se pudo crear la carpeta: " + ec.message(); return false; }
    try {
        if (a.saver) {
            a.saver(full.string(), OutData(a, i));
        } else {
            std::ofstream out(full);
            const auto& v = OutData(a, i);
            for (size_t k = 0; k < v.size(); k++) out << v[k] << (k + 1 < v.size() ? " -> " : "");
        }
    } catch (...) {
        err = "Error al escribir " + full.string();
        return false;
    }
    if (!fs::exists(full, ec)) { err = "No se pudo crear " + full.string(); return false; }
    a.savedFor[i] = full.string();
    return true;
}

inline void DrawSaveScreen(App& a) {
    const Palette& p = P();
    ClearBackground(p.night);
    Header("descargar archivos  |  " + Fit(a.loadedName, 300, 20));
    if (IsKeyPressed(KEY_ESCAPE)) a.screen = Screen::Main;
    if (IsKeyPressed(KEY_TAB)) a.focus = a.focus == 2 ? 3 : 2;

    Rectangle dp{16, 60, 1088, 170};
    Section(dp, "DESTINO");
    Txt("Carpeta de destino", 32, 98, 20, p.gray);
    TextBox(Rectangle{32, 122, 520, 44}, a.outDir, 2, a.focus, 120);
    Txt("Prefijo del nombre (opcional)", 576, 98, 20, p.gray);
    TextBox(Rectangle{576, 122, 512, 44}, a.prefix, 3, a.focus, 60);
    Txt("La carpeta se crea si no existe. Vacia = junto al .exe. Tab cambia de cuadro.", 32, 178, 20, p.gray);
    Txt("Ejemplo de prefijo: laberinto1_", 32, 202, 10, p.gray);

    Rectangle fp{16, 246, 1088, 292};
    Section(fp, "ARCHIVOS");
    bool saveAll = false;
    for (int i = 0; i < 5; i++) {
        Rectangle rr{32, fp.y + 44.f + i * 48.f, 1056, 42};
        bool acc = i >= M_BFS_ROUTE;
        Panel(rr, p.panel, Hex(0x3b4547));
        Rectangle nb{rr.x + 4, rr.y + 4, 34, rr.height - 8};
        DrawRectangleRec(nb, acc ? p.bfs : p.dfs);
        TxtC(std::to_string(i + 1), nb, 20, Hex(0x14100a));
        Txt(kLabel[i], rr.x + 52, rr.y + 11, 20, p.sand);
        Txt(Fit(a.prefix + kFile[i], 420, 20), rr.x + 260, rr.y + 11, 20, p.gray);
        char cnt[40];
        std::snprintf(cnt, sizeof cnt, "%d coord.", (int)OutData(a, i).size());
        Txt(cnt, rr.x + 700, rr.y + 11, 20, p.gray);
        bool done = !a.savedFor[i].empty() && a.savedFor[i] == OutPath(a, i).string();
        if (done) Txt("OK", rr.x + 830, rr.y + 11, 20, p.green);
        if (Button(Rectangle{rr.x + rr.width - 150, rr.y + 2, 142, 38}, "GUARDAR", 20, Hex(0x3b4547), Hex(0x566362), p.sand)) {
            std::string err;
            if (SaveOne(a, i, err)) {
                std::error_code ec;
                a.saveMsg = "Guardado: " + std::filesystem::absolute(OutPath(a, i), ec).string();
                a.saveErr = false;
            } else {
                a.saveMsg = err;
                a.saveErr = true;
            }
        }
    }

    if (Button(Rectangle{16, 552, 340, 48}, "GUARDAR LOS 5 ARCHIVOS", 20, p.amber, p.amberHi, Hex(0x1b1406))) saveAll = true;
    if (Button(Rectangle{368, 552, 160, 48}, "VOLVER", 20, Hex(0x3b4547), Hex(0x566362), p.sand)) a.screen = Screen::Main;
    if (saveAll) {
        std::string err;
        int ok = 0;
        for (int i = 0; i < 5; i++)
            if (SaveOne(a, i, err)) ok++;
            else break;
        if (ok == 5) {
            std::error_code ec;
            a.saveMsg = "Guardados 5 archivos en: " + std::filesystem::absolute(OutPath(a, 0).parent_path(), ec).string();
            a.saveErr = false;
        } else {
            a.saveMsg = err;
            a.saveErr = true;
        }
    }

    Rectangle con{16, 608, 1088, 40};
    DrawRectangleRec(con, p.console);
    DrawRectangleLinesEx(con, 4, Hex(0x2f3638));
    if (a.saveMsg.empty())
        Txt("> Elige la carpeta y pulsa GUARDAR en cada archivo, o guarda los 5 de una vez", 28, 618, 20, p.green);
    else
    {
        // "Etiqueta: ruta" -> recorta solo la ruta para que siempre se lea la etiqueta
        Color mc = a.saveErr ? Hex(0xff7a5c) : p.green;
        size_t pos = a.saveMsg.find(": ");
        std::string label = "> " + (pos == std::string::npos ? a.saveMsg : a.saveMsg.substr(0, pos + 2));
        std::string rest = pos == std::string::npos ? "" : a.saveMsg.substr(pos + 2);
        Txt(label, 28, 618, 20, mc);
        Txt(Fit(rest, 1050 - Wid(label, 20), 20), 28 + Wid(label, 20), 618, 20, mc);
    }
}

// ---------------------------------------------------------------------------
//  Bucle principal
// ---------------------------------------------------------------------------
inline void Run(MazeSolver solver, FileSaver saver) {
    App app;
    app.solver = solver;
    app.saver = saver;

    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(WIN_W, WIN_H, "Runner.exe");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    g_font = GetFontDefault();
    g_custom = false;
    if (FileExists("PressStart2P-Regular.ttf")) {
        Font f = LoadFontEx("PressStart2P-Regular.ttf", 32, nullptr, 0);
        if (f.texture.id != 0) {
            SetTextureFilter(f.texture, TEXTURE_FILTER_POINT);
            g_font = f;
            g_custom = true;
        }
    }
    ScanFiles(app);

    while (!WindowShouldClose()) {
        g_claimed = false;
        BeginDrawing();
        switch (app.screen) {
            case Screen::Title: DrawTitleScreen(app); break;
            case Screen::Load: DrawLoadScreen(app); break;
            case Screen::Main: DrawMainScreen(app); break;
            case Screen::Anim: DrawAnimScreen(app); break;
            case Screen::Save: DrawSaveScreen(app); break;
        }
        EndDrawing();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !g_claimed) app.focus = 0;
    }
    if (g_custom) UnloadFont(g_font);
    CloseWindow();
}

}  // namespace maze_ui_detail

// ============================================================================
//  UNICA FUNCION PUBLICA
// ============================================================================
inline void run_maze_ui(MazeSolver solver, FileSaver saver = nullptr) {
    maze_ui_detail::Run(solver, saver);
}