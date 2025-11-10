#include <raylib.h>
#include <cmath>
#include <vector>
#include <fstream>
#include <iostream>
#include <string>

// Rotate a vector by an angle
Vector2 RotateVector(Vector2 v, float cosA, float sinA) {
    return {
        v.x * cosA - v.y * sinA,
        v.x * sinA + v.y * cosA
    };
}

// Add two vectors
Vector2 Vector2Add(Vector2 v1, Vector2 v2) {
    return { v1.x + v2.x, v1.y + v2.y };
}

void DrawShip(Vector2 pos, float angle) {
    float size = 20.0f;

    Vector2 p1 = { 0, -size };           // Tip
    Vector2 p2 = { -size / 2, size };    // Left
    Vector2 p3 = { size / 2, size };     // Right

    float cosA = cosf(angle);
    float sinA = sinf(angle);

    p1 = Vector2Add(pos, RotateVector(p1, cosA, sinA));
    p2 = Vector2Add(pos, RotateVector(p2, cosA, sinA));
    p3 = Vector2Add(pos, RotateVector(p3, cosA, sinA));

    DrawTriangle(p1, p2, p3, WHITE);
}

struct Bullet {
    Vector2 position;
    Vector2 velocity;
    float radius = 3.0f;
    bool active = true;
};

struct Asteroid {
    Vector2 position;
	Vector2 velocity;
    float radius;
	bool active = true;
};

void createAsteroid(std::vector<Asteroid>& asteroids, Vector2 position, Vector2 velocity, float radius) {
    Asteroid a;
    a.position = position;
    a.velocity = velocity;
    if (a.velocity.x == 0 && a.velocity.y == 0) a.velocity = { 1.0f, 1.0f, };
    
	a.radius = radius;
	a.active = true;
    asteroids.push_back(a);
}

std::vector<Vector2> GetShipPoints(Vector2 pos, float angle) {
    float size = 20.0f;
    float cosA = cosf(angle);
    float sinA = sinf(angle);

    Vector2 p1 = Vector2Add(pos, RotateVector({ 0, -size }, cosA, sinA));
    Vector2 p2 = Vector2Add(pos, RotateVector({ -size / 2, size }, cosA, sinA));
    Vector2 p3 = Vector2Add(pos, RotateVector({ size / 2, size }, cosA, sinA));

    return { p1, p2, p3 };
}

bool CheckCollisionCircleLine(Vector2 circleCenter, float radius, Vector2 lineStart, Vector2 lineEnd) {
	Vector2 ac = { circleCenter.x - lineStart.x, circleCenter.y - lineStart.y };
	Vector2 ab = { lineEnd.x - lineStart.x, lineEnd.y - lineStart.y };

	float abLenSq = ab.x * ab.x + ab.y * ab.y;
    float dot = (ac.x * ab.x + ac.y * ab.y) / abLenSq;

    dot = fmaxf(0.0f, fminf(1.0f, dot));

	Vector2 closest = { lineStart.x + ab.x * dot, lineStart.y + ab.y * dot };

	float dx = circleCenter.x - closest.x;
	float dy = circleCenter.y - closest.y;
	float distSq = dx * dx + dy * dy;

	return distSq <= radius * radius;
}

void resetPlayer(Vector2& shipPos, Vector2& shipVelocity, float& shipAngle) {
    shipPos = { 400, 300 };
    shipVelocity = { 0, 0 };
    shipAngle = 0.0f;
}

bool checkPlayerDistance(Vector2 spawnPos, Vector2 playerPos, float minDistance) {
    float dx = spawnPos.x - playerPos.x;
    float dy = spawnPos.y - playerPos.y;
    float distSq = dx * dx + dy * dy;
    return distSq >= minDistance * minDistance;
}

void SpawnRandomAsteroids(std::vector<Asteroid>& asteroids, int count, Vector2 playerPos = { -1000.0f, -1000.0f }) {
    for (int i = 0; i < count; ++i) {
        Vector2 spawnPos;
        int attempts = 0;
        do {
            spawnPos = { static_cast<float>(GetRandomValue(0, GetScreenWidth())), static_cast<float>(GetRandomValue(0, GetScreenHeight())) };
            ++attempts;
        } while ((playerPos.x >= 0.0f) && !checkPlayerDistance(spawnPos, playerPos, 200.0f) && attempts < 300);

        Vector2 velocity = { static_cast<float>(GetRandomValue(-2, 2)), static_cast<float>(GetRandomValue(-2, 2)) };
        if (velocity.x == 0.0f && velocity.y == 0.0f) velocity = { 1.0f, 1.0f };

        float radius = static_cast<float>(GetRandomValue(20, 50));
        createAsteroid(asteroids, spawnPos, velocity, radius);
    }
}

