#include <SFML/Graphics.hpp>
#include <fstream>
#include <cstdlib>   
#include <ctime>    
using namespace std;

//  Game values Jo Hamesha Fix Rahain Gi

const int WINDOW_W = 600;
const int WINDOW_H = 700;
const int HUD_HEIGHT = 50;

const int BRICK_ROWS = 8;
const int BRICK_COLS = 10;
const int BRICK_W = 60;
const int BRICK_H = 25;
const int BRICKS_Y = 100;

const int PADDLE_Y = 650;
const int PADDLE_H = 14;
const int PADDLE_START_X = 250;
const int PADDLE_NORMAL_W = 100;
const int PADDLE_WIDE_W = 150;
const int PADDLE_SPEED = 8;

const int BALL_SIZE = 12;
const int BALL_ON_PADDLE_Y = 638;
const int BALL_SPEED_Y = 5;
const int BALL_LAUNCH_SPEED_X = 3;
const int TRAIL_LENGTH = 5;

const int START_LIVES = 3;
const int POINTS_HIT = 10;
const int POINTS_BREAK = 50;
const int MAX_LIVES_SHOWN = 8;

// power-ups (bonus)
const int POWERUP_SPEED = 3;
const int POWERUP_W = 36;
const int POWERUP_H = 18;
const int POWERUP_CHANCE = 5;      
const int WIDE_FRAMES = 600;       
const int WIDE_PADDLE = 0;
const int EXTRA_LIFE = 1;

// game states
const int MENU = 0;
const int PLAYING = 1;
const int GAME_OVER = 2;

// menu options
const int OPTION_NEW_GAME = 0;
const int OPTION_CONTINUE = 1;
const int OPTION_EXIT = 2;
const int OPTION_COUNT = 3;

const int STAR_COUNT = 60;


//  Structures

struct Paddle
{
    int x;
    int width;
};

struct Ball
{
    int x;            // top-left corner
    int y;
    int dx;           // pixels moved to the right every frame
    int dy;           // pixels moved down every frame
    bool launched;    // false while the ball is sitting on the paddle
    int trailX[TRAIL_LENGTH];   // old positions used for the glowing tail
    int trailY[TRAIL_LENGTH];
};

struct PowerUp
{
    bool active;
    int x;
    int y;
    int type;
};

// keys that were pressed in this frame
struct Input
{
    bool up;
    bool down;
    bool enter;
    bool space;
    bool escape;
    bool left;        
    bool right;      
};

struct Game
{
    int gameState;
    int menuChoice;
    bool noSavedGameMessage;
    bool levelErrorMessage;
    bool playerWon;
    bool quitGame;

    int bricks[BRICK_ROWS][BRICK_COLS];
    int score;
    int lives;

    Paddle paddle;
    Ball ball;
    PowerUp powerUp;
    int wideFramesLeft;

    int frameCount;
    Input input;

    int starX[STAR_COUNT];
    int starY[STAR_COUNT];
    int starSize[STAR_COUNT];
};

// The font is global so every drawing function can use it.
sf::Font gameFont;
bool fontLoaded = false;


//  Small helpers for building text

// Adds the text "source" at the end of "destination".
void addText(char* destination, const char* source)
{
    int end = 0;
    while (destination[end] != '\0')
    {
        end++;
    }
    int i = 0;
    while (source[i] != '\0')
    {
        destination[end + i] = source[i];
        i++;
    }
    destination[end + i] = '\0';
}

// Adds a number (as digits) at the end of "destination".
void addNumber(char* destination, int number)
{
    char digits[12];
    int digitCount = 0;
    bool negative = false;

    if (number < 0)
    {
        negative = true;
        number = -number;
    }
    if (number == 0)
    {
        digits[digitCount] = '0';
        digitCount++;
    }
    // the last digit comes out first, so we reverse them afterwards
    while (number > 0)
    {
        digits[digitCount] = (char)('0' + number % 10);
        digitCount++;
        number = number / 10;
    }

    int end = 0;
    while (destination[end] != '\0')
    {
        end++;
    }
    if (negative)
    {
        destination[end] = '-';
        end++;
    }
    while (digitCount > 0)
    {
        digitCount--;
        destination[end] = digits[digitCount];
        end++;
    }
    destination[end] = '\0';
}


//  Small drawing helpers


void loadFont()
{
    // The game looks for a font next to the program first, then in the
    // normal font folders of Windows / Linux / Mac.
    const char* fontFiles[8] = {
        "font.ttf",
        "C:/Windows/Fonts/consolab.ttf",
        "C:/Windows/Fonts/arialbd.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
        "/Library/Fonts/Arial.ttf"
    };

    for (int i = 0; i < 8; i++)
    {
        if (gameFont.openFromFile(fontFiles[i]))
        {
            fontLoaded = true;
            return;
        }
    }
}

