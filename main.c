#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

// Game Constants
#define DELAY 50000 // Microseconds for delay
#define MIN_ROWS 20
#define MIN_COLS 20

// Color pairs
#define COLOR_SNAKE_HEAD 1
#define COLOR_SNAKE_BODY 2
#define COLOR_FOOD 3
#define COLOR_BORDER 4
#define COLOR_TEXT 5

// Directions
typedef enum {
    UP,
    DOWN,
    LEFT,
    RIGHT
} Direction;

// Point structure for coordinates
typedef struct {
    int x;
    int y;
} Point;

// Snake structure
typedef struct {
    Point *body;      // Array of body segments
    int length;       // Current length
    int max_length;   // Maximum possible length (half perimeter)
    Direction dir;    // Current direction
} Snake;

// Global variables
int max_y = 0, max_x = 0;
Snake snake;
Point food;
bool game_over = false;
bool victory = false;
// Game window
WINDOW *game_win = NULL;
int pit_height = 20; // playable area rows
int pit_width = 40;  // playable area cols
int win_start_y = 0;
int win_start_x = 0;

/*
 * Functionality: Initializes ncurses and game settings with color support.
 */
void init_game() {
    initscr();              // Start ncurses mode
    noecho();               // Don't echo keystrokes
    cbreak();               // Disable line buffering
    keypad(stdscr, TRUE);   // Enable function keys (arrows)
    curs_set(0);            // Hide cursor
    timeout(0);             // Non-blocking getch
    
    // Initialize colors if terminal supports it
    if (has_colors()) {
        start_color();
        // Green snake head
        init_pair(COLOR_SNAKE_HEAD, COLOR_GREEN, COLOR_BLACK);
        // Bright green snake body
        init_pair(COLOR_SNAKE_BODY, COLOR_GREEN, COLOR_BLACK);
        // Red food
        init_pair(COLOR_FOOD, COLOR_RED, COLOR_BLACK);
        // Cyan border
        init_pair(COLOR_BORDER, COLOR_CYAN, COLOR_BLACK);
        // Yellow text
        init_pair(COLOR_TEXT, COLOR_YELLOW, COLOR_BLACK);
    }
}

/*
 * Functionality: Initializes the snake with length 3 at the center of the screen.
 */
void init_snake() {
    // Use window-local coordinates (1..pit_height, 1..pit_width)
    // Calculate half perimeter for win condition
    int perimeter = 2 * (pit_height + pit_width);
    snake.max_length = perimeter / 2;

    // Allocate memory for snake body (max possible length)
    snake.body = (Point *)malloc(snake.max_length * sizeof(Point));

    // Initialize snake with length 3 at center of the pit
    snake.length = 3;
    int center_y = pit_height / 2 + 1; // local coords (1-based inside border)
    int center_x = pit_width / 2 + 1;

    snake.body[0].y = center_y;
    snake.body[0].x = center_x;
    snake.body[1].y = center_y;
    snake.body[1].x = center_x - 1;
    snake.body[2].y = center_y;
    snake.body[2].x = center_x - 2;

    snake.dir = RIGHT; // Initial direction
}

/*
 * Functionality: Draws the border around the snake pit (20x20 minimum) with color.
 */
void draw_border() {
    if (!game_win) return;
    if (has_colors()) {
        wattron(game_win, COLOR_PAIR(COLOR_BORDER));
    }

    // Draw border around game window
    wborder(game_win, 0,0,0,0,0,0,0,0);

    if (has_colors()) {
        wattroff(game_win, COLOR_PAIR(COLOR_BORDER));
    }
}

/*
 * Functionality: Draws the snake on the screen with larger characters and color.
 */
void draw_snake() {
    if (!game_win) return;

    // Draw head with larger block character
    // Choose head glyph based on direction
    chtype head_glyph = '>';
    switch (snake.dir) {
        case UP:
            head_glyph = '^';
            break;
        case DOWN:
            head_glyph = 'v';
            break;
        case LEFT:
            head_glyph = '<';
            break;
        case RIGHT:
        default:
            head_glyph = '>';
            break;
    }

    if (has_colors()) {
        wattron(game_win, COLOR_PAIR(COLOR_SNAKE_HEAD) | A_BOLD);
    }
    mvwaddch(game_win, snake.body[0].y, snake.body[0].x, head_glyph);
    if (has_colors()) {
        wattroff(game_win, COLOR_PAIR(COLOR_SNAKE_HEAD) | A_BOLD);
    }

    // Draw body with block characters
    if (has_colors()) {
        wattron(game_win, COLOR_PAIR(COLOR_SNAKE_BODY));
    }
    for (int i = 1; i < snake.length; i++) {
        mvwaddch(game_win, snake.body[i].y, snake.body[i].x, ACS_BLOCK);
    }
    if (has_colors()) {
        wattroff(game_win, COLOR_PAIR(COLOR_SNAKE_BODY));
    }
}

