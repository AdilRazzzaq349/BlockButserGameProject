#include <SFML/Graphics.hpp>
#include <fstream>
#include <cstdlib>
#include <ctime>
using namespace std;

// ============================================================
//  Game ki fixed values (yeh program chalte waqt change nahi hoti)
// ============================================================

// window ka size
const int WINDOW_W = 600;
const int WINDOW_H = 700;
const int HUD_HEIGHT = 50;          // upar wali score/lives wali patti

// bricks ki grid
const int BRICK_ROWS = 8;
const int BRICK_COLS = 10;
const int BRICK_W = 60;
const int BRICK_H = 25;
const int BRICKS_Y = 100;           // bricks yahan se shuru hoti hain (upar se)

// paddle
const int PADDLE_Y = 650;
const int PADDLE_H = 14;
const int PADDLE_START_X = 250;
const int PADDLE_NORMAL_W = 100;
const int PADDLE_WIDE_W = 150;
const int PADDLE_SPEED = 8;

// ball
const int BALL_SIZE = 12;
const int BALL_ON_PADDLE_Y = 638;   // ball paddle ke bilkul upar yahan baithti hai
const int BALL_SPEED_Y = 5;
const int BALL_LAUNCH_SPEED_X = 3;
const int TRAIL_LENGTH = 5;         // ball ki dum (tail) kitni lambi ho

// score aur lives
const int START_LIVES = 3;
const int POINTS_HIT = 10;          // brick ko hit kiya
const int POINTS_BREAK = 50;        // brick toot gayi
const int MAX_LIVES_SHOWN = 8;      // HUD mein itne se zyada circles nahi dikhayenge

// power-ups (bonus)
const int POWERUP_SPEED = 3;
const int POWERUP_W = 36;
const int POWERUP_H = 18;
const int POWERUP_CHANCE = 5;       // 1 / 5 chance brick tootne par
const int WIDE_FRAMES = 600;        // 60 fps par 600 frames = 10 second
const int WIDE_PADDLE = 0;          // power-up type: paddle chaura
const int EXTRA_LIFE = 1;           // power-up type: ek extra life

// game ki states
const int MENU = 0;
const int PLAYING = 1;
const int GAME_OVER = 2;

// menu ke options
const int OPTION_NEW_GAME = 0;
const int OPTION_CONTINUE = 1;
const int OPTION_EXIT = 2;
const int OPTION_COUNT = 3;

const int STAR_COUNT = 60;


// ============================================================
//  Structures
// ============================================================

struct Paddle
{
    int x;
    int width;
};

struct Ball
{
    int x;                      // top-left corner
    int y;
    int dx;                     // har frame mein kitne pixel right ki taraf
    int dy;                     // har frame mein kitne pixel neeche ki taraf
    bool launched;              // false = ball abhi paddle par rakhi hai
    int trailX[TRAIL_LENGTH];   // purani positions, tail banane ke liye
    int trailY[TRAIL_LENGTH];
};

struct PowerUp
{
    bool active;                // true = abhi screen par gir raha hai
    int x;
    int y;
    int type;
};

// Is frame mein kaun si keys dabi hain
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

    int bricks[BRICK_ROWS][BRICK_COLS];   // 0 = brick nahi, 1/2/3 = kitni hits baaki
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

// Font global hai taake har draw function use kar sake
sf::Font gameFont;
bool fontLoaded = false;


// ============================================================
//  Text banane ke chhote helpers (string class ke baghair)
// ============================================================

// "source" ka text "destination" ke end mein chipka do
void appendText(char* destination, const char* source)
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

// Number ko digits bana kar "destination" ke end mein chipka do
void appendNumber(char* destination, int number)
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
    // aakhri digit pehle nikalta hai, isliye baad mein ulta likhenge
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


// ============================================================
//  Drawing ke chhote helpers
// ============================================================

