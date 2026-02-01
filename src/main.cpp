#include <M5Cardputer.h>
#include <ArduinoJson.h>
#include <vector>
#include <SD.h>
#include <SPI.h>

#define SNAKE_SIZE 5
#define COLLISION_OFFSET 10
#define GROWTH_INCREMENT 2
#define SPEEDUP_THRESHOLD 5
#define SPEEDUP_PERCENTAGE 10

enum class SnakeDirection {
  RIGHT = 0,
  DOWN,
  LEFT,
  UP
};

struct Position {
  int x;
  int y;
};

struct Player {
  std::vector<Position> position;
  SnakeDirection direction = SnakeDirection::RIGHT;
  Position prevTail;
  int speedMultiplier = 1;
  int fruitsEaten = 0;
  bool eatFruit = false;
};

struct Fruit {
  Position position;
};

struct GameState {
  bool gameOver = false;
  bool isPaused = false;
  int highScore = 0;
  bool sdCardInserted = false;
  const char* recordFile = "/snake-config.json";
};

void initSD(GameState* game);
void saveHighScore(const GameState& game);
void initGame(GameState* game);
void readButtons(SnakeDirection* direction, bool* isPaused);
void readPauseButton(bool* isPaused);
void readRestartButton(GameState* game);
void moveSnake(Player* snake, Fruit* fruit, GameState* game);
void checkCollision(std::vector<Position>& snake, GameState* game);
void playGameOverSound();
void displayGameOver(int fruitsEaten, int highScore);
void drawStaticElements(int fruitsEaten, int highScore, const Position& fruit);
void draw(Player* snake, int highScore);
void placeFruit(const std::vector<Position>& snake, Position* fruit);
void printScoreBoard(int fruitsEaten, int highScore);
void saveGameConfig(const GameState& game);

__attribute__((noreturn)) void setup() {
  GameState game;
  auto cfg = m5::M5Unified::config();

  M5Cardputer.begin(cfg);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5Cardputer.Display.fillScreen(TFT_BLACK);
  Serial.begin(115200);
  initSD(&game);
  initGame(&game);
}

void initSD(GameState* game) {
  static SPIClass SPICardputer(FSPI);
  SPICardputer.begin(SDCARD_SCK, SDCARD_MISO, SDCARD_MOSI);

  Serial.println("initSD: initializing SD card...");
  if (!SD.begin(SDCARD_CS, SPICardputer)) {
    Serial.println("initSD: Card Mount Failed");
    game->highScore = 0;
    game->sdCardInserted = false;
    return;
  }

  Serial.println("initSD: Card mounted with success");
  game->sdCardInserted = true;
  if (SD.exists(game->recordFile)) {
    loadGameConfig(game);
  } else {
    saveGameConfig(*game);
    game->highScore = 0;
  }
}

void saveGameConfig(const GameState& game) {
  JsonDocument doc;

  doc["highScore"] = game.highScore;
  doc["difficulty"] = 1;

  if (File file = SD.open(game.recordFile, FILE_WRITE); file) {
    serializeJson(doc, file);
    file.close();
    Serial.println("saveGameConfig: write OK");
  } else {
    Serial.println("saveGameConfig: fail to open file to write");
  }
}

void loadGameConfig(GameState* game) {
  if (File file = SD.open(game->recordFile); file) {
    JsonDocument doc;
    deserializeJson(doc, file);
    game->highScore = doc["highScore"] | 0;
    Serial.print("loadGameConfig: highScore read = ");
    Serial.println(game->highScore);
    file.close();
  }
}

void saveHighScore(const GameState& game) {
  if (game.highScore > 0 && game.sdCardInserted) {
    Serial.print("saveHighScore: writing highScore = ");
    Serial.println(game.highScore);

    saveGameConfig(game);
  }
}

