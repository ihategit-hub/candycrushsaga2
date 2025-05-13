#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

#define ROWS 8
#define COLS 8
#define CANDY_TYPES 6
#define CELL_SIZE 64
#define BOARD_OFFSET_X 100
#define BOARD_OFFSET_Y 100
#define ANIMATION_SPEED 12.0f
#define DESTROY_ANIMATION_SPEED 0.15f

typedef enum
{
    CANDY_RED,
    CANDY_GREEN,
    CANDY_BLUE,
    CANDY_YELLOW,
    CANDY_PURPLE,
    CANDY_ORANGE
} CandyType;

typedef struct
{
    CandyType type;
    bool isMoving;
    float yOffset; // Animasyon için
    bool isMarkedToDestroy;
    float scale; // Patlama animasyonu için
} Candy;

typedef struct
{
    int row, col;
    bool selected;
} Cell;

Candy board[ROWS][COLS];
int score = 0;
Cell selectedCell = { -1, -1, false };
bool isSwapping = false;
Cell swapTarget = { -1, -1, false };
bool isAnimating = false;
bool isDestroying = false;
bool comboActive = false;
int comboMultiplier = 1;

// Renkler (yedek olarak saklanıyor)
Color candyColors[CANDY_TYPES];

// Doku/Görseller
Texture2D candyTextures[CANDY_TYPES];
bool useTextures = false; // Dokuların başarıyla yüklenip yüklenmediğini kontrol için

// Yardımcı fonksiyonlar
bool IsAdjacent(Cell a, Cell b)
{
    return (abs(a.row - b.row) == 1 && a.col == b.col) ||
        (abs(a.col - b.col) == 1 && a.row == b.row);
}

void SwapCandies(Cell a, Cell b)
{
    Candy temp = board[a.row][a.col];
    board[a.row][a.col] = board[b.row][b.col];
    board[b.row][b.col] = temp;
}

void ResetDestroyFlags()
{
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            board[r][c].isMarkedToDestroy = false;
}

void ResetMovingFlags()
{
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            board[r][c].isMoving = false;
}

void ResetScales()
{
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            board[r][c].scale = 1.0f;
}

// Eşleşme kontrolü ve işaretleme
bool MarkMatches()
{
    bool found = false;
    // Satır kontrolü
    for (int r = 0; r < ROWS; r++)
    {
        int count = 1;
        for (int c = 1; c < COLS; c++)
        {
            if (board[r][c].type == board[r][c - 1].type)
                count++;
            else
                count = 1;
            if (count >= 3)
            {
                found = true;
                for (int k = 0; k < count; k++)
                    board[r][c - k].isMarkedToDestroy = true;
            }
        }
    }
    // Sütun kontrolü
    for (int c = 0; c < COLS; c++)
    {
        int count = 1;
        for (int r = 1; r < ROWS; r++)
        {
            if (board[r][c].type == board[r - 1][c].type)
                count++;
            else
                count = 1;
            if (count >= 3)
            {
                found = true;
                for (int k = 0; k < count; k++)
                    board[r - k][c].isMarkedToDestroy = true;
            }
        }
    }
    return found;
}

// Patlayanları yok et ve skor ekle
int DestroyMarkedCandies()
{
    int destroyed = 0;
    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            if (board[r][c].isMarkedToDestroy)
            {
                destroyed++;
                board[r][c].type = -1; // Boş
                board[r][c].scale = 0.0f;
            }
        }
    }
    return destroyed;
}

// Düşürme ve doldurma
void DropCandies()
{
    for (int c = 0; c < COLS; c++)
    {
        for (int r = ROWS - 1; r >= 0; r--)
        {
            if (board[r][c].type == -1)
            {
                // Yukarıdan ilk dolu şekeri bul
                int rr = r - 1;
                while (rr >= 0 && board[rr][c].type == -1)
                    rr--;
                if (rr >= 0)
                {
                    board[r][c] = board[rr][c];
                    board[r][c].isMoving = true;
                    board[r][c].yOffset = (float)((rr - r) * CELL_SIZE);
                    board[rr][c].type = -1;
                }
            }
        }
    }
    // En üstte boş kalanlara yeni şeker
    for (int c = 0; c < COLS; c++)
    {
        for (int r = 0; r < ROWS; r++)
        {
            if (board[r][c].type == -1)
            {
                board[r][c].type = GetRandomValue(0, CANDY_TYPES - 1);
                board[r][c].isMoving = true;
                board[r][c].yOffset = -((float)CELL_SIZE * (float)(r + 1));
                board[r][c].scale = 1.0f;
            }
        }
    }
}