void loadFont()
{
    // Pehle program ke saath wali font dhoondte hain, phir Windows / Linux / Mac
    // ke normal font folders mein
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

void showText(sf::RenderWindow& window, const char* message, int size,
    float x, float y, sf::Color color, bool centered)
{
    if (!fontLoaded)
    {
        return;     // font hi nahi mili to text nahi dikhega, crash bhi nahi hoga
    }
    sf::Text text(gameFont, message, (unsigned int)size);
    text.setFillColor(color);
    if (centered)
    {
        // origin text ke beech mein le aao, taake x uska center ban jaye
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin({ bounds.position.x + bounds.size.x / 2.f, 0.f });
    }
    text.setPosition({ x, y });
    window.draw(text);
}

void fillBox(sf::RenderWindow& window, float x, float y, float w, float h, sf::Color color)
{
    sf::RectangleShape box({ w, h });
    box.setPosition({ x, y });
    box.setFillColor(color);
    window.draw(box);
}

void fillCircle(sf::RenderWindow& window, float centerX, float centerY, float radius, sf::Color color)
{
    sf::CircleShape circle(radius);
    circle.setPosition({ centerX - radius, centerY - radius });
    circle.setFillColor(color);
    window.draw(circle);
}

// Gol kono wala rectangle: do rectangles ek doosre ko cross karte hain
// aur har kone mein ek circle laga dete hain
void fillRoundedBox(sf::RenderWindow& window, float x, float y, float w, float h,
    float radius, sf::Color color)
{
    fillBox(window, x + radius, y, w - 2 * radius, h, color);
    fillBox(window, x, y + radius, w, h - 2 * radius, color);
    fillCircle(window, x + radius, y + radius, radius, color);
    fillCircle(window, x + w - radius, y + radius, radius, color);
    fillCircle(window, x + radius, y + h - radius, radius, color);
    fillCircle(window, x + w - radius, y + h - radius, radius, color);
}

// Brick ka rang, is par depend karta hai ke kitni hits baaki hain
sf::Color brickColorFor(int hitsLeft)
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
    return sf::Color(25, 25, 45);          // brick hai hi nahi
}

// Do boxes tab touch karte hain jab dono ek doosre ke left-right
// aur up-down dono mein overlap karein
bool boxesOverlap(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2)
{
    return x1 < x2 + w2 && x1 + w1 > x2 &&
        y1 < y2 + h2 && y1 + h1 > y2;
}


// ============================================================
//  Game setup karna
// ============================================================

void scatterStars(Game& game)
{
    for (int i = 0; i < STAR_COUNT; i++)
    {
        game.starX[i] = rand() % WINDOW_W;
        game.starY[i] = HUD_HEIGHT + rand() % (WINDOW_H - HUD_HEIGHT);
        game.starSize[i] = 1 + rand() % 2;
    }
}

// Ball ko paddle ke bilkul beech mein rakh do
void placeBallOnPaddle(Game& game)
{
    game.ball.launched = false;
    game.ball.x = game.paddle.x + game.paddle.width / 2 - BALL_SIZE / 2;
    game.ball.y = BALL_ON_PADDLE_Y;
    game.ball.dx = 0;
    game.ball.dy = 0;
    for (int i = 0; i < TRAIL_LENGTH; i++)
    {
        game.ball.trailX[i] = game.ball.x;
        game.ball.trailY[i] = game.ball.y;
    }
}

// Paddle wapas beech mein, normal size, koi power-up nahi
void resetPaddle(Game& game)
{
    game.paddle.x = PADDLE_START_X;
    game.paddle.width = PADDLE_NORMAL_W;
    game.wideFramesLeft = 0;
    game.powerUp.active = false;
}

// Program pehli baar start hone par sab kuch initial values se bhar do
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
    game.input = Input{};      // saari keys false

    resetPaddle(game);
    placeBallOnPaddle(game);
    scatterStars(game);
}


// ============================================================
//  Files: level aur save game
// ============================================================

// level.txt se bricks padho. Pehli line mein rows aur columns, phir har brick
// ke liye ek number. File na mile ya galat ho to false return hota hai.
bool readLevelFile(Game& game)
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
            // sirf 0, 1, 2, 3 allowed hain, baaki sab 0 bana do
            if (game.bricks[r][c] < 0 || game.bricks[r][c] > 3)
            {
                game.bricks[r][c] = 0;
            }
        }
    }
    return true;
}

// Score, lives aur bricks ki maujooda halat save karo.
// Line 1: score lives, phir 8 lines jin mein 10-10 numbers.
void saveProgress(Game& game)
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
}

