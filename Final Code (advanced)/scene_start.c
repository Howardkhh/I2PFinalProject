#include "scene_start.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include "game.h"
#include "utility.h"
#include "scene_menu.h"
#include <math.h>


static ALLEGRO_BITMAP* img_background;
static ALLEGRO_BITMAP* img_plane;
static ALLEGRO_BITMAP* img_small_fighter;
static ALLEGRO_BITMAP* img_small_fighter_broken1;
static ALLEGRO_BITMAP* img_small_fighter_broken2;
static ALLEGRO_BITMAP* img_bullet;

typedef struct {
    // The center coordinate of the image.
    float x, y;
    // The width and height of the object.
    float w, h;
    // The velocity in x, y axes.
    float vx, vy;
    // Should we draw this object on the screen.
    bool hidden;
    // The pointer to the object’s image.
    ALLEGRO_BITMAP* img[5];

} MovableObject;

typedef struct {
    int health;
    int max_health;
    MovableObject obj;
} Player;

typedef struct {
    //0: small fighter, 1: rocket
    int type;
    int health;
    int max_health;
    MovableObject obj;
} Enemy;

typedef struct {
    //0: player's bullet, 1; rocket's bullet
    int type;
    MovableObject obj;
} Bullet;

static void init(void);
static void init_plane(Player* player);
static void init_enemy(Enemy* enemy, int type);
static void init_bullet(Bullet* bullet, int type);
static void update(void);
static void draw_movable_object(MovableObject obj, int pic_num);
static void draw(void);
static void destroy(void);

#define MAX_ENEMY 5
#define MAX_BULLET 50

static Player plane;
static Enemy enemies[MAX_ENEMY];
static Bullet bullets[MAX_BULLET];
static const float MAX_COOLDOWN = 0.1;
static double last_shoot_timestamp;
static ALLEGRO_SAMPLE* bgm;
static ALLEGRO_SAMPLE_ID bgm_id;
static bool draw_gizmos;

static void init(void) {
    int i;
    img_background = load_bitmap_resized("resources\\start-bg.jpg", SCREEN_W, SCREEN_H);
    img_small_fighter = load_bitmap("resources\\smallfighter.png");
    img_small_fighter_broken1 = load_bitmap("resources\\smallfighter_broken1.png");
    img_small_fighter_broken2 = load_bitmap("resources\\smallfighter_broken2.png");
    img_bullet = load_bitmap("resources\\image12.png");
    img_plane = load_bitmap("resources\\plane.png");


    init_plane(&plane);

    for (i = 0; i < MAX_ENEMY; i++) {
        init_enemy(&enemies[i], 0);
    }

    for (int i = 0; i < MAX_BULLET; i++) {
        init_bullet(&bullets[i], 0);
    }
    // Can be moved to shared_init to decrease loading time.
    bgm = load_audio("resources\\mythica.ogg");
    game_log("Start scene initialized");
    bgm_id = play_bgm(bgm, 1);
}


static void init_plane(Player* player) {
    player->obj.img[0] = img_plane;
    player->obj.x = 400;
    player->obj.y = 500;
    player->obj.w = al_get_bitmap_width(player->obj.img[0]);
    player->obj.h = al_get_bitmap_height(player->obj.img[0]);
    player->max_health = 1;
    player->health = player->max_health;

}


static void init_enemy(Enemy* enemy, int type) {
    //type 0: small fighter, 1:mid fighter
    switch (type) {
    case 0:
        enemy->obj.img[0] = img_small_fighter;
        enemy->obj.img[1] = img_small_fighter_broken1;
        enemy->obj.img[2] = img_small_fighter_broken2;
        enemy->max_health = 3;
        break;
    }

    enemy->type = type;
    enemy->obj.w = al_get_bitmap_width(enemy->obj.img[0]);
    enemy->obj.h = al_get_bitmap_height(enemy->obj.img[0]);
    enemy->obj.x = enemy->obj.w / 2 + (float)rand() / RAND_MAX * (SCREEN_W - enemy->obj.w);
    enemy->obj.y = 80;
    enemy->obj.vx = 0;
    enemy->obj.vy = 1;
    enemy->obj.hidden = true;
    enemy->health = enemy->max_health;

}

