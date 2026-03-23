#include <iostream>
#include <ncurses.h>
#include <thread>
#include <utility>
#include <unordered_map>
#include <chrono>

typedef std::pair<int,int> pii;

#define WINDOW_HEIGHT 30
#define WINDOW_WIDTH 60
#define WINDOW_TL_X 2
#define WINDOW_TL_Y 5
#define WINDOW_WALL_TOP 0
#define WINDOW_WALL_LEFT 0
#define WINDOW_WALL_BOTTOM (WINDOW_HEIGHT-1)
#define WINDOW_WALL_RIGHT (WINDOW_WIDTH-1)

bool gameover = false;
bool caught_mouse = false;
int score = 0;

enum DIRECTION {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

std::unordered_map<DIRECTION, DIRECTION> opposite = {
    {UP,DOWN},
    {DOWN,UP},
    {LEFT,RIGHT},
    {RIGHT,LEFT}
};

pii getRandomXY() {
    int x = 1 + rand() % (WINDOW_HEIGHT-2);
    int y = 1 + rand() % ((WINDOW_WIDTH/2)-2);
    return {x,y};
}

void showGameOverMessage(WINDOW* window) {
    int cy = WINDOW_HEIGHT / 2;
    int cx = WINDOW_WIDTH / 2;
    mvwprintw(window, cy,   cx - 5, "GAME OVER");
    mvwprintw(window, cy+1, cx - 5, "Score: %d", score);
}

class Mouse {
private:
    int x;
    int y;
public:
    Mouse() {
        pii coords = getRandomXY();
        x = coords.first;
        y = coords.second;
        y++;
    }
    pii getCoords() {
        return {x,y};
    }
    void move() {
        pii coords = getRandomXY();
        x = coords.first;
        y = coords.second;
        y++;
    }
    void draw(WINDOW* window) {
        mvwaddch(window, x, y, ACS_DIAMOND);
        //mvwaddch(window, x, y*2+1, ACS_DIAMOND);
    }
};

struct SNAKENODE {
    int x = 0;
    int y = 0;
    SNAKENODE* next = nullptr;
    SNAKENODE* prev = nullptr;
    SNAKENODE(pii coords) {
        x = coords.first;
        y = coords.second;
    }
    pii getCoords() {
        return {x,y};
    }
};

class Snake {
private:
    DIRECTION direction = DOWN;
    SNAKENODE* HEAD;
    SNAKENODE* TAIL;

public:
    Snake() {
        pii random_initial_coords = getRandomXY();
        HEAD = new SNAKENODE({random_initial_coords.first,random_initial_coords.second});
        TAIL = new SNAKENODE({random_initial_coords.first-1,random_initial_coords.second});
        HEAD->prev = TAIL;
        HEAD->next = nullptr;
        TAIL->prev = nullptr;
        TAIL->next = HEAD;
    }

    ~Snake() {
        SNAKENODE* currnode = TAIL;
        while (currnode != nullptr) {
            SNAKENODE* deletenode = currnode;
            currnode = currnode->next;
            deletenode->next = nullptr;
            deletenode->prev = nullptr;
            delete deletenode;
        }
        HEAD = nullptr;
        TAIL = nullptr;
    }

    DIRECTION getDirection() {
        return direction;
    }

    void setDirection(DIRECTION new_direction) {
        DIRECTION current_direction = direction;
        if (new_direction != current_direction && new_direction != opposite[current_direction] ) {
            direction = new_direction;
        }
    }

    pii getHeadCoords() {
        return {HEAD->x,HEAD->y};
    }

    void removeTail() {
        SNAKENODE* oldtail = TAIL;
        TAIL = oldtail->next;
        TAIL->prev = nullptr;
        oldtail->next = nullptr;
        delete oldtail;
    };

    void addHead(SNAKENODE* sn) {
        SNAKENODE* oldhead = HEAD;
        HEAD = sn;
        oldhead->next = HEAD;
        HEAD->prev = oldhead;
    };

    void process_input() {

        int c = getch();
        switch (c) {
            case KEY_UP: setDirection(UP); break;
            case KEY_DOWN: setDirection(DOWN); break;
            case KEY_LEFT: setDirection(LEFT); break;
            case KEY_RIGHT: setDirection(RIGHT); break;
            default: break;
        }

        pii coords = getHeadCoords();
        DIRECTION current_direction = getDirection();
        switch (current_direction) {
            case UP: coords.first--; break;
            case DOWN: coords.first++; break;
            case LEFT: coords.second -= 2; break;
            case RIGHT: coords.second += 2; break;
            default: break;
        }

        SNAKENODE* newhead = new SNAKENODE(coords);
        addHead(newhead);

        if (caught_mouse) {
            score++;
            caught_mouse = false;
        } else {
            removeTail();
        }

        return;
    }

    void draw(WINDOW* window) {

        SNAKENODE* currnode = HEAD;
        while (currnode != nullptr) {
            mvwaddch(window, currnode->x, currnode->y, ACS_CKBOARD);
            mvwaddch(window, currnode->x, currnode->y+1, ACS_CKBOARD);
            currnode = currnode->prev;
        }

        return;
    }

    bool check_wall_collision() {
        pii playercoords = getHeadCoords();
        bool collided = false;
        if (WINDOW_WALL_LEFT >= playercoords.second || playercoords.second >= WINDOW_WALL_RIGHT) {
            collided = true;
        }
        if (WINDOW_WALL_TOP >= playercoords.first || playercoords.first >= WINDOW_WALL_BOTTOM) {
            collided = true;
        }
        return collided;
    }

    bool check_self_collision() {
        SNAKENODE* checknode = HEAD->prev;
        pii headcoords = getHeadCoords();
        while (checknode != nullptr) {
            pii checkcoords = checknode->getCoords();
            bool xmatched = checkcoords.first == headcoords.first;
            bool ymatched = checkcoords.second == headcoords.second || checkcoords.second == headcoords.second+1;

            if (xmatched && ymatched) {
                return true;
            }
            checknode = checknode->prev;
        }
        return false;
    }

    bool check_mouse_collision(Mouse& prey) {
        bool collided = false;
        pii preycoords = prey.getCoords();
        pii playercoords = getHeadCoords();
        bool xmatched = preycoords.first == playercoords.first;
        bool ymatched = preycoords.second == playercoords.second || preycoords.second == playercoords.second+1;

        if (xmatched && ymatched) {
            caught_mouse = true;
            collided = true;
        }
        return collided;
    }

};

int main() {

    initscr();
    noecho();
    curs_set(0);
    WINDOW* window = newwin(WINDOW_HEIGHT, WINDOW_WIDTH, WINDOW_TL_X, WINDOW_TL_Y);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    Snake player = Snake();
    Mouse prey = Mouse();

    int movecount = 0;
    int moveinterval = 5;
    while (!gameover) {

        std::this_thread::sleep_for(std::chrono::milliseconds(16));

        if (movecount % moveinterval == 0) {
            player.process_input();
            movecount=0;
        }
        movecount++;

        wclear(window);
        box(window, 0, 0);
        player.draw(window);
        prey.draw(window);

        if (player.check_wall_collision() || player.check_self_collision()) {
            mvwprintw(window, 0, 2, " Score: %d ", score);
            showGameOverMessage(window);
            wrefresh(window);
            std::this_thread::sleep_for(std::chrono::seconds(10));
            gameover = true;
        }

        if (player.check_mouse_collision(prey)) {
            prey.move();
        }
        mvwprintw(window, 0, 2, " Score: %d ", score);
        wrefresh(window);

    }

    delwin(window);
    endwin();

    return 0;

}