// savegame.txt padho. Pehle numbers ek temporary array mein jaate hain, taake
// kharab ya khali file chalti hui game ko kharab na kare.
// Agar continue karne ke liye kuch nahi to false return hota hai.
bool readSavedProgress(Game& game)
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
    if (bricksLeft == 0)
    {
        return false;
    }

    // sab theek hai, ab asli game mein copy kar do
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


// ============================================================
//  Game shuru aur khatam karna
// ============================================================

void startNewGame(Game& game)
{
    if (!readLevelFile(game))
    {
        game.levelErrorMessage = true;     // menu error dikhayega
        return;
    }
    game.score = 0;
    game.lives = START_LIVES;
    resetPaddle(game);
    placeBallOnPaddle(game);
    game.noSavedGameMessage = false;
    game.levelErrorMessage = false;
    game.gameState = PLAYING;
}

void continueSavedGame(Game& game)
{
    if (!readSavedProgress(game))
    {
        game.noSavedGameMessage = true;    // menu "No saved game" dikhayega
        return;
    }
    resetPaddle(game);                     // paddle beech mein, ball us par intezar karegi
    placeBallOnPaddle(game);
    game.noSavedGameMessage = false;
    game.levelErrorMessage = false;
    game.gameState = PLAYING;
}

void endGame(Game& game, bool won)
{
    game.playerWon = won;
    game.gameState = GAME_OVER;

    // save file khali kar do taake khatam hui game dobara continue na ho
    ofstream fout("savegame.txt");
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


// ============================================================
//  Paddle
// ============================================================

// Paddle ko window ke andar hi rakho
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


// ============================================================
//  Ball
// ============================================================

// Paddle ko 4 barabar hisson mein baanto. Ball ke beech ke neeche jo hissa
// aata hai wohi naya dx decide karta hai, is se player aim kar sakta hai.
void bounceBallOffPaddle(Game& game)
{
    int newDx[4] = { -6, -3, 3, 6 };

    game.ball.y = BALL_ON_PADDLE_Y;        // ball ko paddle ke bilkul upar rakh do
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


// ============================================================
//  Power-ups (bonus)
// ============================================================

// Jab brick tootti hai: shayad uske andar se ek power-up girna shuru ho jaye
void maybeSpawnPowerUp(Game& game, int row, int col)
{
    if (game.powerUp.active)
    {
        return;                            // ek waqt mein sirf ek power-up
    }
    if (rand() % POWERUP_CHANCE != 0)
    {
        return;                            // is baar kismat nahi chali
    }
    game.powerUp.active = true;
    game.powerUp.x = col * BRICK_W + (BRICK_W - POWERUP_W) / 2;
    game.powerUp.y = BRICKS_Y + row * BRICK_H;
    game.powerUp.type = rand() % 2;
}

// Har frame: girta hua power-up move karo, paddle se pakda gaya to apply karo,
// aur wide paddle ka timer bhi ghatao
void updatePowerUps(Game& game)
{
    if (game.powerUp.active)
    {
        game.powerUp.y = game.powerUp.y + POWERUP_SPEED;

        bool caught = boxesOverlap(game.powerUp.x, game.powerUp.y, POWERUP_W, POWERUP_H,
            game.paddle.x, PADDLE_Y, game.paddle.width, PADDLE_H);

        if (caught)
        {
            if (game.powerUp.type == WIDE_PADDLE)
            {
                if (game.paddle.width == PADDLE_NORMAL_W)
                {
                    // beech se bada karo taake paddle ek taraf na uchhal jaye
                    game.paddle.x = game.paddle.x - (PADDLE_WIDE_W - PADDLE_NORMAL_W) / 2;
                    game.paddle.width = PADDLE_WIDE_W;
                    keepPaddleInside(game);
                }
                game.wideFramesLeft = WIDE_FRAMES;  // dobara W pakda to timer phir se shuru
            }
            else
            {
                game.lives++;
            }
            game.powerUp.active = false;
        }
        else if (game.powerUp.y > WINDOW_H)
        {
            game.powerUp.active = false;   // miss ho gaya, ab gayab
        }
    }

    // wide paddle ka countdown, 10 second baad wapas chhota
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


// ============================================================
//  Bricks
// ============================================================

// Un bricks ko dhoondo jinhe ball touch kar rahi hai aur un par hit lagao.
// Ball ke 4 kone 4 points hain. Kisi point ke neeche wali brick ek division se
// mil jati hai:
//      row = (y - 100) / 25         column = x / 60
// Agar ball ne kam az kam ek brick ko chhua to true return hota hai.
bool hitBricksUnderBall(Game& game)
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

        // corner sirf tab use karo jab wo brick area ke andar ho,
        // warna division array se bahar ki row/column de degi
        if (cy < BRICKS_Y || cy >= BRICKS_Y + BRICK_ROWS * BRICK_H || cx < 0 || cx >= WINDOW_W)
        {
            continue;
        }

        int row = (cy - BRICKS_Y) / BRICK_H;
        int col = cx / BRICK_W;
        if (game.bricks[row][col] == 0)
        {
            continue;                      // yahan koi brick nahi
        }

        // agar do corners ek hi brick par hain to us ki sirf ek hit kategi
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

    // saare corners check hone ke baad hi bricks ko damage do
    for (int i = 0; i < hitCount; i++)
    {
        int row = hitRow[i];
        int col = hitCol[i];

        game.bricks[row][col]--;
        if (game.bricks[row][col] > 0)
        {
            game.score = game.score + POINTS_HIT;
        }
        else
        {
            game.score = game.score + POINTS_BREAK;   // is hit ne brick tod di
            maybeSpawnPowerUp(game, row, col);
        }
    }
    return hitCount > 0;
}


// ============================================================
//  Ball ko har frame move karna
// ============================================================

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
        placeBallOnPaddle(game);           // bricks jaisi thi waisi hi rahengi
    }
}

