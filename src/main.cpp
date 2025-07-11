#include "raylib.h"
#include "flecs.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <ctime>

static constexpr int screenWidth{ 960 };
static constexpr int screenHeight{ 960 };
static constexpr int squaresWidthAmount{ 24 };
static constexpr int squaresHeightAmount{ 24 };

static const flecs::world ecs;
static bool gameShouldReset{ false };
static int applesInGame{ 3 };
static double squareWidth;
static double squareHeight;

// Components
struct Position {
    double x;
    double y;
};

struct GridPosition {
    int x;
    int y;
};

struct GridDirection {
    int x;
    int y;
};

struct RectangleProp {
    double width;
    double height;
};

struct ColorProp {
    Color color;
};

struct SnakeProp {
    double moveInterval;
    int snakeTailSize;

    double moveTimer{ 0.0 };
};

struct SnakeTailProp {
    int lifeLength;
};

// Tags
struct RectangleDraw { };
struct BindPositionToGridPosition { };
struct Apple { };

static void Init();
static void Update();

static void resetGame();
static void gridBoundCheck(GridPosition& gridPos);

int main() {
    InitWindow(screenWidth, screenHeight, "Snake");
    
    Init();

    while(!WindowShouldClose()) {
        Update();
    }

    return 0;
}

void Init() {
    srand(time(0));

    if (applesInGame > squaresWidthAmount * squaresHeightAmount)
        applesInGame = squaresWidthAmount * squaresHeightAmount;

    squareWidth = (double)screenWidth / squaresWidthAmount;
    squareHeight = (double)screenHeight / squaresHeightAmount;

    ecs.system<Position, RectangleProp, ColorProp, RectangleDraw>()
        .each([](Position& pos, RectangleProp& rectProp, ColorProp& colorProp, const RectangleDraw&) {
            DrawRectangle((int)pos.x, (int)pos.y, (int)rectProp.width, (int)rectProp.height, colorProp.color);
        });

    ecs.system<Position, GridPosition, BindPositionToGridPosition>()
        .each([](Position& pos, GridPosition& gridPos, const BindPositionToGridPosition&) {
            pos.x = gridPos.x * squareWidth;
            pos.y = gridPos.y * squareHeight;
        });

    ecs.system<GridDirection>()
        .each([](GridDirection& dir) {
            dir.x = std::clamp(dir.x, -1, 1);
            dir.y = std::clamp(dir.y, -1, 1);
        });
    
    ecs.system<GridPosition, GridDirection, ColorProp, SnakeProp>()
        .each([](GridPosition& gridPos, GridDirection& dir, ColorProp& colorProp, SnakeProp& SnakeProp) {
            SnakeProp.moveTimer += GetFrameTime();
            if (SnakeProp.moveTimer > SnakeProp.moveInterval) {
                SnakeProp.moveTimer = 0.0;

                gridPos.x += dir.x;
                gridPos.y += dir.y;

                gridBoundCheck(gridPos);

                auto snakeTailPropQ = ecs.query<SnakeTailProp>();

                snakeTailPropQ.each([](SnakeTailProp& tailProp) {
                    tailProp.lifeLength += 1;
                });
                
                int snakeTailCount = snakeTailPropQ.count();
                if (snakeTailCount <= SnakeProp.snakeTailSize) {
                    GridPosition newTailGridPos = GridPosition{ gridPos.x - dir.x, gridPos.y - dir.y };
                    gridBoundCheck(newTailGridPos);

                    ecs.entity()
                        .set(Position{})
                        .set(GridPosition{ newTailGridPos })
                        .set(RectangleProp{ squareWidth, squareHeight })
                        .set(ColorProp{ colorProp.color })
                        .set(SnakeTailProp{ 0 })
                        .add<RectangleDraw>()
                        .add<BindPositionToGridPosition>();
                } 
                if (snakeTailCount >= SnakeProp.snakeTailSize) {
                    flecs::entity oldestTail;
                    int oldestLife = 0;
                    snakeTailPropQ.each([&oldestTail, &oldestLife](flecs::entity ent, SnakeTailProp& tailProp) {
                        if (tailProp.lifeLength >= oldestLife) {
                            oldestTail = ent;
                            oldestLife = tailProp.lifeLength;
                        }
                    });

                    oldestTail.destruct();
                }
            }
        });

        ecs.system<const GridPosition, SnakeProp>()
            .each([](const GridPosition& snakeGridPos, SnakeProp& snakeProp) {
                auto snakeTailPropQ = ecs.query<const GridPosition, const SnakeTailProp>();
                snakeTailPropQ.each([snakeGridPos](const GridPosition& snakeTailGridPos, const SnakeTailProp&) {
                    if (snakeGridPos.x == snakeTailGridPos.x && snakeGridPos.y == snakeTailGridPos.y)
                        gameShouldReset = true;
                });

                auto appleQ = ecs.query<const GridPosition, const Apple>();
                appleQ.each([snakeGridPos, &snakeProp](flecs::entity appleEnt, const GridPosition& appleGridPos, const Apple&) {
                    if (snakeGridPos.x == appleGridPos.x && snakeGridPos.y == appleGridPos.y) {
                        snakeProp.snakeTailSize += 1;
                        appleEnt.destruct();
                    }
                });
            });
    
    flecs::entity snake = ecs.entity("Snake")
        .set(Position{})
        .set(GridPosition{squaresWidthAmount / 2 - 1, squaresHeightAmount / 2 - 1})
        .set(GridDirection{ 1, 0 })
        .set(RectangleProp{squareWidth, squareHeight})
        .set(ColorProp{BLUE})
        .set(SnakeProp{ 0.25, 3 })
        .add<RectangleDraw>()
        .add<BindPositionToGridPosition>();
}

