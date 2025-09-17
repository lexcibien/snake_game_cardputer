#include <M5Cardputer.h>
#include <vector>
#include <SD.h>

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 135
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
};

struct Fruit {
  Position position;
};

struct GameState {
  bool gameOver = false;
  bool isPaused = false;
  int highScore = 0;
  const char* recordFile = "/snake-record.txt";
};

void initSD(GameState* game);
void saveHighScore(GameState game);
void initGame(GameState* game);
void readButtons(SnakeDirection* direction, bool* isPaused);
void readPauseButton(bool* isPaused);
void readRestartButton(GameState* game);
void moveSnake(Player* snake, Fruit* fruit, GameState* game);
void checkCollision(std::vector<Position>& snake, GameState* game);
void playGameOverSound();
void displayGameOver(int fruitsEaten, int highScore);
void drawStaticElements(int fruitsEaten, int highScore, const Position& fruit);
void draw(const Player& snake, int highScore);
void placeFruit(const std::vector<Position>& snake, Position* fruit);

__attribute__((noreturn)) void setup() {
  GameState game;
  auto cfg = m5::M5Unified::config();

  M5Cardputer.begin(cfg);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5Cardputer.Display.fillScreen(TFT_BLACK);
  Serial.begin(115200);
  // initSD(&game);
  initGame(&game);
}

void initSD(GameState* game) {
  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    game->highScore = 0;
  } else {
    if (SD.exists(game->recordFile)) {
      File file = SD.open(game->recordFile);
      if (file) {
        game->highScore = file.parseInt();
        file.close();
      }
    } else {
      File file = SD.open(game->recordFile, FILE_WRITE);
      if (file) {
        file.println("0");
        file.close();
      }
      game->highScore = 0;
    }
  }
}

void saveHighScore(GameState game) {
  if (game.highScore > 0) {
    File file = SD.open(game.recordFile, FILE_WRITE);
    if (file) {
      file.println(game.highScore);
      file.close();
    }
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

  snake.direction = SnakeDirection::RIGHT; // Ustaw kierunek domyślny na prawo
  snake.fruitsEaten = 0; // Zresetuj licznik zjedzonych owoców
  snake.speedMultiplier = 0; // Zresetuj współczynnik przyspieszenia
  placeFruit(snake.position, &fruit.position); // Umieść owoc w losowym miejscu
  M5Cardputer.Display.clear();
  game->gameOver = false; // Reset stanu końca gry
  game->isPaused = false; // Reset stanu pauzy


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
          draw(snake, game->highScore);
          lastTime = currentTime;
        }
      } else {
        readPauseButton(&game->isPaused);
      }
    } else {
      if (!game->isPaused) { // Dodano, aby rysować napis "GAME OVER" tylko raz
        playGameOverSound();
        displayGameOver(snake.fruitsEaten, game->highScore);
        game->isPaused = true; // Zatrzymanie dalszego rysowania
      }
      readRestartButton(game);
    }
  }
}

void loop() { // Usado em initGame
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
    *direction = LEFT; // Left
  }
  if (M5Cardputer.Keyboard.isKeyPressed('/') && *direction != LEFT) {
    *direction = RIGHT; // Right
  }
  readPauseButton(isPaused);
}

void readPauseButton(bool* isPaused) {
  if (M5Cardputer.Keyboard.isKeyPressed('p')) { // Przycisk 'p' na klawiaturze
    *isPaused = !*isPaused;
    delay(200); // Opóźnienie, aby uniknąć podwójnego przełączania
  }
}