// Ball do steps mein chalti hai: pehle x, phir y. Har step ke baad walls aur
// bricks check hote hain. Kuch lag gaya to step wapas (undo) aur us direction
// ki speed ulti.
void moveBall(Game& game)
{
    // purani positions yaad rakho (tail ke liye)
    for (int i = TRAIL_LENGTH - 1; i > 0; i--)
    {
        game.ball.trailX[i] = game.ball.trailX[i - 1];
        game.ball.trailY[i] = game.ball.trailY[i - 1];
    }
    game.ball.trailX[0] = game.ball.x;
    game.ball.trailY[0] = game.ball.y;

    // step 1: x mein chalo
    game.ball.x = game.ball.x + game.ball.dx;
    if (game.ball.x < 0 || game.ball.x + BALL_SIZE > WINDOW_W)
    {
        game.ball.x = game.ball.x - game.ball.dx;      // undo
        game.ball.dx = -game.ball.dx;
    }
    else if (hitBricksUnderBall(game))
    {
        game.ball.x = game.ball.x - game.ball.dx;      // undo
        game.ball.dx = -game.ball.dx;
    }

    // step 2: y mein chalo
    game.ball.y = game.ball.y + game.ball.dy;
    if (game.ball.y < HUD_HEIGHT)
    {
        game.ball.y = game.ball.y - game.ball.dy;      // undo
        game.ball.dy = -game.ball.dy;
    }
    else if (hitBricksUnderBall(game))
    {
        game.ball.y = game.ball.y - game.ball.dy;      // undo
        game.ball.dy = -game.ball.dy;
    }
    else if (game.ball.dy > 0 &&
        boxesOverlap(game.ball.x, game.ball.y, BALL_SIZE, BALL_SIZE,
            game.paddle.x, PADDLE_Y, game.paddle.width, PADDLE_H))
    {
        bounceBallOffPaddle(game);         // sirf tab jab ball neeche ja rahi ho
    }

    // window ke neeche nikal gayi: ball gayi
    if (game.ball.y > WINDOW_H)
    {
        loseLife(game);
    }
}


// ============================================================
//  INPUT (keyboard padhna)
// ============================================================

void handleInput(sf::RenderWindow& window, Game& game)
{
    // jo keys ek press mein ek hi baar kaam karti hain, har frame saaf kar do
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

    // jo keys har frame check hoti hain (dabi rakho to paddle chalti rahe)
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


// ============================================================
//  UPDATE (har frame game ki logic)
// ============================================================

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
            continueSavedGame(game);
            break;
        case OPTION_EXIT:
            game.quitGame = true;
            break;
        }
    }
}