void drawText(sf::RenderWindow& window, const char* message, int size,
    float x, float y, sf::Color color, bool centered)
{
    if (!fontLoaded)
    {
        return;
    }
    sf::Text text(gameFont, message, (unsigned int)size);
    text.setFillColor(color);
    if (centered)
    {
        // move the origin to the middle of the text so x is its centre
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin({ bounds.position.x + bounds.size.x / 2.f, 0.f });
    }
    text.setPosition({ x, y });
    window.draw(text);
}

void drawBox(sf::RenderWindow& window, float x, float y, float w, float h, sf::Color color)
{
    sf::RectangleShape box({ w, h });
    box.setPosition({ x, y });
    box.setFillColor(color);
    window.draw(box);
}

void drawCircle(sf::RenderWindow& window, float centerX, float centerY, float radius, sf::Color color)
{
    sf::CircleShape circle(radius);
    circle.setPosition({ centerX - radius, centerY - radius });
    circle.setFillColor(color);
    window.draw(circle);
}

// A rectangle with round corners: two rectangles that cross each other
// plus a circle in every corner.
void drawRoundBox(sf::RenderWindow& window, float x, float y, float w, float h,
    float radius, sf::Color color)
{
    drawBox(window, x + radius, y, w - 2 * radius, h, color);
    drawBox(window, x, y + radius, w, h - 2 * radius, color);
    drawCircle(window, x + radius, y + radius, radius, color);
    drawCircle(window, x + w - radius, y + radius, radius, color);
    drawCircle(window, x + radius, y + h - radius, radius, color);
    drawCircle(window, x + w - radius, y + h - radius, radius, color);
}

// Colour of a brick, depending on how many hits it has left.
sf::Color getBrickColor(int hitsLeft)
{
    if (hitsLeft == 1)
    {
        return sf::Color(61, 220, 151);    // green
    }
    if (hitsLeft == 2)
    {
        return sf::Color(255, 209, 60);    // yellow
    }
    if (hitsLeft == 3)
    {
        return sf::Color(255, 77, 109);    // red
    }
    return sf::Color(25, 25, 45);          // no brick
}


//  Setting up a game


void makeStars(Game& game)
{
    for (int i = 0; i < STAR_COUNT; i++)
    {
        game.starX[i] = rand() % WINDOW_W;
        game.starY[i] = HUD_HEIGHT + rand() % (WINDOW_H - HUD_HEIGHT);
        game.starSize[i] = 1 + rand() % 2;
    }
}

// Puts the ball on the middle of the paddle.
void putBallOnPaddle(Game& game)
{
    game.ball.launched = false;
    game.ball.x = game.paddle.x + game.paddle.width / 2 - BALL_SIZE / 2;   // paddleX + 44
    game.ball.y = BALL_ON_PADDLE_Y;
    game.ball.dx = 0;
    game.ball.dy = 0;
    for (int i = 0; i < TRAIL_LENGTH; i++)
    {
        game.ball.trailX[i] = game.ball.x;
        game.ball.trailY[i] = game.ball.y;
    }
}

void resetPaddle(Game& game)
{
    game.paddle.x = PADDLE_START_X;
    game.paddle.width = PADDLE_NORMAL_W;
    game.wideFramesLeft = 0;
    game.powerUp.active = false;
}

// Gives the game its values the very first time the program starts.
void setupGame(Game& game)
{
    game.gameState = MENU;
    game.menuChoice = 0;
    game.noSavedGameMessage = false;
    game.levelErrorMessage = false;
    game.playerWon = false;
    game.quitGame = false;

    game.score = 0;
    game.lives = START_LIVES;
    for (int r = 0; r < BRICK_ROWS; r++)
    {
        for (int c = 0; c < BRICK_COLS; c++)
        {
            game.bricks[r][c] = 0;
        }
    }

    game.frameCount = 0;
    game.input.up = false;
    game.input.down = false;
    game.input.enter = false;
    game.input.space = false;
    game.input.escape = false;
    game.input.left = false;
    game.input.right = false;

    resetPaddle(game);
    putBallOnPaddle(game);
    makeStars(game);
}


//  Files: level, save game

