#define _CRT_SECURE_NO_WARNINGS
#include <graphics.h>
#include <conio.h>
#include <windows.h>
#include <thread>
#include <atomic>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <vector>
using namespace std;

#define CELL_SIZE    50
#define RADIUS       15
#define MARGIN       CELL_SIZE
#define WIN_WIDTH    800
#define WIN_HEIGHT   600

const int DEFAULT_BOARD_LINE = 9;
const int DEFAULT_SEARCH_DEPTH = 4;
const int DEFAULT_CAND_RANGE = 2;

struct GameSettings {
    int boardLine;
    int searchDepth;
    int candRange;
    bool enableUndo;
};
GameSettings settings = {
    DEFAULT_BOARD_LINE,
    DEFAULT_SEARCH_DEPTH,
    DEFAULT_CAND_RANGE,
    true
};

int chessMap[20][20];
int flag, gameOver, stepCount;
int blackWins, whiteWins;
bool aiEnabled;
int aiSide;

IMAGE background;
struct Step { int x, y, player; };
Step steps[400];

struct Button { int x, y, w, h; COLORREF color; const char* text; };
Button undoBtn = { 650,300,120,40,RGB(200,200,255),"悔棋" };
Button menuBtn = { 650,360,120,40,RGB(255,200,200),"返回菜单" };
Button settingsBtn = { 300,340,200,50,RGB(200,255,200),"游戏设置" };
Button incDepthBtn = { 450,175, 80,40,RGB(220,220,255),"+ 深度" };
Button decDepthBtn = { 550,175, 80,40,RGB(220,220,255),"- 深度" };
Button incRangeBtn = { 450,235, 80,40,RGB(220,220,255),"+ 半径" };
Button decRangeBtn = { 550,235, 80,40,RGB(220,220,255),"- 半径" };
Button toggleUndoBtn = { 450,295,180,40,RGB(255,220,220),"切换 悔棋" };
Button backBtn = { 300,400,200,50,RGB(200,200,200),"返回菜单" };

static atomic<bool> aiThinking(false);
static thread    aiThread;

void loadBackgroundImage() {
    loadimage(&background, "背景.jpg", WIN_WIDTH, WIN_HEIGHT);
}

void drawButton(const Button& btn) {
    setlinecolor(BLACK);
    setfillcolor(btn.color);
    solidrectangle(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h);
    settextcolor(BLACK);
    setbkmode(TRANSPARENT);
    settextstyle(25, 0, "宋体");
    int tx = btn.x + (btn.w - textwidth(btn.text)) / 2;
    int ty = btn.y + (btn.h - textheight(btn.text)) / 2;
    outtextxy(tx, ty, btn.text);
}

bool isInside(const Button& btn, int mx, int my) {
    return mx >= btn.x && mx <= btn.x + btn.w
        && my >= btn.y && my <= btn.y + btn.h;
}

vector<pair<int, int>> generateMoves() {
    static bool used[20][20];
    memset(used, 0, sizeof(used));
    vector<pair<int, int>> moves;
    int n = settings.boardLine;
    for (int x = 0;x < n;x++)for (int y = 0;y < n;y++) {
        if (chessMap[x][y] != 0) {
            for (int dx = -settings.candRange;dx <= settings.candRange;dx++)
                for (int dy = -settings.candRange;dy <= settings.candRange;dy++) {
                    int nx = x + dx, ny = y + dy;
                    if (nx >= 0 && ny >= 0 && nx < n && ny < n
                        && chessMap[nx][ny] == 0 && !used[nx][ny]) {
                        used[nx][ny] = true;
                        moves.emplace_back(nx, ny);
                    }
                }
        }
    }
    if (moves.empty())
        moves.emplace_back(n / 2, n / 2);
    return moves;
}

int scoreLine(int x, int y, int dx, int dy, int player) {
    int count = 0, blocks = 0, n = settings.boardLine;
    for (int s = -4;s <= 4;s++) {
        int nx = x + s * dx, ny = y + s * dy;
        if (nx < 0 || ny < 0 || nx >= n || ny >= n) continue;
        if (chessMap[nx][ny] == player) count++;
        else if (chessMap[nx][ny] != 0) blocks++;
    }
    if (count >= 5) return 100000;
    if (count == 4 && blocks == 0) return 10000;
    if (count == 3 && blocks == 0) return 1000;
    if (count == 2 && blocks == 0) return 100;
    if (count == 1 && blocks == 0) return 10;
    return 0;
}