/*
 * Functionality: Checks if a point overlaps with the snake body.
 */
bool is_snake_position(int x, int y) {
    for (int i = 0; i < snake.length; i++) {
        if (snake.body[i].x == x && snake.body[i].y == y) {
            return true;
        }
    }
    return false;
}

/*
 * Functionality: Places food at a random location inside the playable area, avoiding snake body.
 */
void place_food() {
    int attempts = 0;
    int max_attempts = 100; // Prevent infinite loop
    
    do {
        // Place food inside the pit (local window coords: 1..pit_height, 1..pit_width)
        food.y = (rand() % pit_height) + 1;
        food.x = (rand() % pit_width) + 1;
        attempts++;
    } while (is_snake_position(food.x, food.y) && attempts < max_attempts);
    
}

/*
 * Functionality: Checks if snake collides with walls or itself. Returns true if collision detected.
 */
bool check_collision() {
    Point head = snake.body[0];

    // Check wall collision in window-local coords (valid range: 1..pit_width / 1..pit_height)
    if (head.x <= 0 || head.x > pit_width || head.y <= 0 || head.y > pit_height) {
        return true;
    }
    
    // Check self collision
    for (int i = 1; i < snake.length; i++) {
        if (head.x == snake.body[i].x && head.y == snake.body[i].y) {
            return true;
        }
    }
    
    return false;
}

/*
 * Functionality: Checks if snake has reached win condition (length = half perimeter).
 */
bool check_win() {
    return snake.length >= snake.max_length;
}

/*
 * Functionality: Cleans up ncurses before exiting.
 */
void cleanup_game() {
    free(snake.body); // Free allocated memory
    if (game_win) {
        delwin(game_win);
        game_win = NULL;
    }
    endwin(); // End ncurses mode
}

/*
 * Functionality: Updates snake position based on current direction.
 */
void update_snake() {
    // Save tail position
    Point tail = snake.body[snake.length - 1];
    
    // Move body segments (each segment takes position of previous)
    for (int i = snake.length - 1; i > 0; i--) {
        snake.body[i] = snake.body[i - 1];
    }
    
    // Move head based on direction
    switch (snake.dir) {
        case UP:
            snake.body[0].y--;
            break;
        case DOWN:
            snake.body[0].y++;
            break;
        case LEFT:
            snake.body[0].x--;
            break;
        case RIGHT:
            snake.body[0].x++;
            break;
    }
    
    // Check if food is consumed
    if (snake.body[0].x == food.x && snake.body[0].y == food.y) {
        // Grow snake
        snake.length++;
        snake.body[snake.length - 1] = tail;
        // Place new food
        place_food();
    }
}

/*
 * Functionality: Displays a simple start screen and waits for user to press space.
 */
void show_start_screen() {
    clear();
    int center_y = max_y / 2;
    int center_x = max_x / 2;
    
    if (has_colors()) {
        attron(COLOR_PAIR(COLOR_TEXT) | A_BOLD);
    }
    
    mvprintw(center_y - 3, center_x - 10, "================");
    mvprintw(center_y - 2, center_x - 10, "   SNAKE GAME   ");
    mvprintw(center_y - 1, center_x - 10, "================");
    mvprintw(center_y + 1, center_x - 15, "Use Arrow Keys to Move");
    mvprintw(center_y + 2, center_x - 12, "Eat food to grow");
    mvprintw(center_y + 3, center_x - 15, "To Win: Reach a length of %d", snake.max_length);
    mvprintw(center_y + 5, center_x - 10, "Press SPACE to start");
    mvprintw(center_y + 6, center_x - 8, "Press 'q' to quit");
    
    if (has_colors()) {
        attroff(COLOR_PAIR(COLOR_TEXT) | A_BOLD);
    }
    
    refresh();
    
    timeout(-1); // Blocking input
    int ch;
    while ((ch = getch()) != ' ' && ch != 'q' && ch != 'Q');
    timeout(0); // Back to non-blocking
}