// Reads level.txt into the bricks array. The first line has the rows and
// columns, then comes one number for every brick.
// Returns false if the file is missing or wrong.
bool loadLevel(Game& game)
{
    ifstream fin("level.txt");
    if (!fin)
    {
        return false;
    }

    int rows = 0;
    int cols = 0;
    fin >> rows >> cols;
    if (!fin || rows != BRICK_ROWS || cols != BRICK_COLS)
    {
        return false;
    }

    for (int r = 0; r < BRICK_ROWS; r++)
    {
        for (int c = 0; c < BRICK_COLS; c++)
        {
            fin >> game.bricks[r][c];
            if (!fin)
            {
                return false;
            }
            // only 0, 1, 2 and 3 are allowed
            if (game.bricks[r][c] < 0 || game.bricks[r][c] > 3)
            {
                game.bricks[r][c] = 0;
            }
        }
    }
    fin.close();
    return true;
}

// Saves score, lives and the bricks as they are now.
// Line 1: score lives     then 8 lines of 10 numbers.
void saveGame(Game& game)
{
    ofstream fout("savegame.txt");
    fout << game.score << " " << game.lives << "\n";
    for (int r = 0; r < BRICK_ROWS; r++)
    {
        for (int c = 0; c < BRICK_COLS; c++)
        {
            fout << game.bricks[r][c];
            if (c < BRICK_COLS - 1)
            {
                fout << " ";
            }
        }
        fout << "\n";
    }
    fout.close();
}

// Makes savegame.txt empty so a finished game cannot be continued.
void emptySaveFile()
{
    ofstream fout("savegame.txt");
    fout.close();
}

// Reads savegame.txt. The numbers are first read into a temporary array, so
// a broken or empty file never damages the running game.
// Returns false if there is nothing to continue.
bool loadSavedGame(Game& game)
{
    ifstream fin("savegame.txt");
    if (!fin)
    {
        return false;
    }

    int savedScore = 0;
    int savedLives = 0;
    fin >> savedScore >> savedLives;
    if (!fin || savedLives <= 0)
    {
        return false;
    }

    int tempBricks[BRICK_ROWS][BRICK_COLS];
    int bricksLeft = 0;
    for (int r = 0; r < BRICK_ROWS; r++)
    {
        for (int c = 0; c < BRICK_COLS; c++)
        {
            fin >> tempBricks[r][c];
            if (!fin || tempBricks[r][c] < 0 || tempBricks[r][c] > 3)
            {
                return false;
            }
            if (tempBricks[r][c] > 0)
            {
                bricksLeft++;
            }
        }
    }
    fin.close();
    if (bricksLeft == 0)
    {
        return false;
    }

    // everything is fine, copy it into the game
    game.score = savedScore;
    game.lives = savedLives;
    for (int r = 0; r < BRICK_ROWS; r++)
    {
        for (int c = 0; c < BRICK_COLS; c++)
        {
            game.bricks[r][c] = tempBricks[r][c];
        }
    }
    return true;
}


//  Starting and ending a game


void startNewGame(Game& game)
{
    if (!loadLevel(game))
    {
        game.levelErrorMessage = true;     // the menu will show an error
        return;
    }
    game.score = 0;
    game.lives = START_LIVES;
    resetPaddle(game);
    putBallOnPaddle(game);
    game.noSavedGameMessage = false;
    game.levelErrorMessage = false;
    game.gameState = PLAYING;
}

void continueGame(Game& game)
{
    if (!loadSavedGame(game))
    {
        game.noSavedGameMessage = true;    // the menu will show "No saved game"
        return;
    }
    resetPaddle(game);                     // paddle in the middle, ball waits on it
    putBallOnPaddle(game);
    game.noSavedGameMessage = false;
    game.levelErrorMessage = false;
    game.gameState = PLAYING;
}

void endGame(Game& game, bool won)
{
    game.playerWon = won;
    game.gameState = GAME_OVER;
    emptySaveFile();                       // a finished game cannot be continued
}

int countBricksLeft(Game& game)
{
    int count = 0;
    for (int r = 0; r < BRICK_ROWS; r++)
    {
        for (int c = 0; c < BRICK_COLS; c++)
        {
            if (game.bricks[r][c] > 0)
            {
                count++;
            }
        }
    }
    return count;
}


//  Paddle

// Keeps the paddle inside the window.
void keepPaddleInside(Game& game)
{
    if (game.paddle.x < 0)
    {
        game.paddle.x = 0;
    }
    if (game.paddle.x + game.paddle.width > WINDOW_W)
    {
        game.paddle.x = WINDOW_W - game.paddle.width;
    }
}

void movePaddle(Game& game)
{
    if (game.input.left)
    {
        game.paddle.x = game.paddle.x - PADDLE_SPEED;
    }
    if (game.input.right)
    {
        game.paddle.x = game.paddle.x + PADDLE_SPEED;
    }
    keepPaddleInside(game);
}


//  Ball