void readRestartButton(GameState* game) {
  if (M5Cardputer.Keyboard.isKeyPressed('n')) { // Przycisk 'n' na klawiaturze
    delay(200); // Opóźnienie, aby uniknąć podwójnego przełączania
    initGame(game); // Inicjalizuj grę od nowa
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
  if (next.x >= SCREEN_WIDTH) next.x = 0;
  if (next.x < 0) next.x = SCREEN_WIDTH - SNAKE_SIZE;
  if (next.y >= SCREEN_HEIGHT) next.y = 0;
  if (next.y < 0) next.y = SCREEN_HEIGHT - SNAKE_SIZE;

  // move tail to front
  snake->position.insert(snake->position.begin(), next);
  snake->position.pop_back();

  // Check if snake eats the fruit
  if (abs(snake->position[0].x - fruit->position.x) < COLLISION_OFFSET && abs(snake->position[0].y - fruit->position.y) < COLLISION_OFFSET) {
    for (int i = 0; i < GROWTH_INCREMENT; i++) {
      snake->position.push_back(snake->position.back()); // Zwiększ długość węża
    }
    // Play sound
    M5Cardputer.Speaker.tone(4000, 100); // Zagraj krótki dźwięk przy zjedzeniu owocu
    // Clear the previous fruit
    M5Cardputer.Display.fillCircle(fruit->position.x, fruit->position.y, SNAKE_SIZE, TFT_BLACK);
    placeFruit(snake->position, &fruit->position);
    snake->fruitsEaten = snake->fruitsEaten + 1; // Zwiększ licznik zjedzonych owoców
    // Sprawdź, czy przekroczono próg dla przyspieszenia
    if (snake->fruitsEaten > game->highScore) {
      game->highScore = snake->fruitsEaten;
      // saveHighScore(*game);
    }
    if (snake->fruitsEaten % SPEEDUP_THRESHOLD == 0) {
      snake->speedMultiplier += 1; // Zwiększ współczynnik przyspieszenia
    }
  }
}

void checkCollision(std::vector<Position>& snake, GameState* game) {
  for (int i = 1; i < snake.size(); i++) {
    if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
      game->gameOver = true;
      game->isPaused = false; // Umożliwia wyświetlenie napisu "GAME OVER" tylko raz
    }
  }
}

void playGameOverSound() {
  // Zagraj trzy krótkie dźwięki o rosnącej częstotliwości
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
  M5Cardputer.Display.setCursor(50, 40); // Przesunięto w górę
  M5Cardputer.Display.print("GAME OVER");

  M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
  M5Cardputer.Display.setCursor(50, 70); // Przesunięto w górę
  M5Cardputer.Display.print("n - new game");

  M5Cardputer.Display.setTextColor(TFT_CYAN, TFT_BLACK);
  M5Cardputer.Display.setCursor(50, 100); // Dodano napis
  M5Cardputer.Display.print("p - pause");

  M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.print("Fruits ");
  M5Cardputer.Display.print(fruitsEaten);
  M5Cardputer.Display.print(" Record ");
  M5Cardputer.Display.println(highScore);
}

void drawStaticElements(int fruitsEaten, int highScore, const Position& fruit) {
  M5Cardputer.Display.setTextSize(2); // Ustaw rozmiar czcionki na 2
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.print("Fruits ");
  M5Cardputer.Display.print(fruitsEaten);
  M5Cardputer.Display.print(" Record ");
  M5Cardputer.Display.println(highScore);
  M5Cardputer.Display.fillCircle(fruit.x, fruit.y, SNAKE_SIZE, TFT_RED);
}

void draw(const Player& snake, int highScore) {
  // Update fruit count without clearing the entire screen
  M5Cardputer.Display.fillRect(0, 0, SCREEN_WIDTH, 20, TFT_BLACK); // Zmniejszono wysokość obszaru czyszczenia
  M5Cardputer.Display.setTextSize(2); // Ustaw rozmiar czcionki na 2
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.print("Fruits ");
  M5Cardputer.Display.print(snake.fruitsEaten);
  M5Cardputer.Display.print(" Record ");
  M5Cardputer.Display.println(highScore);

  // Redraw snake
  M5Cardputer.Display.fillRect(snake.prevTail.x, snake.prevTail.y, SNAKE_SIZE, SNAKE_SIZE, TFT_BLACK);
  for (const auto& p : snake.position) {
    M5Cardputer.Display.fillRect(p.x, p.y, SNAKE_SIZE, SNAKE_SIZE, TFT_GREEN);
  }
}

void placeFruit(const std::vector<Position>& snake, Position* fruit) {
  bool safePlacement;
  do {
    safePlacement = true;
    fruit->x = random(0, SCREEN_WIDTH / SNAKE_SIZE) * SNAKE_SIZE;
    fruit->y = random(20 / SNAKE_SIZE, SCREEN_HEIGHT / SNAKE_SIZE) * SNAKE_SIZE; // Adjusted to start at y = 20
    // Sprawdź czy owoc nie jest zbyt blisko węża
    for (const auto& p : snake) {
      if (abs(p.x - fruit->x) < COLLISION_OFFSET && abs(p.y - fruit->y) < COLLISION_OFFSET) {
        safePlacement = false;
        break;
      }
    }
  } while (!safePlacement);

  // Draw the fruit only once after placing it
  M5Cardputer.Display.fillCircle(fruit->x, fruit->y, SNAKE_SIZE, TFT_RED);
}
