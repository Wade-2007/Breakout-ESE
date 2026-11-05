#include "raylib.h"
#include <vector>
#include <cmath>
#include <cstdlib>

// Dimensiones de la pantalla
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// Configuración de la matriz de bloques
const int BRICK_ROWS = 5;
const int BRICK_COLS = 10;
const float BRICK_WIDTH = 70.0f;
const float BRICK_HEIGHT = 20.0f;
const float BRICK_PADDING = 8.0f;
const float BRICK_OFFSET_TOP = 60.0f;
const float BRICK_OFFSET_LEFT = (SCREEN_WIDTH - (BRICK_COLS * (BRICK_WIDTH + BRICK_PADDING) - BRICK_PADDING)) / 2.0f;

enum BrickType {
    BRICK_NORMAL,
    BRICK_EXPLOSIVE,   // Explota bloques a su alrededor (3x3)
    BRICK_MULTIPLIER   // Duplica la cantidad de pelotas en juego
};

struct Ball {
    Vector2 position;
    Vector2 speed;
    float radius;
    bool active;
};

struct Paddle {
    Rectangle rect;
    float speed;
};

struct Brick {
    Rectangle rect;
    bool active;
    BrickType type;
    Color color;
    int row;
    int col;
};

// ============================================================================
// FUNCIONES ORIGINALES (INTACTAS)
// ============================================================================

// Reinicia la lista de pelotas con una sola pelota sobre la paleta
void ResetBall(std::vector<Ball>& balls, const Paddle& paddle) {
    balls.clear();
    Ball newBall;
    newBall.position = (Vector2){ paddle.rect.x + paddle.rect.width / 2.0f, paddle.rect.y - 12.0f };
    newBall.speed = (Vector2){ 220.0f, -320.0f };
    newBall.radius = 8.0f;
    newBall.active = true;
    balls.push_back(newBall);
}

// Función para destruir bloques (recursiva para soportar explosiones en cadena)
void DestroyBrick(int index, std::vector<Brick>& bricks, int& score) {
    if (index < 0 || index >= (int)bricks.size() || !bricks[index].active) return;

    bricks[index].active = false;
    score += 10;

    // Si el bloque destruido es explosivo, destruye los bloques vecinos (matriz 3x3)
    if (bricks[index].type == BRICK_EXPLOSIVE) {
        int r = bricks[index].row;
        int c = bricks[index].col;

        for (size_t i = 0; i < bricks.size(); ++i) {
            if (bricks[i].active &&
                std::abs(bricks[i].row - r) <= 1 &&
                std::abs(bricks[i].col - c) <= 1) {
                DestroyBrick((int)i, bricks, score);
            }
        }
    }
}

// ============================================================================
// NUEVAS ESTRUCTURAS Y FUNCIONES DE MEJORA
// ============================================================================

enum PowerUpType {
    POWER_EXPAND,
    POWER_EXTRA_LIFE,
    POWER_SLOW_BALL
};

struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float alpha;
    float size;
    bool active;
};

struct PowerUp {
    Rectangle rect;
    PowerUpType type;
    Color color;
    float speedY;
    bool active;
};

// Genera partículas en la posición de un bloque destruido
void SpawnBrickParticles(std::vector<Particle>& particles, const Brick& brick) {
    int count = (brick.type == BRICK_EXPLOSIVE) ? 28 : 12;
    Vector2 center = { brick.rect.x + brick.rect.width / 2.0f, brick.rect.y + brick.rect.height / 2.0f };

    for (int i = 0; i < count; ++i) {
        Particle p;
        p.position = center;
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(60, (brick.type == BRICK_EXPLOSIVE) ? 280 : 160);
        p.velocity = { std::cos(angle) * speed, std::sin(angle) * speed };
        p.color = brick.color;
        p.alpha = 1.0f;
        p.size = (float)GetRandomValue(3, 6);
        p.active = true;
        particles.push_back(p);
    }
}

// Actualiza y limpia las partículas activas
void UpdateParticles(std::vector<Particle>& particles, float deltaTime) {
    for (size_t i = 0; i < particles.size(); ++i) {
        if (!particles[i].active) continue;
        particles[i].position.x += particles[i].velocity.x * deltaTime;
        particles[i].position.y += particles[i].velocity.y * deltaTime;
        particles[i].velocity.y += 180.0f * deltaTime; // Gravedad ligera
        particles[i].alpha -= 1.4f * deltaTime;
        if (particles[i].alpha <= 0.0f) {
            particles[i].active = false;
        }
    }
}