void launchBall(Game& game)
{
    game.ball.launched = true;
    game.ball.dy = -BALL_SPEED_Y;
    if (rand() % 2 == 0)
    {
        game.ball.dx = BALL_LAUNCH_SPEED_X;
    }
    else
    {
        game.ball.dx = -BALL_LAUNCH_SPEED_X;
    }
}

// Two boxes touch when each one starts before the other one ends,
// both left-right and up-down.
bool ballTouchesPaddle(Game& game)
{
    bool touchesLeftRight = game.ball.x < game.paddle.x + game.paddle.width &&
        game.ball.x + BALL_SIZE > game.paddle.x;
    bool touchesUpDown = game.ball.y < PADDLE_Y + PADDLE_H &&
        game.ball.y + BALL_SIZE > PADDLE_Y;
    return touchesLeftRight && touchesUpDown;
}

// The paddle is split into 4 equal parts. The part under the middle of the
// ball decides the new dx, so the player can aim.
void bounceOnPaddle(Game& game)
{
    int newDx[4] = { -6, -3, 3, 6 };

    game.ball.y = BALL_ON_PADDLE_Y;        // put the ball just on top of the paddle
    game.ball.dy = -BALL_SPEED_Y;

    int part = (game.ball.x + BALL_SIZE / 2 - game.paddle.x) * 4 / game.paddle.width;
    if (part < 0)
    {
        part = 0;
    }
    if (part > 3)
    {
        part = 3;
    }
    game.ball.dx = newDx[part];
}


//  Power-ups (bonus)


// Called when a brick breaks: maybe a power-up starts to fall from it.
void maybeDropPowerUp(Game& game, int row, int col)
{
    if (game.powerUp.active)
    {
        return;                            // only one power-up at a time
    }
    if (rand() % POWERUP_CHANCE != 0)
    {
        return;
    }
    game.powerUp.active = true;
    game.powerUp.x = col * BRICK_W + (BRICK_W - POWERUP_W) / 2;
    game.powerUp.y = BRICKS_Y + row * BRICK_H;
    game.powerUp.type = rand() % 2;
}

void applyPowerUp(Game& game)
{
    if (game.powerUp.type == WIDE_PADDLE)
    {
        if (game.paddle.width == PADDLE_NORMAL_W)
        {
            // grow from the middle so the paddle does not jump to one side
            game.paddle.x = game.paddle.x - (PADDLE_WIDE_W - PADDLE_NORMAL_W) / 2;
            game.paddle.width = PADDLE_WIDE_W;
            keepPaddleInside(game);
        }
        game.wideFramesLeft = WIDE_FRAMES;  // catching another W restarts the timer
    }
    else
    {
        game.lives++;
    }
}

void updatePowerUp(Game& game)
{
    if (!game.powerUp.active)
    {
        return;
    }
    game.powerUp.y = game.powerUp.y + POWERUP_SPEED;

    bool touchesLeftRight = game.powerUp.x < game.paddle.x + game.paddle.width &&
        game.powerUp.x + POWERUP_W > game.paddle.x;
    bool touchesUpDown = game.powerUp.y < PADDLE_Y + PADDLE_H &&
        game.powerUp.y + POWERUP_H > PADDLE_Y;

    if (touchesLeftRight && touchesUpDown)
    {
        applyPowerUp(game);
        game.powerUp.active = false;
    }
    else if (game.powerUp.y > WINDOW_H)
    {
        game.powerUp.active = false;       // missed, it is gone
    }
}

// Counts down the wide paddle and shrinks it back after 10 seconds.
void updateWideTimer(Game& game)
{
    if (game.wideFramesLeft > 0)
    {
        game.wideFramesLeft--;
        if (game.wideFramesLeft == 0)
        {
            game.paddle.x = game.paddle.x + (PADDLE_WIDE_W - PADDLE_NORMAL_W) / 2;
            game.paddle.width = PADDLE_NORMAL_W;
            keepPaddleInside(game);
        }
    }
}


//  Bricks


// Takes one hit off a brick and gives points.
void damageBrick(Game& game, int row, int col)
{
    game.bricks[row][col]--;
    if (game.bricks[row][col] > 0)
    {
        game.score = game.score + POINTS_HIT;
    }
    else
    {
        game.score = game.score + POINTS_BREAK;   // this hit broke the brick
        maybeDropPowerUp(game, row, col);
    }
}