void Update() {
    BeginDrawing();

    // Snake Input
    auto snakeQ = ecs.query<GridDirection, SnakeProp>();
    snakeQ.each([](GridDirection& dir, SnakeProp& SnakeProp) {
        if (IsKeyPressed(KEY_UP) && abs(dir.y) != 1) {
            dir.y = -1;
            dir.x = 0;
            SnakeProp.moveTimer = SnakeProp.moveInterval;
        }
        if (IsKeyPressed(KEY_LEFT) && abs(dir.x) != 1) {
            dir.y = 0;
            dir.x = -1;
            SnakeProp.moveTimer = SnakeProp.moveInterval;
        }
        if (IsKeyPressed(KEY_DOWN) && abs(dir.y) != 1) {
            dir.y = 1;
            dir.x = 0;
            SnakeProp.moveTimer = SnakeProp.moveInterval;
        }
        if (IsKeyPressed(KEY_RIGHT) && abs(dir.x) != 1) {
            dir.y = 0;
            dir.x = 1;
            SnakeProp.moveTimer = SnakeProp.moveInterval;
        }
    });

    // Apple Spawn
    auto appleQ = ecs.query<const GridPosition, const Apple>();
    if (appleQ.count() < applesInGame) {
        while (true) {
            bool appleAtPositionAlready = false;
            int randomX = rand() % squaresWidthAmount;
            int randomY = rand() % squaresHeightAmount;

            appleQ.each([randomX, randomY, &appleAtPositionAlready](const GridPosition& gridPos, const Apple&) {
                if (randomX == gridPos.x && randomY == gridPos.y)
                    appleAtPositionAlready = true;
            });

            if (appleAtPositionAlready)
                continue;

            ecs.entity()
                .set(Position{})
                .set(GridPosition{randomX, randomY})
                .set(RectangleProp{squareWidth, squareHeight})
                .set(ColorProp{RED})
                .add<RectangleDraw>()
                .add<BindPositionToGridPosition>()
                .add<Apple>();

            break;
        }
    }

    if (gameShouldReset) {
        resetGame();
        gameShouldReset = false;
    }
    ecs.progress();

    ClearBackground(Color{100, 255, 79, 255});

    EndDrawing();
}

void resetGame() {
    auto snakeTailQ = ecs.query<SnakeTailProp>();
    ecs.defer([snakeTailQ] {
        snakeTailQ.each([](flecs::entity ent, const SnakeTailProp&) {
            ent.destruct();
        });
    });

    auto appleQ = ecs.query<Apple>();
    ecs.defer([appleQ] {
        appleQ.each([](flecs::entity ent, const Apple&) {
            ent.destruct();
        });
    });

    auto snakeQ = ecs.query<GridPosition, GridDirection, SnakeProp>();
    snakeQ.each([](GridPosition& gridPos, GridDirection& dir, SnakeProp& snakeProp) {
        gridPos.x = squaresWidthAmount / 2 - 1;
        gridPos.y = squaresHeightAmount / 2 - 1;
        dir.x = 1;
        dir.y = 0;
    });
}

void gridBoundCheck(GridPosition& gridPos) {
    if (gridPos.x >= squaresWidthAmount)
        gridPos.x = 0;
    if (gridPos.x < 0)
        gridPos.x = squaresWidthAmount - 1;
    if (gridPos.y >= squaresHeightAmount)
        gridPos.y = 0;
    if (gridPos.y < 0)
        gridPos.y = squaresHeightAmount - 1;
}