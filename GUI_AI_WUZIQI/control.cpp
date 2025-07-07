#define _CRT_SECURE_NO_WARNINGS
#include <graphics.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>

#define BOARD_LINE 19
#define CELL_SIZE 30
#define RADIUS 10
#define MARGIN CELL_SIZE

int chessMap[BOARD_LINE][BOARD_LINE] = { 0 };
int flag = 0;
int gameOver = 0;
int blackWins = 0;
int whiteWins = 0;

IMAGE background;
bool hasBackground = false;

typedef struct {
    int x, y, player;
} Step;

Step steps[BOARD_LINE * BOARD_LINE];
int stepCount = 0;

typedef struct {
    int x, y, w, h;
    COLORREF color;
    char text[32];
} Button;

Button undoBtn = { 650, 300, 120, 40, RGB(200, 200, 255), "悔棋" };
Button menuBtn = { 650, 360, 120, 40, RGB(255, 200, 200), "返回菜单" };

void drawBoard();
void drawAllChess();
void undoStep();
int judge(int x, int y);
void restartGame();
void playGame();
void initGame();
int showMenu();
void drawButton(Button btn, bool hover);
bool isInsideButton(Button btn, int mx, int my);

void drawBoard() {
    if (hasBackground) {
        putimage(0, 0, &background);
    }
    else {
        setbkcolor(WHITE);
        cleardevice();
    }
    setlinecolor(BLACK);
    setlinestyle(PS_SOLID, 1);
    for (int i = 0; i < BOARD_LINE; i++) {
        int x = MARGIN + i * CELL_SIZE;
        line(MARGIN, x, MARGIN + CELL_SIZE * (BOARD_LINE - 1), x);
        line(x, MARGIN, x, MARGIN + CELL_SIZE * (BOARD_LINE - 1));
    }
    setfillcolor(BLACK);
    int stars[7][2] = { {3,3},{3,9},{3,15},{9,9},{15,3},{15,9},{15,15} };
    for (int i = 0; i < 7; i++) {
        int x = MARGIN + stars[i][0] * CELL_SIZE;
        int y = MARGIN + stars[i][1] * CELL_SIZE;
        solidcircle(x, y, 3);
    }
    settextstyle(20, 0, "宋体");
    settextcolor(BLACK);
    setbkmode(TRANSPARENT);
    outtextxy(MARGIN + CELL_SIZE * BOARD_LINE + 20, 40, "五子棋游戏");
    outtextxy(MARGIN + CELL_SIZE * BOARD_LINE + 20, 80, "玩家1: 黑棋");
    outtextxy(MARGIN + CELL_SIZE * BOARD_LINE + 20, 110, "玩家2: 白棋");
    char score[64];
    sprintf(score, "黑棋胜局: %d", blackWins);
    outtextxy(MARGIN + CELL_SIZE * BOARD_LINE + 20, 160, score);
    sprintf(score, "白棋胜局: %d", whiteWins);
    outtextxy(MARGIN + CELL_SIZE * BOARD_LINE + 20, 190, score);
    drawButton(undoBtn, false);
    drawButton(menuBtn, false);
}

void initGame() {
    if (!hasBackground && _access("背景.jpg", 0) == 0) {
        loadimage(&background, "背景.jpg", 800, 640);
        hasBackground = true;
    }
    drawBoard();
}

void drawAllChess() {
    for (int i = 0; i < stepCount; i++) {
        int cx = MARGIN + steps[i].x * CELL_SIZE;
        int cy = MARGIN + steps[i].y * CELL_SIZE;
        setfillcolor(steps[i].player == 1 ? BLACK : WHITE);
        solidcircle(cx, cy, RADIUS);
    }
}

void undoStep() {
    if (stepCount <= 0 || gameOver) return;
    stepCount--;
    int x = steps[stepCount].x;
    int y = steps[stepCount].y;
    chessMap[x][y] = 0;
    flag--;
    drawBoard();
    drawAllChess();
}

int judge(int x, int y) {
    int player = chessMap[x][y];
    if (player == 0) return 0;
    int dirs[4][2] = { {1,0},{0,1},{1,1},{1,-1} };
    for (int d = 0; d < 4; d++) {
        int count = 1;
        for (int i = 1; i < 5; i++) {
            int nx = x + dirs[d][0] * i;
            int ny = y + dirs[d][1] * i;
            if (nx < 0 || ny < 0 || nx >= BOARD_LINE || ny >= BOARD_LINE) break;
            if (chessMap[nx][ny] == player) count++; else break;
        }
        for (int i = 1; i < 5; i++) {
            int nx = x - dirs[d][0] * i;
            int ny = y - dirs[d][1] * i;
            if (nx < 0 || ny < 0 || nx >= BOARD_LINE || ny >= BOARD_LINE) break;
            if (chessMap[nx][ny] == player) count++; else break;
        }
        if (count >= 5) return 1;
    }
    return 0;
}