int evaluateBoard() {
    int total = 0;
    int dirs[4][2] = { {1,0},{0,1},{1,1},{1,-1} };
    for (int x = 0;x < settings.boardLine;x++)
        for (int y = 0;y < settings.boardLine;y++) {
            int p = chessMap[x][y]; if (p == 0) continue;
            int sum = 0;
            for (auto& d : dirs)
                sum += scoreLine(x, y, d[0], d[1], p);
            total += (p == aiSide ? sum : -sum);
        }
    return total;
}

int alphaBeta(int depth, int alpha, int beta, bool isMax) {
    if (depth == 0) return evaluateBoard();
    auto moves = generateMoves();
    if (isMax) {
        int val = INT_MIN;
        for (auto& m : moves) {
            chessMap[m.first][m.second] = aiSide;
            int sc = alphaBeta(depth - 1, alpha, beta, false);
            chessMap[m.first][m.second] = 0;
            val = max(val, sc);
            alpha = max(alpha, val);
            if (alpha >= beta) break;
        }
        return val;
    }
    else {
        int opp = aiSide == 1 ? 2 : 1, val = INT_MAX;
        for (auto& m : moves) {
            chessMap[m.first][m.second] = opp;
            int sc = alphaBeta(depth - 1, alpha, beta, true);
            chessMap[m.first][m.second] = 0;
            val = min(val, sc);
            beta = min(beta, val);
            if (beta <= alpha) break;
        }
        return val;
    }
}

void aiMove() {
    auto moves = generateMoves();
    int bestScore = INT_MIN;
    pair<int, int> best = moves[0];
    for (auto& m : moves) {
        chessMap[m.first][m.second] = aiSide;
        int sc = alphaBeta(settings.searchDepth - 1, INT_MIN, INT_MAX, false);
        chessMap[m.first][m.second] = 0;
        if (sc > bestScore) {
            bestScore = sc; best = m;
        }
    }
    chessMap[best.first][best.second] = aiSide;
    steps[stepCount++] = { best.first, best.second, aiSide };
}

bool judge(int x, int y) {
    int p = chessMap[x][y];
    int dirs[4][2] = { {1,0},{0,1},{1,1},{1,-1} };
    for (int d = 0;d < 4;d++) {
        int cnt = 1;
        for (int s = 1;s < 5;s++) {
            int nx = x + s * dirs[d][0], ny = y + s * dirs[d][1];
            if (nx < 0 || ny < 0 || nx >= settings.boardLine || ny >= settings.boardLine || chessMap[nx][ny] != p) break;
            cnt++;
        }
        for (int s = 1;s < 5;s++) {
            int nx = x - s * dirs[d][0], ny = y - s * dirs[d][1];
            if (nx < 0 || ny < 0 || nx >= settings.boardLine || ny >= settings.boardLine || chessMap[nx][ny] != p) break;
            cnt++;
        }
        if (cnt >= 5) return true;
    }
    return false;
}

void drawAll() {
    for (int i = 0;i < stepCount;i++) {
        int cx = MARGIN + steps[i].x * CELL_SIZE;
        int cy = MARGIN + steps[i].y * CELL_SIZE;
        setfillcolor(steps[i].player == 1 ? BLACK : WHITE);
        setlinecolor(BLACK);
        solidcircle(cx, cy, RADIUS);
    }
}

void drawBoard() {
    putimage(0, 0, &background);
    // 棋盘线
    setlinecolor(BLACK);
    for (int i = 0;i < settings.boardLine;i++) {
        int pos = MARGIN + i * CELL_SIZE;
        line(MARGIN, pos, MARGIN + (settings.boardLine - 1) * CELL_SIZE, pos);
        line(pos, MARGIN, pos, MARGIN + (settings.boardLine - 1) * CELL_SIZE);
    }
    // 中心星
    solidcircle(MARGIN + (settings.boardLine / 2) * CELL_SIZE,
        MARGIN + (settings.boardLine / 2) * CELL_SIZE, 4);

    // 右侧 UI
    int px = MARGIN + settings.boardLine * CELL_SIZE + 20;
    settextstyle(20, 0, "宋体");
    settextcolor(BLACK);
    outtextxy(px, 40, "五子棋");
    if (aiEnabled) {
        outtextxy(px, 80, aiSide == 1 ? "AI: 黑棋" : "AI: 白棋");
        outtextxy(px, 110, aiSide == 1 ? "玩家: 白棋" : "玩家: 黑棋");
        if (aiThinking) {
            int dots = (GetTickCount() / 500) % 3 + 1;
            char buf[32];
            sprintf(buf, "AI 思考中%.*s", dots, "...");
            outtextxy(px, 140, buf);
        }
    }
    else {
        outtextxy(px, 80, "玩家1: 黑棋");
        outtextxy(px, 110, "玩家2: 白棋");
    }
    char statBuf[64];
    sprintf(statBuf, "统计：黑 %d 胜  白 %d 胜", blackWins, whiteWins);
    outtextxy(px, 180, statBuf);

    if (flag >= 0) {
        if (settings.enableUndo) drawButton(undoBtn);
        drawButton(menuBtn);
    }
}