// Animasyonları güncelle
bool UpdateAnimations()
{
    bool animating = false;
    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            if (board[r][c].isMoving)
            {
                if (board[r][c].yOffset < 0.0f)
                {
                    board[r][c].yOffset += ANIMATION_SPEED;
                    if (board[r][c].yOffset >= 0.0f)
                    {
                        board[r][c].yOffset = 0.0f;
                        board[r][c].isMoving = false;
                    }
                    else
                    {
                        animating = true;
                    }
                }
                else if (board[r][c].yOffset > 0.0f)
                {
                    board[r][c].yOffset -= ANIMATION_SPEED;
                    if (board[r][c].yOffset <= 0.0f)
                    {
                        board[r][c].yOffset = 0.0f;
                        board[r][c].isMoving = false;
                    }
                    else
                    {
                        animating = true;
                    }
                }
            }
            if (board[r][c].isMarkedToDestroy && board[r][c].scale > 0.0f)
            {
                board[r][c].scale -= DESTROY_ANIMATION_SPEED;
                if (board[r][c].scale < 0.0f)
                    board[r][c].scale = 0.0f;
                animating = true;
            }
        }
    }
    return animating;
}

// Swap sonrası eşleşme var mı kontrolü
bool IsValidSwap(Cell a, Cell b)
{
    SwapCandies(a, b);
    bool valid = MarkMatches();
    SwapCandies(a, b);
    ResetDestroyFlags();
    return valid;
}

// Oynanabilir hamle var mı?
bool HasValidMove()
{
    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            Cell a = { r, c, false };
            Cell b;
            // Sağ ile swap
            if (c < COLS - 1)
            {
                b = (Cell){ r, c + 1, false };
                if (IsValidSwap(a, b))
                    return true;
            }
            // Aşağı ile swap
            if (r < ROWS - 1)
            {
                b = (Cell){ r + 1, c, false };
                if (IsValidSwap(a, b))
                    return true;
            }
        }
    }
    return false;
}

// Tahtayı rastgele doldur, başlangıçta eşleşme olmasın
void FillBoardNoMatches()
{
    do
    {
        for (int r = 0; r < ROWS; r++)
        {
            for (int c = 0; c < COLS; c++)
            {
                board[r][c].type = GetRandomValue(0, CANDY_TYPES - 1);
                board[r][c].isMoving = false;
                board[r][c].yOffset = 0.0f;
                board[r][c].isMarkedToDestroy = false;
                board[r][c].scale = 1.0f;
            }
        }
        ResetDestroyFlags();
        MarkMatches();
    } while (MarkMatches() || !HasValidMove());
    ResetDestroyFlags();
}

// Skor hesaplama
void AddScore(int destroyed)
{
    if (destroyed == 3)
        score += 60 * comboMultiplier;
    else if (destroyed == 4)
        score += 100 * comboMultiplier;
    else if (destroyed >= 5)
        score += 200 * comboMultiplier;
}

// Çizim
void DrawBoard()
{
    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            int x = BOARD_OFFSET_X + c * CELL_SIZE;
            int y = BOARD_OFFSET_Y + r * CELL_SIZE + (int)board[r][c].yOffset;
            float scale = board[r][c].scale;

            if (board[r][c].type >= 0)
            {
                // Arka plan çerçevesi
                DrawRectangleLinesEx((Rectangle) { (float)x, (float)y, (float)CELL_SIZE, (float)CELL_SIZE }, 1, LIGHTGRAY);

                if (useTextures && board[r][c].type < CANDY_TYPES)
                {
                    // Dokular varsa doku ile çiz
                    float textureScale = scale * 0.9f; // Texture biraz daha küçük olsun
                    Rectangle sourceRect = { 0, 0, (float)candyTextures[board[r][c].type].width, (float)candyTextures[board[r][c].type].height };
                    Rectangle destRect = {
                        (float)x + (CELL_SIZE * (1.0f - textureScale) / 2.0f),
                        (float)y + (CELL_SIZE * (1.0f - textureScale) / 2.0f),
                        CELL_SIZE * textureScale,
                        CELL_SIZE * textureScale };
                    DrawTexturePro(
                        candyTextures[board[r][c].type],
                        sourceRect,
                        destRect,
                        (Vector2) {
                        0, 0
                    },
                        0.0f,
                        WHITE);
                }
                else
                {
                    // Dokular yoksa renkli daireler çiz
                    Color color = candyColors[board[r][c].type];
                    Rectangle rect = {
                        (float)x + (CELL_SIZE * (1.0f - scale) / 2.0f),
                        (float)y + (CELL_SIZE * (1.0f - scale) / 2.0f),
                        CELL_SIZE * scale,
                        CELL_SIZE * scale };
                    DrawRectangleRounded(rect, 0.3f, 8, color);
                }
            }

            // Seçili hücre vurgusu
            if (selectedCell.selected && selectedCell.row == r && selectedCell.col == c)
            {
                DrawRectangleLinesEx((Rectangle) { (float)(x + 2), (float)(y + 2), (float)(CELL_SIZE - 4), (float)(CELL_SIZE - 4) }, 4, GOLD);
            }
        }
    }
}