static void init_bullet(Bullet* bullet, int type) {
    //type 1: normal bullets, 2: mid fighter bullets
    switch (type) {
    case 0:
        bullet->obj.img[0] = img_bullet;
        break;
    }
    bullet->type = type;
    bullet->obj.w = al_get_bitmap_width(img_bullet);
    bullet->obj.h = al_get_bitmap_height(img_bullet);
    bullet->obj.vx = 0;
    bullet->obj.vy = -3;
    bullet->obj.hidden = true;
}

int collision_detect(MovableObject obj1, MovableObject obj2) {
    if ((obj1.x - obj1.w / 2 >= obj2.x - obj2.w / 2 && obj1.x - obj1.w / 2 <= obj2.x + obj1.w / 2  ||
         obj1.x + obj1.w / 2 >= obj2.x - obj2.w / 2 && obj1.x + obj1.w / 2 <= obj2.x + obj2.w / 2) &&
        (obj1.y - obj1.h / 2 >= obj2.y - obj2.h / 2 && obj1.y - obj1.h / 2 <= obj2.y + obj2.h / 2  ||
         obj1.y + obj1.h / 2 >= obj2.y - obj2.h / 2 && obj1.y + obj1.h / 2 <= obj2.y + obj2.h / 2)) {
        return 1;
    }
    else {
        return 0;
    }
}

//restrict_in_LRUD: 0:check boundaries, 1: don't check boundaries, 2: go out then hidden
void move_object(MovableObject* obj, float dx, float dy, int* restrict_in_LRUD) {
    obj->x += dx;
    obj->y += dy;

    if (restrict_in_LRUD[0] == 0) {
        if (obj->x - obj->w / 2 < 0)
            obj->x = obj->w / 2;
    }
    else if (restrict_in_LRUD[0] == 1);
    else if (restrict_in_LRUD[0] == 2){
        if (obj->x + obj->w / 2 < 0) {
            obj->hidden = true;
           return;
        }
    }
    if (restrict_in_LRUD[1] == 0) {
        if (obj->x + obj->w / 2 > SCREEN_W)
            obj->x = SCREEN_W - obj->w / 2;
    }
    else if (restrict_in_LRUD[1] == 1);
    else if (restrict_in_LRUD[1] == 2) {
        if (obj->x - obj->w / 2 > SCREEN_W) {
            obj->hidden = true;
            return;
        }
    }
    if (restrict_in_LRUD[2] == 0) {
        if (obj->y - obj->h / 2 < 0) 
            obj->y = obj->h / 2;
    }
    else if (restrict_in_LRUD[2] == 1);
    else if (restrict_in_LRUD[2] == 2) {
        if (obj->y + obj->h / 2 < 0) {
            obj->hidden = true;
            return;
        }
    }
    if (restrict_in_LRUD[3] == 0) {
        if (obj->y + obj->h / 2 > SCREEN_H)
            obj->y = SCREEN_H - obj->h / 2;
    }
    else if (restrict_in_LRUD[3] == 1);
    else if (restrict_in_LRUD[3] == 2) {
        if (obj->y - obj->h / 2 > SCREEN_H) {
            obj->hidden = true;
            return;
        }
    }
    return;
}