/*
 * Functionality: The main game loop handling input and updates.
 */
void game_loop() {
    int ch;
    bool running = true;
    int frame_count = 0;
    int move_interval = 2; // Move snake every N frames (faster movement)

    while (running && !game_over && !victory) {
        ch = getch();

        // Handle input
        switch (ch) {
            case 'q':
            case 'Q':
                running = false;
                break;
            case KEY_UP:
                if (snake.dir != DOWN) snake.dir = UP;
                break;
            case KEY_DOWN:
                if (snake.dir != UP) snake.dir = DOWN;
                break;
            case KEY_LEFT:
                if (snake.dir != RIGHT) snake.dir = LEFT;
                break;
            case KEY_RIGHT:
                if (snake.dir != LEFT) snake.dir = RIGHT;
                break;
        }

        // Update game state (move snake every N frames)
        frame_count++;
        if (frame_count >= move_interval) {
            update_snake();
            frame_count = 0;
            
            // Check collisions
            if (check_collision()) {
                game_over = true;
                break;
            }
            
            // Check win condition
            if (check_win()) {
                victory = true;
                break;
            }
        }
        
        // Draw everything
        // Clear both stdscr and game window
        clear();
        if (game_win) werase(game_win);

        draw_border();
        draw_snake();

        // Draw food with larger character and color inside window
        if (game_win) {
            if (has_colors()) {
                wattron(game_win, COLOR_PAIR(COLOR_FOOD) | A_BOLD);
            }
            mvwaddch(game_win, food.y, food.x, ACS_DIAMOND);
            if (has_colors()) {
                wattroff(game_win, COLOR_PAIR(COLOR_FOOD) | A_BOLD);
            }
        }

        // Display score/length with color on stdscr (above the window)
        if (has_colors()) {
            attron(COLOR_PAIR(COLOR_TEXT));
        }
        mvprintw(win_start_y - 1, win_start_x, "Length: %d/%d", snake.length, snake.max_length);
        if (has_colors()) {
            attroff(COLOR_PAIR(COLOR_TEXT));
        }

        // Refresh windows
        if (game_win) wrefresh(game_win);
        usleep(DELAY); // Control game speed
    }
    
    // Display game over or victory message
    clear();
    if (game_win) werase(game_win);
    draw_border();
    draw_snake();
    
    if (has_colors()) {
        attron(COLOR_PAIR(COLOR_TEXT) | A_BOLD);
    }
    
    if (game_over) {
        mvprintw(max_y / 2 - 1, max_x / 2 - 5, "GAME OVER!");
        mvprintw(max_y / 2, max_x / 2 - 8, "Final Length: %d", snake.length);
    } else if (victory) {
        mvprintw(max_y / 2 - 1, max_x / 2 - 5, "YOU WIN!");
        mvprintw(max_y / 2, max_x / 2 - 8, "Length: %d/%d", snake.length, snake.max_length);
    }
    
    mvprintw(max_y / 2 + 1, max_x / 2 - 8, "Press 'q' to quit");
    
    if (has_colors()) {
        attroff(COLOR_PAIR(COLOR_TEXT) | A_BOLD);
    }
    if (game_win) wrefresh(game_win);
    refresh();
    
    // Wait for quit key if game ended
    timeout(-1); // Blocking input
    while ((ch = getch()) != 'q' && ch != 'Q');
}

int main() {
    // Initialize random seed once
    srand(time(NULL));
    
    init_game();
    
    // Check if terminal is large enough and create centered small game window
    getmaxyx(stdscr, max_y, max_x);
    // Require a bit of padding around window for messages
    if (max_y < pit_height + 4 || max_x < pit_width + 4) {
        endwin();
        printf("Terminal too small! Need at least %dx%d (including borders)\n", pit_height + 2, pit_width + 2);
        return 1;
    }

    // Center the game window
    win_start_y = (max_y - (pit_height + 2)) / 2;
    win_start_x = (max_x - (pit_width + 2)) / 2;
    game_win = newwin(pit_height + 2, pit_width + 2, win_start_y, win_start_x);
    keypad(game_win, TRUE);

    // Initialize snake
    init_snake();
    
    // Show start screen
    show_start_screen();
    
    // Place initial food
    place_food();
    
    // Start game loop
    game_loop();
    
    cleanup_game();
    return 0;
}

