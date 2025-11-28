/*
 * Author: [Your Name/Group Member Name]
 * Functionality: Main entry point of the program. Initializes the game, runs the main loop, and handles cleanup.
 */

#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

// Game Constants
#define DELAY 100000 // Microseconds for game loop delay
#define MIN_ROWS 20
#define MIN_COLS 20

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

/*
 * Author: [Your Name]
 * Functionality: Initializes ncurses and game settings.
 */
void init_game() {
    initscr();              // Start ncurses mode
    noecho();               // Don't echo keystrokes
    cbreak();               // Disable line buffering
    keypad(stdscr, TRUE);   // Enable function keys (arrows)
    curs_set(0);            // Hide cursor
    timeout(0);             // Non-blocking getch
}

/*
 * Author: [Your Name]
 * Functionality: Initializes the snake with length 3 at the center of the screen.
 */
void init_snake() {
    getmaxyx(stdscr, max_y, max_x);
    
    // Calculate playable area (inside border)
    int pit_height = max_y - 2;  // -2 for top and bottom border
    int pit_width = max_x - 2;   // -2 for left and right border
    
    // Calculate half perimeter for win condition
    int perimeter = 2 * (pit_height + pit_width);
    snake.max_length = perimeter / 2;
    
    // Allocate memory for snake body (max possible length)
    snake.body = (Point *)malloc(snake.max_length * sizeof(Point));
    
    // Initialize snake with length 3 at center
    snake.length = 3;
    int center_y = max_y / 2;
    int center_x = max_x / 2;
    
    snake.body[0].y = center_y;
    snake.body[0].x = center_x;
    snake.body[1].y = center_y;
    snake.body[1].x = center_x - 1;
    snake.body[2].y = center_y;
    snake.body[2].x = center_x - 2;
    
    snake.dir = RIGHT; // Initial direction
}

/*
 * Author: [Your Name]
 * Functionality: Draws the border around the snake pit (20x20 minimum).
 */
void draw_border() {
    // Draw top and bottom borders
    for (int x = 0; x < max_x; x++) {
        mvaddch(0, x, '-');           // Top border
        mvaddch(max_y - 1, x, '-');   // Bottom border
    }
    
    // Draw left and right borders
    for (int y = 0; y < max_y; y++) {
        mvaddch(y, 0, '|');           // Left border
        mvaddch(y, max_x - 1, '|');   // Right border
    }
    
    // Draw corners
    mvaddch(0, 0, '+');
    mvaddch(0, max_x - 1, '+');
    mvaddch(max_y - 1, 0, '+');
    mvaddch(max_y - 1, max_x - 1, '+');
}

/*
 * Author: [Your Name]
 * Functionality: Draws the snake on the screen.
 */
void draw_snake() {
    // Draw head
    mvaddch(snake.body[0].y, snake.body[0].x, '@');
    
    // Draw body
    for (int i = 1; i < snake.length; i++) {
        mvaddch(snake.body[i].y, snake.body[i].x, 'o');
    }
}

/*
 * Author: [Your Name]
 * Functionality: Places food at a random location inside the playable area.
 */
void place_food() {
    srand(time(NULL));
    // Place food inside border (avoid walls)
    food.y = (rand() % (max_y - 2)) + 1;
    food.x = (rand() % (max_x - 2)) + 1;
    mvaddch(food.y, food.x, '*');
}

/*
 * Author: [Your Name]
 * Functionality: Checks if snake collides with walls or itself. Returns true if collision detected.
 */
bool check_collision() {
    Point head = snake.body[0];
    
    // Check wall collision
    if (head.x <= 0 || head.x >= max_x - 1 || head.y <= 0 || head.y >= max_y - 1) {
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
 * Author: [Your Name]
 * Functionality: Checks if snake has reached win condition (length = half perimeter).
 */
bool check_win() {
    return snake.length >= snake.max_length;
}

/*
 * Author: [Your Name]
 * Functionality: Cleans up ncurses before exiting.
 */
void cleanup_game() {
    free(snake.body); // Free allocated memory
    endwin(); // End ncurses mode
}

/*
 * Author: [Your Name]
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
 * Author: [Your Name]
 * Functionality: The main game loop handling input and updates.
 */
void game_loop() {
    int ch;
    bool running = true;

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

        // Update game state
        update_snake();
        
        // Check collisions
        if (check_collision()) {
            game_over = true;
            mvprintw(max_y / 2, max_x / 2 - 5, "GAME OVER!");
            break;
        }
        
        // Check win condition
        if (check_win()) {
            victory = true;
            mvprintw(max_y / 2, max_x / 2 - 5, "YOU WIN!");
            break;
        }
        
        // Draw everything
        clear();
        draw_border();
        draw_snake();
        mvaddch(food.y, food.x, '*');
        
        // Display score/length
        mvprintw(0, 2, "Length: %d/%d", snake.length, snake.max_length);
        
        refresh(); // Refresh the screen
        usleep(DELAY); // Control game speed
    }
    
    // Wait for quit key if game ended
    if (game_over || victory) {
        timeout(-1); // Blocking input
        mvprintw(max_y / 2 + 1, max_x / 2 - 5, "Press 'q' to quit");
        refresh();
        while (getch() != 'q' && getch() != 'Q');
    }
}

int main() {
    init_game();
    
    // Check if terminal is large enough
    getmaxyx(stdscr, max_y, max_x);
    if (max_y < MIN_ROWS + 2 || max_x < MIN_COLS + 2) {
        endwin();
        printf("Terminal too small! Need at least %dx%d\n", MIN_ROWS + 2, MIN_COLS + 2);
        return 1;
    }
    
    // Initialize snake
    init_snake();
    
    // Place initial food
    place_food();
    
    // Start game loop
    game_loop();
    
    cleanup_game();
    return 0;
}