// Finds the bricks the ball is touching and damages them.
// Each of the 4 corners of the ball is a point. The brick under a point
// is found with one division:
//      row = (y - 100) / 25         column = x / 60
// Returns true if the ball touched at least one brick.
bool hitBricks(Game& game)
{
    int cornerX[4] = { game.ball.x, game.ball.x + BALL_SIZE - 1,
                       game.ball.x, game.ball.x + BALL_SIZE - 1 };
    int cornerY[4] = { game.ball.y, game.ball.y,
                       game.ball.y + BALL_SIZE - 1, game.ball.y + BALL_SIZE - 1 };

    int hitRow[4];
    int hitCol[4];
    int hitCount = 0;

    for (int i = 0; i < 4; i++)
    {
        int cx = cornerX[i];
        int cy = cornerY[i];

        // only use the corner if it is inside the brick area,
        // otherwise the division gives a row or column outside the array
        if (cy < BRICKS_Y || cy >= BRICKS_Y + BRICK_ROWS * BRICK_H || cx < 0 || cx >= WINDOW_W)
        {
            continue;
        }

        int row = (cy - BRICKS_Y) / BRICK_H;
        int col = cx / BRICK_W;
        if (game.bricks[row][col] == 0)
        {
            continue;                      // no brick here
        }

        // if two corners are on the same brick, it must lose only one hit
        bool alreadyFound = false;
        for (int j = 0; j < hitCount; j++)
        {
            if (hitRow[j] == row && hitCol[j] == col)
            {
                alreadyFound = true;
            }
        }
        if (!alreadyFound)
        {
            hitRow[hitCount] = row;
            hitCol[hitCount] = col;
            hitCount++;
        }
    }

    // damage the bricks only after all corners are checked
    for (int i = 0; i < hitCount; i++)
    {
        damageBrick(game, hitRow[i], hitCol[i]);
    }
    return hitCount > 0;
}


//  Moving the ball every frame

void loseLife(Game& game)
{
    game.lives--;
    if (game.lives <= 0)
    {
        game.lives = 0;
        endGame(game, false);
    }
    else
    {
        putBallOnPaddle(game);             // the bricks stay as they are
    }
}

// The ball moves in two steps: first x, then y. After every step we check
// the walls and the bricks. If it hit something, we undo the step and
// reverse that speed.
void moveBall(Game& game)
{
    // remember the old positions for the tail
    for (int i = TRAIL_LENGTH - 1; i > 0; i--)
    {
        game.ball.trailX[i] = game.ball.trailX[i - 1];
        game.ball.trailY[i] = game.ball.trailY[i - 1];
    }
    game.ball.trailX[0] = game.ball.x;
    game.ball.trailY[0] = game.ball.y;

    //step 1: move in x
    game.ball.x = game.ball.x + game.ball.dx;
    if (game.ball.x < 0 || game.ball.x + BALL_SIZE > WINDOW_W)
    {
        game.ball.x = game.ball.x - game.ball.dx;      // undo
        game.ball.dx = -game.ball.dx;
    }
    else if (hitBricks(game))
    {
        game.ball.x = game.ball.x - game.ball.dx;      // undo
        game.ball.dx = -game.ball.dx;
    }

    //step 2: move in y
    game.ball.y = game.ball.y + game.ball.dy;
    if (game.ball.y < HUD_HEIGHT)
    {
        game.ball.y = game.ball.y - game.ball.dy;      // undo
        game.ball.dy = -game.ball.dy;
    }
    else if (hitBricks(game))
    {
        game.ball.y = game.ball.y - game.ball.dy;      // undo
        game.ball.dy = -game.ball.dy;
    }
    else if (game.ball.dy > 0 && ballTouchesPaddle(game))
    {
        bounceOnPaddle(game);              // only works while the ball moves down
    }

    // below the bottom of the window: the ball is lost
    if (game.ball.y > WINDOW_H)
    {
        loseLife(game);
    }
}

//  INPUT

void handleInput(sf::RenderWindow& window, Game& game)
{
    // keys that work once per press are cleared every frame
    game.input.up = false;
    game.input.down = false;
    game.input.enter = false;
    game.input.space = false;
    game.input.escape = false;

    while (const std::optional event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            window.close();
        }
        else if (const auto* key = event->getIf<sf::Event::KeyPressed>())
        {
            if (key->code == sf::Keyboard::Key::Up || key->code == sf::Keyboard::Key::W)
            {
                game.input.up = true;
            }
            if (key->code == sf::Keyboard::Key::Down || key->code == sf::Keyboard::Key::S)
            {
                game.input.down = true;
            }
            if (key->code == sf::Keyboard::Key::Enter)
            {
                game.input.enter = true;
            }
            if (key->code == sf::Keyboard::Key::Space)
            {
                game.input.space = true;
            }
            if (key->code == sf::Keyboard::Key::Escape)
            {
                game.input.escape = true;
            }
        }
    }

    // keys that are checked every frame (the paddle moves while held down)
    game.input.left = false;
    game.input.right = false;
    if (window.hasFocus())
    {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
        {
            game.input.left = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        {
            game.input.right = true;
        }
    }
}