__attribute__((noreturn)) void initGame(GameState* game) {
  Player snake;
  Fruit fruit;

  snake.position.clear();
  snake.position.push_back({ 60, 60 });
  for (int i = 1; i < 5; i++) {
    snake.position.push_back({ 60 - i * SNAKE_SIZE, 60 });
  }

  snake.direction = SnakeDirection::RIGHT; // Set default direction to right
  snake.fruitsEaten = 0; // Reset counter of eaten fruits
  snake.speedMultiplier = 0; // Reset speed multiplier
  placeFruit(snake.position, &fruit.position); // Place fruit in a random location
  M5Cardputer.Display.clear();
  game->gameOver = false; // Reset game over state
  game->isPaused = false; // Reset pause state


  drawStaticElements(snake.fruitsEaten, game->highScore, fruit.position);

  long lastTime = millis();
  while (true) {
    int frameRate = 100 - snake.speedMultiplier * 10;
    long currentTime = millis();

    M5Cardputer.update();

    if (!game->gameOver) {
      if (!game->isPaused) {
        if (currentTime - lastTime >= frameRate) {
          readButtons(&snake.direction, &game->isPaused);
          moveSnake(&snake, &fruit, game);
          checkCollision(snake.position, game);
          draw(&snake, game->highScore);
          lastTime = currentTime;
        }
      } else {
        readPauseButton(&game->isPaused);
      }
    } else {
      if (!game->isPaused) { // Added to draw "GAME OVER" message only once
        playGameOverSound();
        displayGameOver(snake.fruitsEaten, game->highScore);
        game->isPaused = true; // Stop further drawing
      }
      readRestartButton(game);
    }
  }
}

void loop() { // Used on initGame
  M5Cardputer.update();
}

void readButtons(SnakeDirection* direction, bool* isPaused) {
  using enum SnakeDirection;

  if (M5Cardputer.Keyboard.isKeyPressed(';') && *direction != DOWN) {
    *direction = UP;
  }
  if (M5Cardputer.Keyboard.isKeyPressed('.') && *direction != UP) {
    *direction = DOWN;
  }
  if (M5Cardputer.Keyboard.isKeyPressed(',') && *direction != RIGHT) {
    *direction = LEFT;
  }
  if (M5Cardputer.Keyboard.isKeyPressed('/') && *direction != LEFT) {
    *direction = RIGHT;
  }
  readPauseButton(isPaused);
}

void readPauseButton(bool* isPaused) {
  if (M5Cardputer.Keyboard.isKeyPressed('p')) { // Button 'p' on the keyboard
    *isPaused = !*isPaused;
    delay(200); // Delay to avoid double switching
  }
}

void readRestartButton(GameState* game) {
  if (M5Cardputer.Keyboard.isKeyPressed('n')) { // Button 'n' on the keyboard
    delay(200); // Delay to avoid double switching
    initGame(game); // Initialize the game again
  }
}

void moveSnake(Player* snake, Fruit* fruit, GameState* game) {
  using enum SnakeDirection;
  snake->prevTail = snake->position.back();
  Position next = snake->position[0];
  switch (snake->direction) {
    case RIGHT: next.x += SNAKE_SIZE; break;
    case DOWN: next.y += SNAKE_SIZE; break;
    case LEFT: next.x -= SNAKE_SIZE; break;
    case UP: next.y -= SNAKE_SIZE; break;
    default: next.x = 0; next.y = 0; break;
  }

  // Handle screen wrapping
  if (next.x >= TFT_WIDTH) next.x = 0;
  if (next.x < 0) next.x = TFT_WIDTH - SNAKE_SIZE;
  if (next.y >= TFT_HEIGHT) next.y = 0;
  if (next.y < 0) next.y = TFT_HEIGHT - SNAKE_SIZE;

  // Move tail to front
  snake->position.insert(snake->position.begin(), next);
  snake->position.pop_back();

  // Check if snake eats the fruit
  if (abs(snake->position[0].x - fruit->position.x) < COLLISION_OFFSET && abs(snake->position[0].y - fruit->position.y) < COLLISION_OFFSET) {
    for (int i = 0; i < GROWTH_INCREMENT; i++) {
      snake->position.push_back(snake->position.back()); // Increase snake length
    }
    // Play sound
    M5Cardputer.Speaker.tone(4000, 100); // Play a short sound when eating fruit
    // Clear the previous fruit
    M5Cardputer.Display.fillCircle(fruit->position.x, fruit->position.y, SNAKE_SIZE, TFT_BLACK);
    placeFruit(snake->position, &fruit->position);
    snake->fruitsEaten = snake->fruitsEaten + 1; // Increase counter of eaten fruits
    snake->eatFruit = true;
    // Check if speed threshold has been exceeded
    if (snake->fruitsEaten > game->highScore) {
      game->highScore = snake->fruitsEaten;
      saveHighScore(*game);
    }
    if (snake->fruitsEaten % SPEEDUP_THRESHOLD == 0) {
      snake->speedMultiplier += 1; // Increase speed multiplier
    }
  }
}

