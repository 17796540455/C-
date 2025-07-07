#define _CRT_SECURE_NO_WARNINGS
#include <graphics.h>
#include <conio.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <vector>
using namespace std;

#define BOARD_LINE   9
#define CELL_SIZE    50
#define RADIUS       15
#define MARGIN       CELL_SIZE
#define WIN_WIDTH    800
#define WIN_HEIGHT   600

// 搜索参数
const int SEARCH_DEPTH = 4;      // 搜索深度，可调
const int CAND_RANGE = 2;      // 候选落子半径

int chessMap[BOARD_LINE][BOARD_LINE];
int flag, gameOver, stepCount;
int blackWins, whiteWins;
int aiEnabled, aiSide;
int inGame;       // 0=菜单 1=游戏

IMAGE background;
struct Step { int x, y, player; };
Step steps[BOARD_LINE * BOARD_LINE];

struct Button { int x, y, w, h; COLORREF color; const char* text; };
Button undoBtn = { 650,300,120,40,RGB(200,200,255),"悔棋" };
Button menuBtn = { 650,360,120,40,RGB(255,200,200),"返回菜单" };

//-----------------------------------------------------------------------------
// 图形与按钮
//-----------------------------------------------------------------------------
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
    return mx >= btn.x && mx <= btn.x + btn.w && my >= btn.y && my <= btn.y + btn.h;
}

//-----------------------------------------------------------------------------
// 候选着法生成 & 评估
//-----------------------------------------------------------------------------
vector<pair<int, int>> generateMoves() {
    static bool used[BOARD_LINE][BOARD_LINE];
    memset(used, 0, sizeof(used));
    vector<pair<int, int>> moves;
    for (int x = 0;x < BOARD_LINE;x++) {
        for (int y = 0;y < BOARD_LINE;y++) {
            if (chessMap[x][y] != 0) {
                for (int dx = -CAND_RANGE;dx <= CAND_RANGE;dx++) {
                    for (int dy = -CAND_RANGE;dy <= CAND_RANGE;dy++) {
                        int nx = x + dx, ny = y + dy;
                        if (nx >= 0 && nx < BOARD_LINE && ny >= 0 && ny < BOARD_LINE
                            && chessMap[nx][ny] == 0 && !used[nx][ny]) {
                            used[nx][ny] = 1;
                            moves.emplace_back(nx, ny);
                        }
                    }
                }
            }
        }
    }
    if (moves.empty()) {
        moves.emplace_back(BOARD_LINE / 2, BOARD_LINE / 2);
    }
    return moves;
}

int scoreLine(int x, int y, int dx, int dy, int player) {
    int count = 0, blocks = 0;
    for (int s = -4;s <= 4;s++) {
        int nx = x + s * dx, ny = y + s * dy;
        if (nx < 0 || ny < 0 || nx >= BOARD_LINE || ny >= BOARD_LINE) continue;
        if (chessMap[nx][ny] == player) count++;
        else if (chessMap[nx][ny] != 0) blocks++;
    }
    if (count >= 5)      return 100000;
    if (count == 4 && blocks == 0) return 10000;
    if (count == 3 && blocks == 0) return 1000;
    if (count == 2 && blocks == 0) return 100;
    if (count == 1 && blocks == 0) return 10;
    return 0;
}

int evaluateBoard() {
    int total = 0;
    int dirs[4][2] = { {1,0},{0,1},{1,1},{1,-1} };
    for (int x = 0;x < BOARD_LINE;x++) {
        for (int y = 0;y < BOARD_LINE;y++) {
            int p = chessMap[x][y];
            if (p == 0) continue;
            int sum = 0;
            for (auto& d : dirs) {
                sum += scoreLine(x, y, d[0], d[1], p);
            }
            total += (p == aiSide ? +sum : -sum);
        }
    }
    return total;
}