//  UPDATE


void updateMenu(Game& game)
{
    if (game.input.up)
    {
        game.menuChoice--;
        if (game.menuChoice < 0)
        {
            game.menuChoice = OPTION_COUNT - 1;
        }
        game.noSavedGameMessage = false;
        game.levelErrorMessage = false;
    }
    if (game.input.down)
    {
        game.menuChoice++;
        if (game.menuChoice >= OPTION_COUNT)
        {
            game.menuChoice = 0;
        }
        game.noSavedGameMessage = false;
        game.levelErrorMessage = false;
    }

    if (game.input.enter)
    {
        switch (game.menuChoice)
        {
        case OPTION_NEW_GAME:
            startNewGame(game);
            break;
        case OPTION_CONTINUE:
            continueGame(game);
            break;
        case OPTION_EXIT:
            game.quitGame = true;
            break;
        }
    }
}

void updatePlaying(Game& game)
{
    // Esc: save the game and go back to the menu
    if (game.input.escape)
    {
        saveGame(game);
        game.menuChoice = 0;
        game.gameState = MENU;
        return;
    }

    movePaddle(game);

    if (!game.ball.launched)
    {
        // the ball sits on the paddle and moves with it until Space
        putBallOnPaddle(game);
        if (game.input.space)
        {
            launchBall(game);
        }
    }
    else
    {
        moveBall(game);
    }

    // moveBall can end the game (no lives left), so check first
    if (game.gameState != PLAYING)
    {
        return;
    }

    updatePowerUp(game);
    updateWideTimer(game);

    if (countBricksLeft(game) == 0)
    {
        endGame(game, true);
    }
}

void updateGameOver(Game& game)
{
    if (game.input.enter)
    {
        game.menuChoice = 0;
        game.gameState = MENU;
    }
}

void updateGame(Game& game)
{
    game.frameCount++;

    switch (game.gameState)
    {
    case MENU:
        updateMenu(game);
        break;
    case PLAYING:
        updatePlaying(game);
        break;
    case GAME_OVER:
        updateGameOver(game);
        break;
    }
}

// ============================================================================
//  DRAW
// ============================================================================

// Slowly twinkling stars in the background.
void drawStars(sf::RenderWindow& window, Game& game)
{
    for (int i = 0; i < STAR_COUNT; i++)
    {
        int t = (game.frameCount + i * 17) % 100;
        if (t > 50)
        {
            t = 100 - t;
        }
        int brightness = 40 + t * 2;
        drawBox(window, (float)game.starX[i], (float)game.starY[i],
            (float)game.starSize[i], (float)game.starSize[i],
            sf::Color(170, 190, 255, (unsigned char)brightness));
    }
}

void drawBackground(sf::RenderWindow& window, Game& game)
{
    window.clear(sf::Color(14, 17, 42));
    drawStars(window, game);
    // thin glowing walls on the left and right
    drawBox(window, 0.f, (float)HUD_HEIGHT, 3.f, (float)(WINDOW_H - HUD_HEIGHT), sf::Color(45, 55, 125));
    drawBox(window, (float)(WINDOW_W - 3), (float)HUD_HEIGHT, 3.f, (float)(WINDOW_H - HUD_HEIGHT), sf::Color(45, 55, 125));
}

void drawHud(sf::RenderWindow& window, Game& game)
{
    drawBox(window, 0.f, 0.f, (float)WINDOW_W, (float)HUD_HEIGHT, sf::Color(22, 26, 62));
    drawBox(window, 0.f, (float)(HUD_HEIGHT - 2), (float)WINDOW_W, 2.f, sf::Color(95, 105, 200));

    // score
    char scoreText[40];
    scoreText[0] = '\0';
    addText(scoreText, "SCORE  ");
    addNumber(scoreText, game.score);
    drawText(window, scoreText, 22, 16.f, 10.f, sf::Color(240, 244, 255), false);

    // wide paddle timer (bonus)
    if (game.wideFramesLeft > 0)
    {
        drawText(window, "WIDE", 12, 215.f, 8.f, sf::Color(60, 220, 255), false);
        drawBox(window, 215.f, 30.f, 100.f, 5.f, sf::Color(35, 45, 100));
        drawBox(window, 215.f, 30.f, 100.f * game.wideFramesLeft / WIDE_FRAMES, 5.f, sf::Color(60, 220, 255));
    }

    // lives as small circles
    drawText(window, "LIVES", 14, 345.f, 16.f, sf::Color(150, 160, 225), false);
    for (int i = 0; i < game.lives && i < MAX_LIVES_SHOWN; i++)
    {
        drawCircle(window, 425.f + i * 20.f, 26.f, 8.f, sf::Color(255, 255, 255));
        drawCircle(window, 425.f + i * 20.f, 26.f, 6.f, sf::Color(255, 77, 109));
    }
}