void restartGame() {
    memset(chessMap, 0, sizeof(chessMap));
    stepCount = 0;
    flag = 0;
    gameOver = 0;
    drawBoard();
}

void playGame() {
    MOUSEMSG msg = {0};
    BeginBatchDraw();
    while (1) {
        if (MouseHit()) {
            msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN) {
                if (isInsideButton(undoBtn, msg.x, msg.y)) {
                    undoStep();
                    continue;
                }
                else if (isInsideButton(menuBtn, msg.x, msg.y)) {
                    EndBatchDraw();
                    return;
                }
                if (gameOver) continue;
                int mx = msg.x, my = msg.y;
                int mapx = (mx - MARGIN + CELL_SIZE / 2) / CELL_SIZE;
                int mapy = (my - MARGIN + CELL_SIZE / 2) / CELL_SIZE;
                if (mapx < 0 || mapx >= BOARD_LINE || mapy < 0 || mapy >= BOARD_LINE) continue;
                if (chessMap[mapx][mapy] != 0) continue;
                int player = (flag % 2 == 0) ? 1 : 2;
                Step s;
                s.x = mapx;
                s.y = mapy;
                s.player = player;
                steps[stepCount++] = s;
                chessMap[mapx][mapy] = player;
                drawBoard();
                drawAllChess();
                if (judge(mapx, mapy)) {
                    gameOver = 1;
                    if (player == 1) blackWins++; else whiteWins++;
                    FlushBatchDraw();
                    char result[64];
                    sprintf(result, "玩家 %d 获胜! 是否再来一局?", player);
                    int ret = MessageBox(GetHWnd(), result, "游戏结束", MB_YESNO);
                    if (ret == IDYES) restartGame(); else { EndBatchDraw(); return; }
                }
                flag++;
            }
        }
        drawBoard();
        drawAllChess();
        drawButton(undoBtn, false);
        drawButton(menuBtn, false);
        FlushBatchDraw();
        Sleep(10);
    }
    EndBatchDraw();
}

void drawButton(Button btn, bool hover) {
    setlinecolor(BLACK);
    setfillcolor(hover ? RGB(200, 200, 255) : btn.color);
    solidrectangle(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h);
    settextcolor(BLACK);
    setbkmode(TRANSPARENT);
    settextstyle(25, 0, "宋体");
    int textX = btn.x + (btn.w - textwidth(btn.text)) / 2;
    int textY = btn.y + (btn.h - textheight(btn.text)) / 2;
    outtextxy(textX, textY, btn.text);
}

bool isInsideButton(Button btn, int mx, int my) {
    return mx >= btn.x && mx <= btn.x + btn.w && my >= btn.y && my <= btn.y + btn.h;
}

int showMenu() {
    initgraph(800, 640, SHOWCONSOLE);
    IMAGE bg;
    if (_access("背景.jpg", 0) == 0)
        loadimage(&bg, "背景.jpg", 800, 640);
    Button btns[3] = {
        {300, 220, 200, 50, RGB(180, 255, 180), "开始游戏"},
        {300, 300, 200, 50, RGB(255, 255, 180), "游戏说明"},
        {300, 380, 200, 50, RGB(255, 180, 180), "退出游戏"}
    };
    MOUSEMSG msg = {0};
    BeginBatchDraw();
    while (1) {
        putimage(0, 0, &bg);
        settextcolor(BLACK);
        settextstyle(50, 0, "宋体");
        outtextxy(230, 100, "欢迎来到五子棋");
        if (MouseHit()) msg = GetMouseMsg();
        for (int i = 0; i < 3; i++) {
            bool hover = isInsideButton(btns[i], msg.x, msg.y);
            drawButton(btns[i], hover);
        }
        FlushBatchDraw();
        if (msg.uMsg == WM_LBUTTONDOWN) {
            if (isInsideButton(btns[0], msg.x, msg.y)) {
                EndBatchDraw();
                return 1;
            }
            else if (isInsideButton(btns[1], msg.x, msg.y)) {
                cleardevice();
                settextstyle(25, 0, "宋体");
                outtextxy(100, 200, "1. 黑棋先手，轮流下子");
                outtextxy(100, 240, "2. 任意方向连成5子即胜");
                outtextxy(100, 280, "3. 点击悔棋按钮进行悔棋");
                outtextxy(100, 320, "4. 点击返回菜单按钮退出对局");
                outtextxy(100, 360, "点击任意处返回");
                FlushBatchDraw();
                while (!MouseHit()) Sleep(10);
                GetMouseMsg();
            }
            else if (isInsideButton(btns[2], msg.x, msg.y)) {
                EndBatchDraw();
                closegraph();
                exit(0);
            }
        }
        Sleep(10);
    }
}

int main() {
    while (1) {
        int choice = showMenu();
        if (choice == 1) {
            initGame();
            playGame();
        }
        else break;
    }
    closegraph();
    return 0;
}