// AI线程
void startAIAsync() {
    if (aiThinking) return;
    aiThinking = true;
    aiThread = thread([]() {
        aiMove();
        aiThinking = false;
        });
    aiThread.detach();
}

// 主流程
void playGame() {
    initgraph(WIN_WIDTH, WIN_HEIGHT);
    loadBackgroundImage();
    BeginBatchDraw();
    memset(chessMap, 0, sizeof(chessMap));
    stepCount = flag = gameOver = 0;
    drawBoard(); drawAll(); FlushBatchDraw();

    if (aiEnabled && aiSide == 1) startAIAsync();

    MOUSEMSG m;
    while (true) {
        drawBoard(); drawAll(); FlushBatchDraw();
        if (aiEnabled && !gameOver
            && (flag % 2 == (aiSide - 1))
            && !aiThinking)
        {
            flag++;
            auto& s = steps[stepCount - 1];
            if (judge(s.x, s.y)) {
                gameOver = 1;
                if (aiSide == 1) blackWins++; else whiteWins++;
                int ret = MessageBox(GetHWnd(),
                    aiSide == 1 ? "AI(黑)获胜!再来一局?" : "AI(白)获胜!再来一局?",
                    "游戏结束", MB_YESNO);
                EndBatchDraw(); closegraph();
                if (ret == IDYES) { playGame(); return; }
                else return;
            }
        }

        if (MouseHit()) {
            m = GetMouseMsg();
            if (m.uMsg == WM_LBUTTONDOWN) {
                if (isInside(menuBtn, m.x, m.y)) {
                    EndBatchDraw(); closegraph(); return;
                }
                if (settings.enableUndo && isInside(undoBtn, m.x, m.y) && stepCount > 0) {
                    int cnt = aiEnabled ? 2 : 1;
                    while (cnt-- > 0 && stepCount > 0) {
                        auto& s = steps[--stepCount];
                        chessMap[s.x][s.y] = 0; flag--;
                    }
                    drawBoard(); drawAll(); FlushBatchDraw();
                    continue;
                }
                if (!gameOver && !aiThinking) {
                    int mapx = (m.x - MARGIN + CELL_SIZE / 2) / CELL_SIZE;
                    int mapy = (m.y - MARGIN + CELL_SIZE / 2) / CELL_SIZE;
                    if (mapx >= 0 && mapx < settings.boardLine
                        && mapy >= 0 && mapy < settings.boardLine
                        && chessMap[mapx][mapy] == 0) {
                        int player = (flag % 2 == 0 ? 1 : 2);
                        if (!(aiEnabled && player == aiSide)) {
                            chessMap[mapx][mapy] = player;
                            steps[stepCount++] = { mapx,mapy,player };
                            drawBoard(); drawAll(); FlushBatchDraw();
                            if (judge(mapx, mapy)) {
                                gameOver = 1;
                                if (player == 1) blackWins++; else whiteWins++;
                                int ret = MessageBox(GetHWnd(),
                                    player == 1 ? "黑棋获胜!再来一局?" : "白棋获胜!再来一局?",
                                    "游戏结束", MB_YESNO);
                                EndBatchDraw(); closegraph();
                                if (ret == IDYES) { playGame(); return; }
                                else return;
                            }
                            flag++;
                            if (aiEnabled && !gameOver) {
                                startAIAsync();
                            }
                        }
                    }
                }
            }
        }
        Sleep(10);
    }
}