void drawBricks(sf::RenderWindow& window, Game& game)
{
    for (int r = 0; r < BRICK_ROWS; r++)
    {
        for (int c = 0; c < BRICK_COLS; c++)
        {
            if (game.bricks[r][c] == 0)
            {
                continue;                  // no brick, draw nothing
            }
            float x = (float)(c * BRICK_W + 1);
            float y = (float)(BRICKS_Y + r * BRICK_H + 1);
            float w = (float)(BRICK_W - 2);
            float h = (float)(BRICK_H - 2);

            // white border, then the coloured body, then a small shine
            drawRoundBox(window, x, y, w, h, 6.f, sf::Color(240, 245, 255));
            drawRoundBox(window, x + 1, y + 1, w - 2, h - 2, 5.f, getBrickColor(game.bricks[r][c]));
            drawBox(window, x + 7, y + 5, 12.f, 3.f, sf::Color(255, 255, 255, 110));
        }
    }
}

void drawPaddleShape(sf::RenderWindow& window, float x, float width)
{
    drawRoundBox(window, x - 1, (float)PADDLE_Y - 1, width + 2, (float)PADDLE_H + 2, 8.f, sf::Color(240, 250, 255));
    drawRoundBox(window, x, (float)PADDLE_Y, width, (float)PADDLE_H, 7.f, sf::Color(60, 220, 255));
    drawBox(window, x + 8, (float)PADDLE_Y + 3, width - 16, 3.f, sf::Color(190, 245, 255));
}

void drawPaddle(sf::RenderWindow& window, Game& game)
{
    drawPaddleShape(window, (float)game.paddle.x, (float)game.paddle.width);
}

void drawBall(sf::RenderWindow& window, Game& game)
{
    float centerX = game.ball.x + BALL_SIZE / 2.f;
    float centerY = game.ball.y + BALL_SIZE / 2.f;

    // glowing tail, only while the ball is flying
    if (game.ball.launched)
    {
        for (int i = TRAIL_LENGTH - 1; i >= 0; i--)
        {
            float radius = 5.f - i * 0.7f;
            int alpha = 110 - i * 20;
            drawCircle(window, game.ball.trailX[i] + BALL_SIZE / 2.f,
                game.ball.trailY[i] + BALL_SIZE / 2.f, radius,
                sf::Color(170, 190, 255, (unsigned char)alpha));
        }
    }
    drawCircle(window, centerX, centerY, 12.f, sf::Color(150, 180, 255, 45));   // glow
    drawCircle(window, centerX, centerY, 6.f, sf::Color(255, 255, 255));
}

void drawPowerUp(sf::RenderWindow& window, Game& game)
{
    if (!game.powerUp.active)
    {
        return;
    }
    float x = (float)game.powerUp.x;
    float y = (float)game.powerUp.y;

    sf::Color color = sf::Color(60, 220, 255);          // W is cyan
    const char* letter = "W";
    if (game.powerUp.type == EXTRA_LIFE)
    {
        color = sf::Color(255, 120, 160);               // + is pink
        letter = "+";
    }
    drawRoundBox(window, x - 1, y - 1, POWERUP_W + 2.f, POWERUP_H + 2.f, 10.f, sf::Color(255, 255, 255));
    drawRoundBox(window, x, y, (float)POWERUP_W, (float)POWERUP_H, 9.f, color);
    drawText(window, letter, 14, x + POWERUP_W / 2.f, y, sf::Color(14, 17, 42), true);
}

// Everything you see while playing (also used behind the game over screen).
void drawPlayScene(sf::RenderWindow& window, Game& game)
{
    drawBackground(window, game);
    drawBricks(window, game);
    drawPowerUp(window, game);
    drawPaddle(window, game);
    drawBall(window, game);
    drawHud(window, game);

    if (game.gameState == PLAYING && !game.ball.launched)
    {
        // blinking hint
        if ((game.frameCount / 30) % 2 == 0)
        {
            drawText(window, "PRESS SPACE TO LAUNCH", 18, WINDOW_W / 2.f, 560.f,
                sf::Color(150, 165, 235), true);
        }
    }
}