void updatePlaying(Game& game)
{
    // Esc: game save karo aur menu par wapas
    if (game.input.escape)
    {
        saveProgress(game);
        game.menuChoice = 0;
        game.gameState = MENU;
        return;
    }

    // paddle ko left/right chalao
    if (game.input.left)
    {
        game.paddle.x = game.paddle.x - PADDLE_SPEED;
    }
    if (game.input.right)
    {
        game.paddle.x = game.paddle.x + PADDLE_SPEED;
    }
    keepPaddleInside(game);

    if (!game.ball.launched)
    {
        // ball paddle par baithi hai aur Space dabne tak uske saath chalti hai
        placeBallOnPaddle(game);
        if (game.input.space)
        {
            game.ball.launched = true;
            game.ball.dy = -BALL_SPEED_Y;
            // left ya right, random side par chhodo
            if (rand() % 2 == 0)
            {
                game.ball.dx = BALL_LAUNCH_SPEED_X;
            }
            else
            {
                game.ball.dx = -BALL_LAUNCH_SPEED_X;
            }
        }
    }
    else
    {
        moveBall(game);
    }

    // moveBall game khatam bhi kar sakta hai (lives zero), isliye pehle check
    if (game.gameState != PLAYING)
    {
        return;
    }

    updatePowerUps(game);

    if (countBricksLeft(game) == 0)
    {
        endGame(game, true);               // saari bricks toot gayin, jeet gaye
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
        // Enter dabao to menu par wapas
        if (game.input.enter)
        {
            game.menuChoice = 0;
            game.gameState = MENU;
        }
        break;
    }
}


// ============================================================
//  DRAW (screen par sab kuch banana)
// ============================================================

// Background, halke halke chamakte tare aur dono taraf patli deewarein
void drawBackground(sf::RenderWindow& window, Game& game)
{
    window.clear(sf::Color(14, 17, 42));

    for (int i = 0; i < STAR_COUNT; i++)
    {
        int t = (game.frameCount + i * 17) % 100;
        if (t > 50)
        {
            t = 100 - t;
        }
        int brightness = 40 + t * 2;
        fillBox(window, (float)game.starX[i], (float)game.starY[i],
            (float)game.starSize[i], (float)game.starSize[i],
            sf::Color(170, 190, 255, (unsigned char)brightness));
    }

    fillBox(window, 0.f, (float)HUD_HEIGHT, 3.f, (float)(WINDOW_H - HUD_HEIGHT), sf::Color(45, 55, 125));
    fillBox(window, (float)(WINDOW_W - 3), (float)HUD_HEIGHT, 3.f, (float)(WINDOW_H - HUD_HEIGHT), sf::Color(45, 55, 125));
}

void drawHud(sf::RenderWindow& window, Game& game)
{
    fillBox(window, 0.f, 0.f, (float)WINDOW_W, (float)HUD_HEIGHT, sf::Color(22, 26, 62));
    fillBox(window, 0.f, (float)(HUD_HEIGHT - 2), (float)WINDOW_W, 2.f, sf::Color(95, 105, 200));

    // score
    char scoreText[40];
    scoreText[0] = '\0';
    appendText(scoreText, "SCORE  ");
    appendNumber(scoreText, game.score);
    showText(window, scoreText, 22, 16.f, 10.f, sf::Color(240, 244, 255), false);

    // wide paddle ka timer bar (bonus)
    if (game.wideFramesLeft > 0)
    {
        showText(window, "WIDE", 12, 215.f, 8.f, sf::Color(60, 220, 255), false);
        fillBox(window, 215.f, 30.f, 100.f, 5.f, sf::Color(35, 45, 100));
        fillBox(window, 215.f, 30.f, 100.f * game.wideFramesLeft / WIDE_FRAMES, 5.f, sf::Color(60, 220, 255));
    }

    // lives chhote circles ki shakal mein
    showText(window, "LIVES", 14, 345.f, 16.f, sf::Color(150, 160, 225), false);
    for (int i = 0; i < game.lives && i < MAX_LIVES_SHOWN; i++)
    {
        fillCircle(window, 425.f + i * 20.f, 26.f, 8.f, sf::Color(255, 255, 255));
        fillCircle(window, 425.f + i * 20.f, 26.f, 6.f, sf::Color(255, 77, 109));
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
                continue;                  // brick nahi hai to kuch draw nahi karna
            }
            float x = (float)(c * BRICK_W + 1);
            float y = (float)(BRICKS_Y + r * BRICK_H + 1);
            float w = (float)(BRICK_W - 2);
            float h = (float)(BRICK_H - 2);

            // pehle safed border, phir rangeen body, phir ek chhoti si chamak
            fillRoundedBox(window, x, y, w, h, 6.f, sf::Color(240, 245, 255));
            fillRoundedBox(window, x + 1, y + 1, w - 2, h - 2, 5.f, brickColorFor(game.bricks[r][c]));
            fillBox(window, x + 7, y + 5, 12.f, 3.f, sf::Color(255, 255, 255, 110));
        }
    }
}