// Genera aleatoriamente un Power-Up al romper un bloque
void TrySpawnPowerUp(std::vector<PowerUp>& powerUps, const Brick& brick) {
    if (GetRandomValue(1, 100) <= 18) { // 18% de probabilidad
        PowerUp pu;
        pu.rect = { brick.rect.x + brick.rect.width / 2.0f - 18.0f, brick.rect.y, 36.0f, 14.0f };
        pu.speedY = 160.0f;
        pu.active = true;

        int r = GetRandomValue(1, 3);
        if (r == 1) {
            pu.type = POWER_EXPAND;
            pu.color = SKYBLUE;
        } else if (r == 2) {
            pu.type = POWER_EXTRA_LIFE;
            pu.color = LIME;
        } else {
            pu.type = POWER_SLOW_BALL;
            pu.color = DARKBLUE;
        }
        powerUps.push_back(pu);
    }
}

// Re-aleatoriza los tipos de bloques al reiniciar o avanzar de nivel
void RegenerateBricks(std::vector<Brick>& bricks) {
    for (size_t i = 0; i < bricks.size(); ++i) {
        bricks[i].active = true;
        int randVal = GetRandomValue(1, 100);
        if (randVal <= 10) {
            bricks[i].type = BRICK_EXPLOSIVE;
            bricks[i].color = RED;
        } else if (randVal <= 20) {
            bricks[i].type = BRICK_MULTIPLIER;
            bricks[i].color = PURPLE;
        } else {
            bricks[i].type = BRICK_NORMAL;
            bricks[i].color = (bricks[i].row % 2 == 0) ? GOLD : ORANGE;
        }
    }
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Breakout C++ - Bloques Especiales Mejorado");
    SetTargetFPS(60);

    // Estado general del juego
    int lives = 3;
    int score = 0;
    int highScore = 0;
    int combo = 0;
    int level = 1;
    bool gameOver = false;
    bool gameWon = false;
    bool paused = false;

    // Temporizador para el Power-Up de paleta expandida y temblor de cámara
    float expandTimer = 0.0f;
    float shakeTimer = 0.0f;

    Camera2D camera = { 0 };
    camera.zoom = 1.0f;

    // Inicializar Paleta
    Paddle paddle;
    paddle.rect = (Rectangle){ SCREEN_WIDTH / 2.0f - 60.0f, SCREEN_HEIGHT - 40.0f, 120.0f, 15.0f };
    paddle.speed = 500.0f;

    // Inicializar Pelotas
    std::vector<Ball> balls;
    ResetBall(balls, paddle);

    // Contenedores de efectos y power-ups
    std::vector<Particle> particles;
    std::vector<PowerUp> powerUps;

    // Inicializar Bloques con distribuciones aleatorias de bloques especiales
    std::vector<Brick> bricks;
    for (int r = 0; r < BRICK_ROWS; ++r) {
        for (int c = 0; c < BRICK_COLS; ++c) {
            Brick brick;
            brick.rect = (Rectangle){
                BRICK_OFFSET_LEFT + c * (BRICK_WIDTH + BRICK_PADDING),
                BRICK_OFFSET_TOP + r * (BRICK_HEIGHT + BRICK_PADDING),
                BRICK_WIDTH,
                BRICK_HEIGHT
            };
            brick.active = true;
            brick.row = r;
            brick.col = c;

            // 10% probabilidad Explosivo, 10% Multiplicador, 80% Normal
            int randVal = GetRandomValue(1, 100);
            if (randVal <= 10) {
                brick.type = BRICK_EXPLOSIVE;
                brick.color = RED;
            } else if (randVal <= 20) {
                brick.type = BRICK_MULTIPLIER;
                brick.color = PURPLE;
            } else {
                brick.type = BRICK_NORMAL;
                brick.color = (r % 2 == 0) ? GOLD : ORANGE;
            }

            bricks.push_back(brick);
        }
    }

    // Bucle Principal del Juego
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // Pausar con tecla P
        if (IsKeyPressed(KEY_P) && !gameOver && !gameWon) {
            paused = !paused;
        }

        if (!gameOver && !gameWon && !paused) {
            // --- TEMPORIZADOR DE PALETA EXPANDIDA ---
            if (expandTimer > 0.0f) {
                expandTimer -= deltaTime;
                if (expandTimer <= 0.0f) {
                    float center = paddle.rect.x + paddle.rect.width / 2.0f;
                    paddle.rect.width = 120.0f;
                    paddle.rect.x = center - paddle.rect.width / 2.0f;
                }
            }

            // --- TEMPORIZADOR DE SCREEN SHAKE ---
            if (shakeTimer > 0.0f) {
                shakeTimer -= deltaTime;
                camera.offset = (Vector2){ (float)GetRandomValue(-5, 5), (float)GetRandomValue(-5, 5) };
            } else {
                camera.offset = (Vector2){ 0.0f, 0.0f };
            }

            // --- CONTROLES DE LA PALETA ---
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) paddle.rect.x -= paddle.speed * deltaTime;
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) paddle.rect.x += paddle.speed * deltaTime;

            // Limitar movimiento dentro de la pantalla
            if (paddle.rect.x < 0) paddle.rect.x = 0;
            if (paddle.rect.x + paddle.rect.width > SCREEN_WIDTH) paddle.rect.x = SCREEN_WIDTH - paddle.rect.width;

            // --- ACTUALIZACIÓN DE PARTÍCULAS Y POWER-UPS ---
            UpdateParticles(particles, deltaTime);

            for (size_t p = 0; p < powerUps.size(); ++p) {
                if (!powerUps[p].active) continue;
                powerUps[p].rect.y += powerUps[p].speedY * deltaTime;

                if (CheckCollisionRecs(powerUps[p].rect, paddle.rect)) {
                    powerUps[p].active = false;
                    score += 25;
                    if (powerUps[p].type == POWER_EXPAND) {
                        paddle.rect.width = 180.0f;
                        expandTimer = 8.0f;
                    } else if (powerUps[p].type == POWER_EXTRA_LIFE) {
                        lives++;
                    } else if (powerUps[p].type == POWER_SLOW_BALL) {
                        for (size_t k = 0; k < balls.size(); ++k) {
                            balls[k].speed.x *= 0.8f;
                            balls[k].speed.y *= 0.8f;
                        }
                    }
                } else if (powerUps[p].rect.y > SCREEN_HEIGHT) {
                    powerUps[p].active = false;
                }
            }

            // --- ACTUALIZACIÓN DE PELOTAS ---
            int activeBallsCount = 0;

            for (size_t i = 0; i < balls.size(); ++i) {
                if (!balls[i].active) continue;

                balls[i].position.x += balls[i].speed.x * deltaTime;
                balls[i].position.y += balls[i].speed.y * deltaTime;

                // Paredes laterales
                if (balls[i].position.x - balls[i].radius <= 0) {
                    balls[i].position.x = balls[i].radius;
                    balls[i].speed.x *= -1;
                } else if (balls[i].position.x + balls[i].radius >= SCREEN_WIDTH) {
                    balls[i].position.x = SCREEN_WIDTH - balls[i].radius;
                    balls[i].speed.x *= -1;
                }

                // Techo
                if (balls[i].position.y - balls[i].radius <= 0) {
                    balls[i].position.y = balls[i].radius;
                    balls[i].speed.y *= -1;
                }

                // Pelota fuera del límite inferior (caída)
                if (balls[i].position.y - balls[i].radius > SCREEN_HEIGHT) {
                    balls[i].active = false;
                    continue;
                }

                activeBallsCount++;

                // Colisión con Paleta
                if (CheckCollisionCircleRec(balls[i].position, balls[i].radius, paddle.rect)) {
                    balls[i].speed.y = -std::abs(balls[i].speed.y);
                    // Cambiar ángulo según el punto de impacto en la paleta
                    float hitFactor = (balls[i].position.x - (paddle.rect.x + paddle.rect.width / 2.0f)) / (paddle.rect.width / 2.0f);
                    balls[i].speed.x = hitFactor * 350.0f;
                    combo = 0; // Reiniciar racha de combo al tocar la paleta
                }

                // Colisión con Bloques
                for (size_t b = 0; b < bricks.size(); ++b) {
                    if (bricks[b].active && CheckCollisionCircleRec(balls[i].position, balls[i].radius, bricks[b].rect)) {
                        // Determinación de rebote horizontal o vertical según solapamiento
                        float overlapLeft = (balls[i].position.x + balls[i].radius) - bricks[b].rect.x;
                        float overlapRight = (bricks[b].rect.x + bricks[b].rect.width) - (balls[i].position.x - balls[i].radius);
                        float overlapTop = (balls[i].position.y + balls[i].radius) - bricks[b].rect.y;
                        float overlapBottom = (bricks[b].rect.y + bricks[b].rect.height) - (balls[i].position.y - balls[i].radius);

                        float minOverlapX = (overlapLeft < overlapRight) ? overlapLeft : overlapRight;
                        float minOverlapY = (overlapTop < overlapBottom) ? overlapTop : overlapBottom;

                        if (minOverlapX < minOverlapY) {
                            balls[i].speed.x *= -1;
                        } else {
                            balls[i].speed.y *= -1;
                        }

                        BrickType hitType = bricks[b].type;

                        // Guardar estado previo de bloques activos para detectar cuáles explotaron
                        std::vector<bool> wasActive(bricks.size());
                        for (size_t m = 0; m < bricks.size(); ++m) wasActive[m] = bricks[m].active;

                        // Destruir bloque y manejar efectos explosivos (función original intacta)
                        DestroyBrick((int)b, bricks, score);

                        // Generar partículas y power-ups para todos los bloques recién destruidos
                        for (size_t m = 0; m < bricks.size(); ++m) {
                            if (wasActive[m] && !bricks[m].active) {
                                SpawnBrickParticles(particles, bricks[m]);
                                TrySpawnPowerUp(powerUps, bricks[m]);
                                if (bricks[m].type == BRICK_EXPLOSIVE) {
                                    shakeTimer = 0.22f;
                                }
                            }
                        }

                        // Bonificación por combo consecutivo
                        combo++;
                        if (combo > 1) {
                            score += (combo - 1) * 5;
                        }
                        if (score > highScore) highScore = score;

                        // Si el bloque era multiplicador, se duplica cada pelota activa
                        if (hitType == BRICK_MULTIPLIER) {
                            int currentBallCount = (int)balls.size();
                            for (int k = 0; k < currentBallCount; ++k) {
                                if (balls[k].active) {
                                    Ball duplicatedBall = balls[k];
                                    // Variación de dirección horizontal para separar las pelotas
                                    duplicatedBall.speed.x = -balls[k].speed.x + GetRandomValue(-80, 80);
                                    balls.push_back(duplicatedBall);
                                }
                            }
                        }

                        break; // Procesar una colisión por fotograma para esta pelota
                    }
                }
            }

            // Si no quedan pelotas activas, se resta una vida
            if (activeBallsCount == 0) {
                lives--;
                combo = 0;
                if (lives > 0) {
                    paddle.rect.width = 120.0f;
                    expandTimer = 0.0f;
                    paddle.rect.x = SCREEN_WIDTH / 2.0f - paddle.rect.width / 2.0f;
                    ResetBall(balls, paddle);
                } else {
                    gameOver = true;
                }
            }

            // Comprobar condición de victoria (todos los bloques destruidos)
            bool remainingBricks = false;
            for (const auto& brick : bricks) {
                if (brick.active) {
                    remainingBricks = true;
                    break;
                }
            }
            if (!remainingBricks) gameWon = true;
        }

        // --- RENDERIZADO ---
        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode2D(camera);

        // Dibujar Partículas
        for (const auto& p : particles) {
            if (p.active) {
                DrawRectangleV(p.position, (Vector2){ p.size, p.size }, Fade(p.color, p.alpha));
            }
        }

        // Dibujar Power-Ups
        for (const auto& pu : powerUps) {
            if (pu.active) {
                DrawRectangleRounded(pu.rect, 0.5f, 6, pu.color);
                DrawRectangleLinesEx(pu.rect, 1, WHITE);
                const char* label = (pu.type == POWER_EXPAND) ? "+TAM" : (pu.type == POWER_EXTRA_LIFE) ? "+1V" : "LNT";
                DrawText(label, (int)pu.rect.x + 4, (int)pu.rect.y + 2, 10, WHITE);
            }
        }

        // Dibujar Paleta
        DrawRectangleRec(paddle.rect, (expandTimer > 0.0f) ? SKYBLUE : LIGHTGRAY);
        DrawRectangleLinesEx(paddle.rect, 1, WHITE);

        // Dibujar Pelotas activas
        for (const auto& ball : balls) {
            if (ball.active) {
                DrawCircleV(ball.position, ball.radius + 2.0f, Fade(SKYBLUE, 0.3f));
                DrawCircleV(ball.position, ball.radius, WHITE);
            }
        }

        // Dibujar Bloques
        for (const auto& brick : bricks) {
            if (brick.active) {
                DrawRectangleRec(brick.rect, brick.color);
                DrawRectangleLinesEx(brick.rect, 1, DARKGRAY);

                if (brick.type == BRICK_EXPLOSIVE) {
                    DrawText("EXP", (int)brick.rect.x + 18, (int)brick.rect.y + 4, 12, WHITE);
                } else if (brick.type == BRICK_MULTIPLIER) {
                    DrawText("x2", (int)brick.rect.x + 25, (int)brick.rect.y + 4, 12, WHITE);
                }
            }
        }

        EndMode2D();

        // HUD (Puntaje, Récord, Combo, Nivel y Vidas)
        DrawText(TextFormat("Puntaje: %d", score), 20, 20, 20, GREEN);
        DrawText(TextFormat("Record: %d", highScore), 200, 20, 20, GOLD);
        DrawText(TextFormat("Nivel: %d", level), 380, 20, 20, SKYBLUE);
        if (combo > 1) {
            DrawText(TextFormat("Combo x%d!", combo), 510, 20, 20, ORANGE);
        }
        DrawText(TextFormat("Vidas: %d", lives), SCREEN_WIDTH - 120, 20, 20, RED);

        // Leyenda inferior
        DrawRectangle(20, SCREEN_HEIGHT - 25, 12, 12, RED);
        DrawText("Explosivo (3x3)", 38, SCREEN_HEIGHT - 25, 14, RAYWHITE);
        DrawRectangle(180, SCREEN_HEIGHT - 25, 12, 12, PURPLE);
        DrawText("Multiplicador (x2)", 198, SCREEN_HEIGHT - 25, 14, RAYWHITE);
        DrawText("[P] Pausa", SCREEN_WIDTH - 100, SCREEN_HEIGHT - 25, 14, GRAY);

        // Mensaje de Pausa
        if (paused) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.6f));
            DrawText("JUEGO EN PAUSA", SCREEN_WIDTH / 2 - 140, SCREEN_HEIGHT / 2 - 20, 32, RAYWHITE);
            DrawText("Presiona P para continuar", SCREEN_WIDTH / 2 - 135, SCREEN_HEIGHT / 2 + 25, 20, LIGHTGRAY);
        }

        // Mensajes de estado (Game Over y Victoria)
        if (gameOver) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.7f));
            DrawText("GAME OVER", SCREEN_WIDTH / 2 - 120, SCREEN_HEIGHT / 2 - 20, 40, RED);
            DrawText("Presiona R para reiniciar", SCREEN_WIDTH / 2 - 130, SCREEN_HEIGHT / 2 + 30, 20, GRAY);
            if (IsKeyPressed(KEY_R)) {
                lives = 3;
                score = 0;
                combo = 0;
                level = 1;
                gameOver = false;
                particles.clear();
                powerUps.clear();
                paddle.rect.width = 120.0f;
                paddle.rect.x = SCREEN_WIDTH / 2.0f - paddle.rect.width / 2.0f;
                ResetBall(balls, paddle);
                RegenerateBricks(bricks);
            }
        } else if (gameWon) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.7f));
            DrawText("¡VICTORIA!", SCREEN_WIDTH / 2 - 110, SCREEN_HEIGHT / 2 - 20, 40, GREEN);
            DrawText("Presiona R para el siguiente nivel", SCREEN_WIDTH / 2 - 175, SCREEN_HEIGHT / 2 + 30, 20, GRAY);
            if (IsKeyPressed(KEY_R)) {
                level++;
                combo = 0;
                gameWon = false;
                particles.clear();
                powerUps.clear();
                paddle.rect.width = 120.0f;
                paddle.rect.x = SCREEN_WIDTH / 2.0f - paddle.rect.width / 2.0f;
                ResetBall(balls, paddle);
                // Aumentar ligeramente la velocidad inicial según el nivel
                balls[0].speed.x *= (1.0f + (level - 1) * 0.08f);
                balls[0].speed.y *= (1.0f + (level - 1) * 0.08f);
                RegenerateBricks(bricks);
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}