static void update(void) {
    for (int i = 0; i < MAX_ENEMY; i++) {
        if (enemies[i].obj.hidden)
            continue;
        if (collision_detect(plane.obj, enemies[i].obj)) {
            game_change_scene(scene_menu_create());
        }
    }

    plane.obj.vx = plane.obj.vy = 0;
    if (key_state[ALLEGRO_KEY_UP] || key_state[ALLEGRO_KEY_W])
        plane.obj.vy -= 1;
    if (key_state[ALLEGRO_KEY_DOWN] || key_state[ALLEGRO_KEY_S])
        plane.obj.vy += 1;
    if (key_state[ALLEGRO_KEY_LEFT] || key_state[ALLEGRO_KEY_A])
        plane.obj.vx -= 1;
    if (key_state[ALLEGRO_KEY_RIGHT] || key_state[ALLEGRO_KEY_D])
        plane.obj.vx += 1;
    move_object(&plane.obj, plane.obj.vx * 4 * (plane.obj.vy ? 0.71f : 1), plane.obj.vy * 4 * (plane.obj.vx ? 0.71f : 1), (int[]){0, 0, 0, 0});


    int i, j;
    for (i = 0; i < MAX_BULLET; i++) {
        if (bullets[i].obj.hidden)
            continue;
        for (j = 0; j < MAX_ENEMY; j++) {
            if (enemies[j].obj.hidden)
                continue;
            if (collision_detect(bullets[i].obj, enemies[j].obj)) {
                enemies[j].health--;
                bullets[i].obj.hidden = true;
                if (enemies[j].health == 0)
                    enemies[j].obj.hidden = true;
            }
        }
    }

    for (i = 0; i < MAX_BULLET; i++) {
        if (bullets[i].obj.hidden)
            continue;
        move_object(&bullets[i].obj, bullets[i].obj.vx, bullets[i].obj.vy, (int[]) {0, 0, 2, 0});
    }

    int hidden_enemy_count = 0;
    int enemy_group_size = 5;
    
    for (i = 0; i < MAX_ENEMY; i++) {
        if (enemies[i].obj.hidden) {
            hidden_enemy_count++;
            continue;
        }
        enemies[i].obj.vx = -5*sin(enemies[i].obj.y/10);
        move_object(&enemies[i].obj, enemies[i].obj.vx, enemies[i].obj.vy, (int[]) { 0, 0, 1, 2 });
    }

    if (hidden_enemy_count >= enemy_group_size) {
        float x_pos = enemies[0].obj.w / 2 + rand() % (int)(SCREEN_W - enemies[0].obj.w);
        float y_pos = -enemies[0].obj.h/2;
        for (i = 0, j = 0; i < MAX_ENEMY && j < enemy_group_size; i++) {
            if (enemies[i].obj.hidden) {
                enemies[i].obj.hidden = false;
                enemies[i].obj.y = y_pos;
                enemies[i].obj.x = x_pos + 50*cos(enemies[i].obj.y/10);
                enemies[i].obj.vy = 1;
                enemies[i].health = enemies[i].max_health;
                y_pos -= enemies[i].obj.h + 5;
                j++;
            }
        }
    }

    double now = al_get_time();
    if (key_state[75] && now - last_shoot_timestamp >= MAX_COOLDOWN) {
        for (i = 0; i < MAX_BULLET; i++) {
            if (bullets[i].obj.hidden) {
                last_shoot_timestamp = now;
                bullets[i].obj.hidden = false;
                bullets[i].obj.x = plane.obj.x;
                bullets[i].obj.y = plane.obj.y;
                break;
            }
        }
    }
}

static void draw_movable_object(MovableObject obj, int pic_num) {
    if (obj.hidden)
        return;
    al_draw_bitmap(obj.img[pic_num], round((double)obj.x - (double)obj.w / 2), round((double)obj.y - (double)obj.h / 2), 0);
    if (draw_gizmos) {
        al_draw_rectangle(round((double)obj.x - (double)obj.w / 2), round((double)obj.y - (double)obj.h / 2),
            round((double)obj.x + (double)obj.w / 2) + 1, round((double)obj.y + (double)obj.h / 2) + 1, al_map_rgb(255, 0, 0), 0);
    }
}

static void draw(void) {
    int i;
    al_draw_bitmap(img_background, 0, 0, 0);
    for (i = 0; i < MAX_BULLET; i++)
        draw_movable_object(bullets[i].obj, 0);
    draw_movable_object(plane.obj, 0);
    for (i = 0; i < MAX_ENEMY; i++)
        draw_movable_object(enemies[i].obj, enemies[i].max_health- enemies[i].health);
}

static void destroy(void) {
    al_destroy_bitmap(img_background);
    al_destroy_bitmap(img_plane);
    al_destroy_bitmap(img_small_fighter);
    al_destroy_bitmap(img_small_fighter_broken1);
    al_destroy_bitmap(img_small_fighter_broken2);
    al_destroy_sample(bgm);
    al_destroy_bitmap(img_bullet);
    stop_bgm(bgm_id);
    game_log("Start scene destroyed");
}

static void on_key_down(int keycode) {
    if (keycode == ALLEGRO_KEY_TAB)
        draw_gizmos = !draw_gizmos;
}

// TODO: Add more event callback functions such as keyboard, mouse, ...

// Functions without 'static', 'extern' prefixes is just a normal
// function, they can be accessed by other files using 'extern'.
// Define your normal function prototypes below.

// The only function that is shared across files.
Scene scene_start_create(void) {
    Scene scene;
    memset(&scene, 0, sizeof(Scene));
    scene.name = "Start";
    scene.initialize = &init;
    scene.update = &update;
    scene.draw = &draw;
    scene.destroy = &destroy;
    scene.on_key_down = &on_key_down;
    // TODO: Register more event callback functions such as keyboard, mouse, ...
    game_log("Start scene created");
    return scene;
}