//-----------------------------------------------------------------------------
// Minimax + α–β 剪枝
//-----------------------------------------------------------------------------
int alphaBeta(int depth, int alpha, int beta, bool isMax) {
    if (depth == 0) return evaluateBoard();
    auto moves = generateMoves();
    if (isMax) {
        int val = INT_MIN;
        for (auto& m : moves) {
            chessMap[m.first][m.second] = aiSide;
            int score = alphaBeta(depth - 1, alpha, beta, false);
            chessMap[m.first][m.second] = 0;
            val = max(val, score);
            alpha = max(alpha, val);
            if (alpha >= beta) break;
        }
        return val;
    }
    else {
        int opp = (aiSide == 1 ? 2 : 1);
        int val = INT_MAX;
        for (auto& m : moves) {
            chessMap[m.first][m.second] = opp;
            int score = alphaBeta(depth - 1, alpha, beta, true);
            chessMap[m.first][m.second] = 0;
            val = min(val, score);
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
        int sc = alphaBeta(SEARCH_DEPTH - 1, INT_MIN, INT_MAX, false);
        chessMap[m.first][m.second] = 0;
        if (sc > bestScore) {
            bestScore = sc;
            best = m;
        }
    }
    int bx = best.first, by = best.second;
    chessMap[bx][by] = aiSide;
    steps[stepCount++] = { bx,by,aiSide };
}

//-----------------------------------------------------------------------------
// 胜负判定 & 绘制
//-----------------------------------------------------------------------------
bool judge(int x, int y) {
    int p = chessMap[x][y];
    int dirs[4][2] = { {1,0},{0,1},{1,1},{1,-1} };
    for (int d = 0;d < 4;d++) {
        int cnt = 1;
        for (int s = 1;s < 5;s++) {
            int nx = x + s * dirs[d][0], ny = y + s * dirs[d][1];
            if (nx < 0 || ny < 0 || nx >= BOARD_LINE || ny >= BOARD_LINE || chessMap[nx][ny] != p) break;
            cnt++;
        }
        for (int s = 1;s < 5;s++) {
            int nx = x - s * dirs[d][0], ny = y - s * dirs[d][1];
            if (nx < 0 || ny < 0 || nx >= BOARD_LINE || ny >= BOARD_LINE || chessMap[nx][ny] != p) break;
            cnt++;
        }
        if (cnt >= 5) return true;
    }
    return false;
}

void drawBoard() {
    putimage(0, 0, &background);
    setlinecolor(BLACK);
    for (int i = 0;i < BOARD_LINE;i++) {
        int pos = MARGIN + i * CELL_SIZE;
        line(MARGIN, pos, MARGIN + (BOARD_LINE - 1) * CELL_SIZE, pos);
        line(pos, MARGIN, pos, MARGIN + (BOARD_LINE - 1) * CELL_SIZE);
    }
    setfillcolor(BLACK);
    solidcircle(MARGIN + 4 * CELL_SIZE, MARGIN + 4 * CELL_SIZE, 4);

    int px = MARGIN + BOARD_LINE * CELL_SIZE + 20;
    settextstyle(20, 0, "宋体");
    settextcolor(BLACK);
    outtextxy(px, 40, "五子棋");
    if (aiEnabled) {
        outtextxy(px, 80, aiSide == 1 ? "AI: 黑棋" : "AI: 白棋");
        outtextxy(px, 110, aiSide == 1 ? "玩家: 白棋" : "玩家: 黑棋");
    }
    else {
        outtextxy(px, 80, "玩家1: 黑棋");
        outtextxy(px, 110, "玩家2: 黑棋");
    }
    char buf[64];
    sprintf(buf, "本轮统计：黑 %d 胜  白 %d 胜", blackWins, whiteWins);
    outtextxy(px, 140, buf);

    if (inGame) {
        drawButton(undoBtn);
        drawButton(menuBtn);
    }
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

//-----------------------------------------------------------------------------
// 流程控制
//-----------------------------------------------------------------------------
int showAISideMenu() {
    inGame = 0;
    initgraph(WIN_WIDTH, WIN_HEIGHT);
    loadBackgroundImage();
    BeginBatchDraw();
    Button b[2] = { {300,250,200,50,RGB(200,200,255),"AI执黑(先)"},
                   {300,320,200,50,RGB(255,200,200),"AI执白(后)"} };
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

int showMenu() {
    inGame = 0;
    blackWins = whiteWins = 0;
    initgraph(WIN_WIDTH, WIN_HEIGHT);
    loadBackgroundImage();
    BeginBatchDraw();
    Button b[3] = {
        {300,200,200,50,RGB(180,255,180),"双人游戏"},
        {300,270,200,50,RGB(200,255,255),"人机对战"},
        {300,340,200,50,RGB(255,180,180),"退出游戏"}
    };
    MOUSEMSG m;
    while (true) {
        putimage(0, 0, &background);
        settextstyle(50, 0, "宋体");
        outtextxy(230, 100, "欢迎来到五子棋");
        for (int i = 0;i < 3;i++) drawButton(b[i]);
        FlushBatchDraw();
        if (MouseHit()) {
            m = GetMouseMsg();
            if (m.uMsg == WM_LBUTTONDOWN) {
                if (isInside(b[0], m.x, m.y)) { EndBatchDraw(); closegraph(); aiEnabled = 0; return 1; }
                if (isInside(b[1], m.x, m.y)) { EndBatchDraw(); closegraph(); aiEnabled = 1; aiSide = showAISideMenu(); return 1; }
                if (isInside(b[2], m.x, m.y)) { EndBatchDraw(); closegraph(); exit(0); }
            }
        }
        Sleep(10);
    }
}

void playGame() {
    inGame = 1;
    initgraph(WIN_WIDTH, WIN_HEIGHT);
    loadBackgroundImage();

    BeginBatchDraw();
    memset(chessMap, 0, sizeof(chessMap));
    stepCount = flag = gameOver = 0;

    drawBoard(); drawAll(); FlushBatchDraw();

    // AI 先手
    if (aiEnabled && aiSide == 1) {
        aiMove();
        flag++;  // ← 修复点：让 flag 从 0 变为 1，进入玩家（白棋）回合
        drawBoard(); drawAll(); FlushBatchDraw();
    }

    MOUSEMSG m;
    while (true) {
        if (MouseHit()) {
            m = GetMouseMsg();
            if (m.uMsg == WM_LBUTTONDOWN) {
                if (isInside(menuBtn, m.x, m.y)) {
                    EndBatchDraw(); closegraph(); return;
                }
                if (isInside(undoBtn, m.x, m.y) && stepCount > 0) {
                    int remove = (aiEnabled ? 2 : 1);
                    for (int i = 0;i < remove && stepCount>0;i++) {
                        chessMap[steps[--stepCount].x][steps[stepCount].y] = 0;
                        flag--;
                    }
                    drawBoard(); drawAll(); FlushBatchDraw();
                    continue;
                }
                if (gameOver) continue;

                int mapx = (m.x - MARGIN + CELL_SIZE / 2) / CELL_SIZE;
                int mapy = (m.y - MARGIN + CELL_SIZE / 2) / CELL_SIZE;
                if (mapx < 0 || mapx >= BOARD_LINE || mapy < 0 || mapy >= BOARD_LINE) continue;
                if (chessMap[mapx][mapy] != 0) continue;
                int player = (flag % 2 == 0 ? 1 : 2);
                if (aiEnabled && player == aiSide) continue;

                // 玩家落子
                chessMap[mapx][mapy] = player;
                steps[stepCount++] = { mapx,mapy,player };
                drawBoard(); drawAll(); FlushBatchDraw();

                // 玩家胜利判断
                if (judge(mapx, mapy)) {
                    gameOver = 1;
                    if (player == 1) blackWins++; else whiteWins++;
                    if (MessageBox(GetHWnd(),
                        player == 1 ? "黑棋获胜!再来一局?" : "白棋获胜!再来一局?",
                        "游戏结束", MB_YESNO) == IDYES)
                    {
                        EndBatchDraw(); closegraph(); playGame(); return;
                    }
                    else {
                        EndBatchDraw(); closegraph(); return;
                    }
                }
                flag++;

                // AI 回合
                if (aiEnabled && !gameOver) {
                    aiMove();
                    drawBoard(); drawAll(); FlushBatchDraw();

                    // AI 胜利判断
                    if (judge(steps[stepCount - 1].x, steps[stepCount - 1].y)) {
                        gameOver = 1;
                        if (aiSide == 1) blackWins++; else whiteWins++;
                        if (MessageBox(GetHWnd(),
                            aiSide == 1 ? "AI(黑)获胜!再来一局?" : "AI(白)获胜!再来一局?",
                            "游戏结束", MB_YESNO) == IDYES)
                        {
                            EndBatchDraw(); closegraph(); playGame(); return;
                        }
                        else {
                            EndBatchDraw(); closegraph(); return;
                        }
                    }
                    flag++;
                }
            }
        }
        Sleep(10);
    }
}


int main() {
    srand((unsigned)time(NULL));
    while (true) {
        int choice = showMenu();
        if (choice == 1) playGame();
        else break;
    }
    closegraph();
    return 0;
}