int showAISideMenu() {
    initgraph(WIN_WIDTH, WIN_HEIGHT);
    loadBackgroundImage();
    BeginBatchDraw();
    Button b[2] = {
        {300,250,200,50,RGB(200,200,255),"AI执黑(先)"},
        {300,320,200,50,RGB(255,200,200),"AI执白(后)"}
    };
    MOUSEMSG m;
    while (true) {
        putimage(0, 0, &background);
        settextstyle(40, 0, "宋体");
        outtextxy(250, 150, "请选择 AI 执子");
        drawButton(b[0]); drawButton(b[1]);
        FlushBatchDraw();
        if (MouseHit()) {
            m = GetMouseMsg();
            if (m.uMsg == WM_LBUTTONDOWN) {
                if (isInside(b[0], m.x, m.y)) { EndBatchDraw(); closegraph(); return 1; }
                if (isInside(b[1], m.x, m.y)) { EndBatchDraw(); closegraph(); return 2; }
            }
        }
        Sleep(10);
    }
}

void showSettingsMenu() {
    initgraph(WIN_WIDTH, WIN_HEIGHT);
    loadBackgroundImage();
    BeginBatchDraw();
    MOUSEMSG m;
    while (true) {
        putimage(0, 0, &background);
        settextstyle(30, 0, "宋体");
        settextcolor(BLACK);
        // 深度
        outtextxy(250, 180, "搜索深度:");
        char buf[32];
        sprintf(buf, "%d", settings.searchDepth);
        outtextxy(390, 180, buf);
        drawButton(incDepthBtn); drawButton(decDepthBtn);
        // 半径
        outtextxy(250, 240, "候选半径:");
        sprintf(buf, "%d", settings.candRange);
        outtextxy(390, 240, buf);
        drawButton(incRangeBtn); drawButton(decRangeBtn);
        // 悔棋
        outtextxy(250, 300, "悔棋:");
        outtextxy(350, 300, settings.enableUndo ? "启用" : "禁用");
        drawButton(toggleUndoBtn);
        // 返回
        drawButton(backBtn);
        FlushBatchDraw();
        if (MouseHit()) {
            m = GetMouseMsg();
            if (m.uMsg == WM_LBUTTONDOWN) {
                if (isInside(incDepthBtn, m.x, m.y) && settings.searchDepth < 10) settings.searchDepth++;
                else if (isInside(decDepthBtn, m.x, m.y) && settings.searchDepth > 1) settings.searchDepth--;
                else if (isInside(incRangeBtn, m.x, m.y) && settings.candRange < 5) settings.candRange++;
                else if (isInside(decRangeBtn, m.x, m.y) && settings.candRange > 0) settings.candRange--;
                else if (isInside(toggleUndoBtn, m.x, m.y)) settings.enableUndo = !settings.enableUndo;
                else if (isInside(backBtn, m.x, m.y)) { EndBatchDraw(); closegraph(); return; }
            }
        }
        Sleep(10);
    }
}

int showMenu() {
    aiEnabled = false;
    blackWins = whiteWins = 0;
    initgraph(WIN_WIDTH, WIN_HEIGHT);
    loadBackgroundImage();
    BeginBatchDraw();
    Button b[4] = {
        {300,200,200,50,RGB(180,255,180),"双人游戏"},
        {300,270,200,50,RGB(200,255,255),"人机对战"},
        settingsBtn,
        {300,410,200,50,RGB(255,180,180),"退出游戏"}
    };
    MOUSEMSG m;
    while (true) {
        putimage(0, 0, &background);
        settextstyle(50, 0, "宋体");
        outtextxy(230, 100, "欢迎来到五子棋");
        for (int i = 0;i < 4;i++) drawButton(b[i]);
        FlushBatchDraw();
        if (MouseHit()) {
            m = GetMouseMsg();
            if (m.uMsg == WM_LBUTTONDOWN) {
                if (isInside(b[0], m.x, m.y)) { EndBatchDraw(); closegraph(); aiEnabled = false; return 1; }
                if (isInside(b[1], m.x, m.y)) { EndBatchDraw(); closegraph(); aiEnabled = true; aiSide = showAISideMenu(); return 1; }
                if (isInside(b[2], m.x, m.y)) { EndBatchDraw(); closegraph(); showSettingsMenu(); return showMenu(); }
                if (isInside(b[3], m.x, m.y)) { EndBatchDraw(); closegraph(); exit(0); }
            }
        }
        Sleep(10);
    }
}

int main() {
    srand((unsigned)time(NULL));
    while (true) {
        if (showMenu() == 1)
            playGame();
        else
            break;
    }
    closegraph();
    return 0;
}