void drawMenu(sf::RenderWindow& window, Game& game)
{
    drawBackground(window, game);

    // title
    drawText(window, "BLOCK", 72, WINDOW_W / 2.f, 70.f, sf::Color(60, 220, 255), true);
    drawText(window, "BUSTER", 72, WINDOW_W / 2.f, 140.f, sf::Color(255, 77, 109), true);

    // colour strip under the title
    drawBox(window, 90.f, 240.f, 105.f, 5.f, sf::Color(255, 77, 109));
    drawBox(window, 195.f, 240.f, 105.f, 5.f, sf::Color(255, 209, 60));
    drawBox(window, 300.f, 240.f, 105.f, 5.f, sf::Color(60, 220, 255));
    drawBox(window, 405.f, 240.f, 105.f, 5.f, sf::Color(180, 120, 255));
    drawText(window, "A BRICK BREAKER GAME", 15, WINDOW_W / 2.f, 258.f, sf::Color(130, 140, 205), true);

    //three menu buttons
    const char* names[OPTION_COUNT] = { "NEW GAME", "CONTINUE", "EXIT" };
    for (int i = 0; i < OPTION_COUNT; i++)
    {
        float x = 160.f;
        float y = 320.f + i * 64.f;
        if (i == game.menuChoice)
        {
            drawRoundBox(window, x - 2, y - 2, 284.f, 52.f, 12.f, sf::Color(60, 220, 255));
            drawRoundBox(window, x, y, 280.f, 48.f, 10.f, sf::Color(30, 62, 120));
            drawText(window, names[i], 24, WINDOW_W / 2.f, y + 9.f, sf::Color(255, 255, 255), true);
        }
        else
        {
            drawRoundBox(window, x - 2, y - 2, 284.f, 52.f, 12.f, sf::Color(70, 80, 150));
            drawRoundBox(window, x, y, 280.f, 48.f, 10.f, sf::Color(22, 26, 62));
            drawText(window, names[i], 24, WINDOW_W / 2.f, y + 9.f, sf::Color(150, 160, 225), true);
        }
    }

    // messages
    if (game.noSavedGameMessage)
    {
        drawText(window, "No saved game", 20, WINDOW_W / 2.f, 520.f, sf::Color(255, 77, 109), true);
    }
    if (game.levelErrorMessage)
    {
        drawText(window, "Error: level.txt was not found", 20, WINDOW_W / 2.f, 520.f, sf::Color(255, 77, 109), true);
    }

    drawText(window, "UP / DOWN  choose        ENTER  select", 14, WINDOW_W / 2.f, 570.f,
        sf::Color(110, 120, 190), true);
}

void drawGameOver(sf::RenderWindow& window, Game& game)
{
    drawPlayScene(window, game);

    // dark layer over the game and a panel in the middle
    drawBox(window, 0.f, 0.f, (float)WINDOW_W, (float)WINDOW_H, sf::Color(8, 10, 28, 190));

    sf::Color mainColor = sf::Color(255, 77, 109);
    const char* title = "GAME OVER";
    if (game.playerWon)
    {
        mainColor = sf::Color(61, 220, 151);
        title = "YOU WIN";
    }

    drawRoundBox(window, 90.f, 230.f, 420.f, 250.f, 18.f, mainColor);
    drawRoundBox(window, 93.f, 233.f, 414.f, 244.f, 16.f, sf::Color(22, 26, 62));

    drawText(window, title, 60, WINDOW_W / 2.f, 255.f, mainColor, true);

    char scoreText[40];
    scoreText[0] = '\0';
    addText(scoreText, "FINAL SCORE  ");
    addNumber(scoreText, game.score);
    drawText(window, scoreText, 26, WINDOW_W / 2.f, 345.f, sf::Color(255, 209, 60), true);

    if ((game.frameCount / 30) % 2 == 0)
    {
        drawText(window, "Press Enter", 22, WINDOW_W / 2.f, 415.f, sf::Color(220, 228, 255), true);
    }
}

void drawGame(sf::RenderWindow& window, Game& game)
{
    switch (game.gameState)
    {
    case MENU:
        drawMenu(window, game);
        break;
    case PLAYING:
        drawPlayScene(window, game);
        break;
    case GAME_OVER:
        drawGameOver(window, game);
        break;
    }
    window.display();
}

//  MAIN

int main()
{
    srand((unsigned int)time(0));

    sf::RenderWindow window(sf::VideoMode({ 600, 700 }), "Block Buster");
    window.setFramerateLimit(60);          // same game speed on every laptop
    window.setKeyRepeatEnabled(false);     // a key press counts only once

    loadFont();

    Game game;
    setupGame(game);

    while (window.isOpen())
    {
        handleInput(window, game);
        if (!window.isOpen())
        {
            break;                         // the X button was pressed
        }
        updateGame(game);
        if (game.quitGame)
        {
            window.close();
            break;
        }
        drawGame(window, game);
    }

    return 0;
}