void drawPaddle(sf::RenderWindow& window, Game& game)
{
    float x = (float)game.paddle.x;
    float width = (float)game.paddle.width;

    fillRoundedBox(window, x - 1, (float)PADDLE_Y - 1, width + 2, (float)PADDLE_H + 2, 8.f, sf::Color(240, 250, 255));
    fillRoundedBox(window, x, (float)PADDLE_Y, width, (float)PADDLE_H, 7.f, sf::Color(60, 220, 255));
    fillBox(window, x + 8, (float)PADDLE_Y + 3, width - 16, 3.f, sf::Color(190, 245, 255));
}

void drawBall(sf::RenderWindow& window, Game& game)
{
    float centerX = game.ball.x + BALL_SIZE / 2.f;
    float centerY = game.ball.y + BALL_SIZE / 2.f;

    // chamakti hui dum, sirf jab ball ud rahi ho
    if (game.ball.launched)
    {
        for (int i = TRAIL_LENGTH - 1; i >= 0; i--)
        {
            float radius = 5.f - i * 0.7f;
            int alpha = 110 - i * 20;
            fillCircle(window, game.ball.trailX[i] + BALL_SIZE / 2.f,
                game.ball.trailY[i] + BALL_SIZE / 2.f, radius,
                sf::Color(170, 190, 255, (unsigned char)alpha));
        }
    }
    fillCircle(window, centerX, centerY, 12.f, sf::Color(150, 180, 255, 45));   // glow
    fillCircle(window, centerX, centerY, 6.f, sf::Color(255, 255, 255));
}

void drawPowerUp(sf::RenderWindow& window, Game& game)
{
    if (!game.powerUp.active)
    {
        return;
    }
    float x = (float)game.powerUp.x;
    float y = (float)game.powerUp.y;

    sf::Color color = sf::Color(60, 220, 255);          // W cyan hai
    const char* letter = "W";
    if (game.powerUp.type == EXTRA_LIFE)
    {
        color = sf::Color(255, 120, 160);               // + pink hai
        letter = "+";
    }
    fillRoundedBox(window, x - 1, y - 1, POWERUP_W + 2.f, POWERUP_H + 2.f, 10.f, sf::Color(255, 255, 255));
    fillRoundedBox(window, x, y, (float)POWERUP_W, (float)POWERUP_H, 9.f, color);
    showText(window, letter, 14, x + POWERUP_W / 2.f, y, sf::Color(14, 17, 42), true);
}

// Khelte waqt jo kuch dikhta hai (game over screen ke peeche bhi yahi hota hai)
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
        // jhapakta hua hint
        if ((game.frameCount / 30) % 2 == 0)
        {
            showText(window, "PRESS SPACE TO LAUNCH", 18, WINDOW_W / 2.f, 560.f,
                sf::Color(150, 165, 235), true);
        }
    }
}