// PNG Dokuları Yükle
bool LoadCandyTextures()
{
    bool success = true;

    // Dosya varlığını kontrol et ve yükle
    if (FileExists("assets/candy_red.png"))
        candyTextures[CANDY_RED] = LoadTexture("assets/candy_red.png");
    else
        success = false;

    if (FileExists("assets/candy_green.png"))
        candyTextures[CANDY_GREEN] = LoadTexture("assets/candy_green.png");
    else
        success = false;

    if (FileExists("assets/candy_blue.png"))
        candyTextures[CANDY_BLUE] = LoadTexture("assets/candy_blue.png");
    else
        success = false;

    if (FileExists("assets/candy_yellow.png"))
        candyTextures[CANDY_YELLOW] = LoadTexture("assets/candy_yellow.png");
    else
        success = false;

    if (FileExists("assets/candy_purple.png"))
        candyTextures[CANDY_PURPLE] = LoadTexture("assets/candy_purple.png");
    else
        success = false;

    if (FileExists("assets/candy_orange.png"))
        candyTextures[CANDY_ORANGE] = LoadTexture("assets/candy_orange.png");
    else
        success = false;

    return success;
}

// Dokuları Boşalt
void UnloadCandyTextures()
{
    if (useTextures)
    {
        for (int i = 0; i < CANDY_TYPES; i++)
        {
            UnloadTexture(candyTextures[i]);
        }
    }
}

// Ana fonksiyon
int main(void)
{
    // time() geriye time_t döndürür, srand() için unsigned int gerekiyor
    srand((unsigned int)time(NULL));
    InitWindow(800, 700, "Candy Crush - Raylib");
    SetTargetFPS(60);

    // Renkleri burada ayarla - küresel değişken sabit bir değerle başlatılamıyor
    candyColors[CANDY_RED] = RED;
    candyColors[CANDY_GREEN] = GREEN;
    candyColors[CANDY_BLUE] = BLUE;
    candyColors[CANDY_YELLOW] = YELLOW;
    candyColors[CANDY_PURPLE] = PURPLE;
    candyColors[CANDY_ORANGE] = ORANGE;

    // PNG'leri yüklemeyi dene
    useTextures = LoadCandyTextures();

    if (!useTextures)
    {
        TraceLog(LOG_WARNING, "PNG dosyaları yüklenemedi! Renkli şekillerle devam ediliyor.");
    }

    FillBoardNoMatches();

    while (!WindowShouldClose())
    {
        // Animasyonlar
        isAnimating = UpdateAnimations();

        // Kullanıcı girişi
        if (!isAnimating && !isDestroying)
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                Vector2 mouse = GetMousePosition();
                int col = (int)((mouse.x - BOARD_OFFSET_X) / CELL_SIZE);
                int row = (int)((mouse.y - BOARD_OFFSET_Y) / CELL_SIZE);
                if (row >= 0 && row < ROWS && col >= 0 && col < COLS)
                {
                    if (!selectedCell.selected)
                    {
                        selectedCell = (Cell){ row, col, true };
                    }
                    else
                    {
                        Cell target = { row, col, false };
                        if (IsAdjacent(selectedCell, target))
                        {
                            if (IsValidSwap(selectedCell, target))
                            {
                                SwapCandies(selectedCell, target);
                                isSwapping = true;
                                swapTarget = target;
                            }
                            else
                            {
                                // Geçersiz hamle, seçimi kaldır
                                selectedCell.selected = false;
                            }
                        }
                        else
                        {
                            // Aynı hücreye tıklandıysa seçimi kaldır
                            if (selectedCell.row == row && selectedCell.col == col)
                                selectedCell.selected = false;
                            else
                                selectedCell = (Cell){ row, col, true };
                        }
                    }
                }
            }
        }

        // Swap sonrası eşleşme kontrolü
        if (isSwapping && !isAnimating)
        {
            ResetDestroyFlags();
            if (MarkMatches())
            {
                isDestroying = true;
                comboActive = true;
                comboMultiplier = 1;
            }
            else
            {
                // Geri al
                SwapCandies(selectedCell, swapTarget);
                isSwapping = false;
                selectedCell.selected = false;
            }
            isSwapping = false;
        }

        // Patlatma ve düşürme
        if (isDestroying && !isAnimating)
        {
            int destroyed = 0;
            if (MarkMatches())
            {
                destroyed = DestroyMarkedCandies();
                AddScore(destroyed);
                DropCandies();
                comboMultiplier++;
            }
            else
            {
                isDestroying = false;
                comboActive = false;
                comboMultiplier = 1;
                selectedCell.selected = false;
                ResetDestroyFlags();
                ResetScales();
            }
        }

        // Oynanabilir hamle yoksa tahtayı yeniden doldur
        if (!isAnimating && !isDestroying && !HasValidMove())
        {
            FillBoardNoMatches();
        }

        // Çizim
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText(TextFormat("Skor: %d", score), 100, 40, 36, DARKBLUE);
        if (comboActive && comboMultiplier > 1)
            DrawText(TextFormat("Combo x%d!", comboMultiplier - 1), 400, 40, 36, RED);

        DrawBoard();

        EndDrawing();
    }

    // Dokuları bellekten boşalt
    UnloadCandyTextures();

    CloseWindow();
    return 0;
}