void checkCollision(std::vector<Position>& snake, GameState* game) {
  for (int i = 1; i < snake.size(); i++) {
    if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
      game->gameOver = true;
      game->isPaused = false; // Added to draw "GAME OVER" message only once
    }
  }
}

void playGameOverSound() {
  // Play three short sounds with increasing frequency
  M5Cardputer.Speaker.tone(2000, 100);
  delay(150);
  M5Cardputer.Speaker.tone(3000, 100);
  delay(150);
  M5Cardputer.Speaker.tone(4000, 100);
  delay(150);
}

void displayGameOver(int fruitsEaten, int highScore) {
  M5Cardputer.Display.clear();
  M5Cardputer.Display.setTextSize(2);

  M5Cardputer.Display.setTextColor(TFT_RED, TFT_BLACK);
  M5Cardputer.Display.setCursor(50, 40); // Moved up
  M5Cardputer.Display.print("GAME OVER");

  M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
  M5Cardputer.Display.setCursor(50, 70); // Moved up
  M5Cardputer.Display.print("n - new game");

  M5Cardputer.Display.setTextColor(TFT_CYAN, TFT_BLACK);
  M5Cardputer.Display.setCursor(50, 100); // Added message
  M5Cardputer.Display.print("p - pause");

  M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  printScoreBoard(fruitsEaten, highScore);
}

void drawStaticElements(int fruitsEaten, int highScore, const Position& fruit) {
  M5Cardputer.Display.setTextSize(2); // Set font size to 2
  printScoreBoard(fruitsEaten, highScore);
  M5Cardputer.Display.fillCircle(fruit.x, fruit.y, SNAKE_SIZE, TFT_RED);
}

void draw(Player* snake, int highScore) {
  // Update fruit count without clearing the entire TFT
  M5Cardputer.Display.setTextSize(2); // Set font size to 2
  if (snake->eatFruit || (snake->prevTail.x <= TFT_WIDTH && snake->prevTail.y <= 20)) {
    printScoreBoard(snake->fruitsEaten, highScore);
    snake->eatFruit = false;
  }

  // Redraw snake
  M5Cardputer.Display.fillRect(snake->prevTail.x, snake->prevTail.y, SNAKE_SIZE, SNAKE_SIZE, TFT_BLACK);
  for (const auto& p : snake->position) {
    M5Cardputer.Display.fillRect(p.x, p.y, SNAKE_SIZE, SNAKE_SIZE, TFT_GREEN);
  }
}

void printScoreBoard(int fruitsEaten, int highScore) {
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.print("Fruits ");
  M5Cardputer.Display.print(fruitsEaten);
  M5Cardputer.Display.print(" Record ");
  M5Cardputer.Display.println(highScore);
}

void placeFruit(const std::vector<Position>& snakePosition, Position* fruit) {
  bool safePlacement;
  do {
    safePlacement = true;
    fruit->x = random(0, TFT_WIDTH / SNAKE_SIZE) * SNAKE_SIZE;
    fruit->y = random(20 / SNAKE_SIZE, TFT_HEIGHT / SNAKE_SIZE) * SNAKE_SIZE; // Adjusted to start at y = 20
    // Check if fruit is not too close to snake
    for (const auto& p : snakePosition) {
      if (abs(p.x - fruit->x) < COLLISION_OFFSET && abs(p.y - fruit->y) < COLLISION_OFFSET) {
        safePlacement = false;
        break;
      }
    }
  } while (!safePlacement);

  // Draw the fruit only once after placing it
  M5Cardputer.Display.fillCircle(fruit->x, fruit->y, SNAKE_SIZE, TFT_RED);
}