void drawMenu(sf::RenderWindow& window, Game& game)
{
    drawBackground(window, game);

    // title
    showText(window, "BLOCK", 72, WINDOW_W / 2.f, 70.f, sf::Color(60, 220, 255), true);
    showText(window, "BUSTER", 72, WINDOW_W / 2.f, 140.f, sf::Color(255, 77, 109), true);

    // title ke neeche rangeen patti
    fillBox(window, 90.f, 240.f, 105.f, 5.f, sf::Color(255, 77, 109));
    fillBox(window, 195.f, 240.f, 105.f, 5.f, sf::Color(255, 209, 60));
    fillBox(window, 300.f, 240.f, 105.f, 5.f, sf::Color(60, 220, 255));
    fillBox(window, 405.f, 240.f, 105.f, 5.f, sf::Color(180, 120, 255));
    showText(window, "A BRICK BREAKER GAME", 15, WINDOW_W / 2.f, 258.f, sf::Color(130, 140, 205), true);

    // teen menu buttons
    const char* names[OPTION_COUNT] = { "NEW GAME", "CONTINUE", "EXIT" };
    for (int i = 0; i < OPTION_COUNT; i++)
    {
        float x = 160.f;
        float y = 320.f + i * 64.f;
        if (i == game.menuChoice)
        {
            // jo option select hai wo cyan border ke saath chamakta hai
            fillRoundedBox(window, x - 2, y - 2, 284.f, 52.f, 12.f, sf::Color(60, 220, 255));
            fillRoundedBox(window, x, y, 280.f, 48.f, 10.f, sf::Color(30, 62, 120));
            showText(window, names[i], 24, WINDOW_W / 2.f, y + 9.f, sf::Color(255, 255, 255), true);
        }
        else
        {
            fillRoundedBox(window, x - 2, y - 2, 284.f, 52.f, 12.f, sf::Color(70, 80, 150));
            fillRoundedBox(window, x, y, 280.f, 48.f, 10.f, sf::Color(22, 26, 62));
            showText(window, names[i], 24, WINDOW_W / 2.f, y + 9.f, sf::Color(150, 160, 225), true);
        }
    }

    // error / info messages
    if (game.noSavedGameMessage)
    {
        showText(window, "No saved game", 20, WINDOW_W / 2.f, 520.f, sf::Color(255, 77, 109), true);
    }
    if (game.levelErrorMessage)
    {
        showText(window, "Error: level.txt was not found", 20, WINDOW_W / 2.f, 520.f, sf::Color(255, 77, 109), true);
    }

    showText(window, "UP / DOWN  choose        ENTER  select", 14, WINDOW_W / 2.f, 570.f,
        sf::Color(110, 120, 190), true);
}

void drawGameOver(sf::RenderWindow& window, Game& game)
{
    drawPlayScene(window, game);

    // game ke upar andhera parda aur beech mein ek panel
    fillBox(window, 0.f, 0.f, (float)WINDOW_W, (float)WINDOW_H, sf::Color(8, 10, 28, 190));

    sf::Color mainColor = sf::Color(255, 77, 109);
    const char* title = "GAME OVER";
    if (game.playerWon)
    {
        mainColor = sf::Color(61, 220, 151);
        title = "YOU WIN";
    }

    fillRoundedBox(window, 90.f, 230.f, 420.f, 250.f, 18.f, mainColor);
    fillRoundedBox(window, 93.f, 233.f, 414.f, 244.f, 16.f, sf::Color(22, 26, 62));

    showText(window, title, 60, WINDOW_W / 2.f, 255.f, mainColor, true);

    char scoreText[40];
    scoreText[0] = '\0';
    appendText(scoreText, "FINAL SCORE  ");
    appendNumber(scoreText, game.score);
    showText(window, scoreText, 26, WINDOW_W / 2.f, 345.f, sf::Color(255, 209, 60), true);

    if ((game.frameCount / 30) % 2 == 0)
    {
        showText(window, "Press Enter", 22, WINDOW_W / 2.f, 415.f, sf::Color(220, 228, 255), true);
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


// ============================================================
//  MAIN
// ============================================================

int main()
{
    srand((unsigned int)time(0));

    sf::RenderWindow window(sf::VideoMode({ 600, 700 }), "Block Buster");
    window.setFramerateLimit(60);          // har laptop par game ki speed ek jaisi
    window.setKeyRepeatEnabled(false);     // key dabane par sirf ek baar count ho

    loadFont();

    Game game;
    setupGame(game);

    while (window.isOpen())
    {
        handleInput(window, game);
        if (!window.isOpen())
        {
            break;                         // X button dab gaya
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