void destroyAllAsteroids(std::vector<Asteroid>& asteroids) {
    for (auto& a : asteroids) {
        a.active = false;
    }
    asteroids.clear();
}

void timer(int time, std::vector<Asteroid>& asteroids) {
    static float startTime = GetTime();
    float elapsedTime = GetTime() - startTime;
    
    if (elapsedTime >= time) {
		SpawnRandomAsteroids(asteroids, 1);
        startTime = GetTime();
    }
}

void writeData(int data) {
    std::ofstream highscore_;
     highscore_.open("highscore.txt");
     highscore_ << data;
     highscore_.close();
}

int readData() {
    std::ifstream file("highscore.txt");
    std::string line;
    int highScore = 0;

    if (std::getline(file, line)) {
        highScore = std::stoi(line);
    }
    file.close();
    return highScore;
}

int main() {
    InitWindow(800, 600, "Asteroids");
    InitAudioDevice();
	Sound shootSound = LoadSound("laserShoot.wav");
    Sound explosionSound = LoadSound("explosion.wav");
    Sound explosionSound2 = LoadSound("explosion1.wav");
    SetTargetFPS(60);

    Vector2 shipPos = { 400, 300 };
    Vector2 shipVelocity = { 0, 0 };
    float shipAngle = 0.0f;

    std::vector<Bullet> bullets;
    std::vector<Asteroid> asteroids;

    bool playing = true;
	bool shipHit = false;
    int score = 0;
    int scoreMultiplier = 15;
    int highScore = readData();
    std::cout << ("Reading file...\n");
    std::cout << ("Highscore: ", highScore);
	
    while (!WindowShouldClose()) {
        
        SpawnRandomAsteroids(asteroids, 5, shipPos);
        
        while (playing) {
            // 1. Input
            if (IsKeyDown(KEY_LEFT)) shipAngle -= 0.1f;
            if (IsKeyDown(KEY_RIGHT)) shipAngle += 0.1f;

            if (IsKeyDown(KEY_UP)) {
                shipVelocity.x += sinf(shipAngle) * 0.1f;
                shipVelocity.y -= cosf(shipAngle) * 0.1f;
            }
            else {
                shipVelocity.x *= 0.99f;
                shipVelocity.y *= 0.99f;
            }
            if (IsKeyPressed(KEY_K)) {
				destroyAllAsteroids(asteroids);
            }
            if (IsKeyPressed(KEY_H)) {
                SpawnRandomAsteroids(asteroids, 1, shipPos);
			}
            if (IsKeyDown(KEY_ESCAPE)) {
                playing = false;
			}

            // 2. Update
            shipPos.x += shipVelocity.x;
            shipPos.y += shipVelocity.y;

                // Screen wraparound
            if (shipPos.x < 0) shipPos.x += GetScreenWidth();
            if (shipPos.x > GetScreenWidth()) shipPos.x -= GetScreenWidth();
            if (shipPos.y < 0) shipPos.y += GetScreenHeight();
            if (shipPos.y > GetScreenHeight()) shipPos.y -= GetScreenHeight();

                // Fire bullet
            if (IsKeyPressed(KEY_SPACE)) {
                PlaySound(shootSound);

                Bullet b;
                b.position = shipPos;

                float bulletSpeed = 15.0f;
                b.velocity = {
                    sinf(shipAngle) * bulletSpeed,
                    -cosf(shipAngle) * bulletSpeed
                };

                b.radius = 2.0f;
                b.active = true;
                bullets.push_back(b);
            }

                // Update bullets
            for (auto& b : bullets) {
                if (!b.active) continue;

                b.position.x += b.velocity.x;
                b.position.y += b.velocity.y;

                if (b.position.x < 0 || b.position.x > GetScreenWidth() ||
                    b.position.y < 0 || b.position.y > GetScreenHeight()) {
                    b.active = false;
                    continue;
                }

                for (auto& a : asteroids) {
                    if (!a.active) continue;

                    if (CheckCollisionCircles(b.position, b.radius, a.position, a.radius)) {
                        score += a.radius * scoreMultiplier;
                        b.active = false;
                        a.active = false;

                        if (a.radius >= 30.0f) {
                            // Split into two smaller asteroids
                            float newRadius = a.radius / 2.0f;
                            Vector2 basePos = a.position;

                            Vector2 v1 = { a.velocity.x + GetRandomValue(-1, 1), a.velocity.y + GetRandomValue(-1, 1) };
                            Vector2 v2 = { a.velocity.x + GetRandomValue(-1, 1), a.velocity.y + GetRandomValue(-1, 1) };

                            if (v1.x == 0) v1.x = 1.0f;
                            if (v1.y == 0) v1.y = -1.0f;
                            if (v2.x == 0) v2.x = -1.0f;
                            if (v2.y == 0) v2.y = 1.0f;

                            createAsteroid(asteroids, basePos, v1, newRadius);
                            createAsteroid(asteroids, basePos, v2, newRadius);
                        }

                        PlaySound(explosionSound);
                        break;
                    }
                }
            }

			    // Update asteroids
            for (auto& a : asteroids) {
                if (!a.active) continue;

                a.position.x += a.velocity.x;
                a.position.y += a.velocity.y;

                if (a.position.x < 0) a.position.x += GetScreenWidth();
                if (a.position.x > GetScreenWidth()) a.position.x -= GetScreenWidth();
                if (a.position.y < 0) a.position.y += GetScreenHeight();
                if (a.position.y > GetScreenHeight()) a.position.y -= GetScreenHeight();

                std::vector<Vector2> shipPoints = GetShipPoints(shipPos, shipAngle);
                if (CheckCollisionCircleLine(a.position, a.radius, shipPoints[0], shipPoints[1]) ||
                    CheckCollisionCircleLine(a.position, a.radius, shipPoints[1], shipPoints[2]) ||
                    CheckCollisionCircleLine(a.position, a.radius, shipPoints[2], shipPoints[0])) {
                    a.active = false;
                    shipHit = true;
                }
            }

            asteroids.erase(
                std::remove_if(asteroids.begin(), asteroids.end(), [](Asteroid& a) {
                    return !a.active;
                    }),
                asteroids.end()
            );

            if (shipHit) {
                PlaySound(explosionSound2);
                destroyAllAsteroids(asteroids);
				shipHit = false;
                playing = false;
            }

            bullets.erase(
                std::remove_if(bullets.begin(), bullets.end(), [](Bullet& b) {
                    return !b.active;
                    }),
                bullets.end()
            );

            if (score < 10000) {
                timer(2, asteroids);
                timer(5, asteroids);
                timer(6, asteroids);
                timer(9, asteroids);
            }
            else {
                timer(1, asteroids);
                timer(3, asteroids);
                timer(5, asteroids);
                timer(9, asteroids);
                scoreMultiplier = 20;
            }
            // 3. Draw
            BeginDrawing();
            ClearBackground(BLACK);

            DrawShip(shipPos, shipAngle);

            for (const auto& b : bullets) {
                if (b.active) {
                    DrawCircleV(b.position, b.radius, WHITE);
                }
            }
            for (const auto& a : asteroids) {
                if (a.active) {
                    DrawCircleV(a.position, a.radius, WHITE);
                }
            }

            DrawText(TextFormat("Score: %d", score), 10, 10, 20, WHITE);
            EndDrawing();
        }

        BeginDrawing();
		ClearBackground(BLACK);

        if (score > readData()) {
            highScore = score;
            writeData(score);
            std::cout << ("\nUpdated score to: ", highScore);
        }       

        DrawText("Press Enter to Restart", GetScreenWidth() / 2 - 100, GetScreenHeight() / 2, 20, WHITE);
        DrawText(TextFormat("Score: %d", score), GetScreenWidth() / 2 - 50, GetScreenHeight() / 2 + 30, 20, WHITE);
        DrawText(TextFormat("High Score: %d", highScore), GetScreenWidth() / 2 - 70, GetScreenHeight() / 2 + 60, 20, WHITE);

        if (IsKeyPressed(KEY_ENTER)) {
			resetPlayer(shipPos, shipVelocity, shipAngle);
			bullets.clear();
			asteroids.clear();
            score = 0;
            playing = true;
		}
        EndDrawing();
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}