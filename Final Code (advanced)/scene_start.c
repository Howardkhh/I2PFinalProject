#include "scene_start.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include "game.h"
#include "utility.h"
#include "scene_menu.h"
#include "scene_dead.h"
#include "shared.h"
#include <math.h>

#define PI 3.14159265358979323846

static ALLEGRO_BITMAP* img_background;
static ALLEGRO_BITMAP* img_plane;
static ALLEGRO_BITMAP* img_small_fighter;
static ALLEGRO_BITMAP* img_small_fighter_broken1;
static ALLEGRO_BITMAP* img_small_fighter_broken2;
static ALLEGRO_BITMAP* img_middle_fighter;
static ALLEGRO_BITMAP* img_plane_bullet;
static ALLEGRO_BITMAP* img_middle_bullet;
static ALLEGRO_BITMAP* img_heart;

typedef struct {
    // The center coordinate of the image.
    float x, y;
    // The width and height of the object.
    float w, h;
    // The velocity in x, y axes.
    float vx, vy;
    // Rotation angle.
    float theta;
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

#define MAX_SMALL_ENEMY 5
#define MAX_MIDDLE_ENEMY 3
#define MAX_PLANE_BULLET 50
#define MAX_MIDDLE_BULLET 6

static Player plane;
static Enemy small_enemies[MAX_SMALL_ENEMY];
static Enemy middle_enemies[MAX_MIDDLE_ENEMY];
static Bullet plane_bullets[MAX_PLANE_BULLET];
static Bullet middle_bullets[MAX_MIDDLE_BULLET];
static const float MAX_COOLDOWN = 0.05;
static double last_shoot_timestamp;
static double last_middle_shoot_timestamp[MAX_MIDDLE_ENEMY];
static double middle_bullet_max_turn_angle = 0.4 * PI / 60;
static ALLEGRO_SAMPLE* bgm;
static ALLEGRO_SAMPLE_ID bgm_id;
static bool draw_gizmos;

static void init(void) {
    int i;
    start_time = al_get_time();
    score = 0;

    init_image();

    init_plane(&plane);

    for (i = 0; i < MAX_SMALL_ENEMY; i++) {
        init_enemy(&small_enemies[i], 0);
    }

    for (int i = 0; i < MAX_MIDDLE_ENEMY; i++) {
        init_enemy(&middle_enemies[i], 1);
    }

    for (int i = 0; i < MAX_PLANE_BULLET; i++) {
        init_bullet(&plane_bullets[i], 0);
    }

    for (int i = 0; i < MAX_MIDDLE_BULLET; i++) {
        init_bullet(&middle_bullets[i], 1);
    }

    // Can be moved to shared_init to decrease loading time.
    bgm = load_audio("resources\\mythica.ogg");
    game_log("Start scene initialized");
    bgm_id = play_bgm(bgm, 1);
}

static init_image() {
    img_background = load_bitmap_resized("resources\\start-bg.jpg", SCREEN_W, SCREEN_H);
    img_small_fighter = load_bitmap("resources\\smallfighter.png");
    img_small_fighter_broken1 = load_bitmap("resources\\smallfighter_broken1.png");
    img_small_fighter_broken2 = load_bitmap("resources\\smallfighter_broken2.png");
    img_middle_fighter = load_bitmap("resources\\rocket-3.png");
    img_middle_bullet = load_bitmap("resources\\missile.png");
    img_plane_bullet = load_bitmap("resources\\image12.png");
    img_plane = load_bitmap("resources\\plane.png");
    img_heart = load_bitmap("resources\\hearts.png");
}


static void init_plane(Player* player) {
    player->obj.img[0] = img_plane;
    player->obj.x = 400;
    player->obj.y = 500;
    player->obj.w = al_get_bitmap_width(player->obj.img[0]);
    player->obj.h = al_get_bitmap_height(player->obj.img[0]);
    player->obj.theta = 0;
    player->max_health = 5;
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

    case 1:
        enemy->obj.img[0] = img_middle_fighter;
        enemy->max_health = 1;
        break;
    }

    enemy->type = type;
    enemy->obj.w = al_get_bitmap_width(enemy->obj.img[0]);
    enemy->obj.h = al_get_bitmap_height(enemy->obj.img[0]);
    enemy->obj.x = enemy->obj.w / 2 + (float)rand() / RAND_MAX * (SCREEN_W - enemy->obj.w);
    enemy->obj.y = 80;
    enemy->obj.vx = 0;
    enemy->obj.vy = 1;
    enemy->obj.theta = PI;
    enemy->obj.hidden = true;
    enemy->health = enemy->max_health;

}

static void init_bullet(Bullet* bullet, int type) {
    //type 1: plane bullets, 2: mid fighter bullets
    switch (type) {
    case 0:
        bullet->obj.img[0] = img_plane_bullet;
        bullet->obj.vx = 0;
        bullet->obj.vy = -20;
        break;

    case 1:
        bullet->obj.img[0] = img_middle_bullet;
        bullet->obj.vx = 0;
        bullet->obj.vy = 0;
        break;
    }
    bullet->type = type;
    bullet->obj.w = al_get_bitmap_width(bullet->obj.img[0]);
    bullet->obj.h = al_get_bitmap_height(bullet->obj.img[0]);
    bullet->obj.theta = 0;
    bullet->obj.hidden = true;
}

#define max_len 101
int collision_detect(MovableObject obj1, MovableObject obj2) {
    if ((obj1.x - obj1.w / 2 >= obj2.x - obj2.w / 2 && obj1.x - obj1.w / 2 <= obj2.x + obj1.w / 2 ||
         obj1.x + obj1.w / 2 >= obj2.x - obj2.w / 2 && obj1.x + obj1.w / 2 <= obj2.x + obj2.w / 2) &&
        (obj1.y - obj1.h / 2 >= obj2.y - obj2.h / 2 && obj1.y - obj1.h / 2 <= obj2.y + obj2.h / 2 ||
         obj1.y + obj1.h / 2 >= obj2.y - obj2.h / 2 && obj1.y + obj1.h / 2 <= obj2.y + obj2.h / 2)) {
        char collision_box1[max_len][max_len] = {0};
        char collision_box2[max_len][max_len] = {0};
        unsigned char pixel, _;

        al_lock_bitmap(obj1.img[0], ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READWRITE);
        al_lock_bitmap(obj2.img[0], ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READWRITE);

        for (int x = 0; x < obj1.w; x++) {
            for (int y = 0; y < obj1.h; y++) {
                al_unmap_rgba(al_get_pixel(obj1.img[0], x, y), &_, &_, &_, &pixel);
                collision_box1[(int)((x - obj1.w / 2) * cos((double)obj1.theta) - (y - obj1.h / 2) * sin((double)obj1.theta) + (max_len-1)/2+1)][(int)((x - obj1.w / 2) * sin((double)obj1.theta) + (y - obj1.h / 2) * cos((double)obj1.theta) + (max_len-1)/2+1)] = (pixel != 0);
            }
        }
        for (int x = 0; x < obj2.w; x++) {
            for (int y = 0; y < obj2.h; y++) {
                al_unmap_rgba(al_get_pixel(obj2.img[0], x, y), &_, &_, &_, &pixel);
                collision_box2[(int)((x - obj2.w / 2) * cos((double)obj2.theta) - (y - obj2.h / 2) * sin((double)obj2.theta) + (max_len-1)/2+1)][(int)((x - obj2.w / 2) * sin((double)obj2.theta) + (y - obj2.h / 2) * cos((double)obj2.theta) + (max_len-1)/2+1)] = (pixel != 0);
            }
        }

        al_unlock_bitmap(obj1.img[0]);
        al_unlock_bitmap(obj2.img[0]);

        for (int y = 0; y < max_len; y++) {
            for (int x = 0; x < max_len; x++) {
                if (!collision_box1[x][y] || obj1.x - obj2.x + x >= max_len || obj1.x - obj2.x + x < 0 || obj1.y - obj2.y + y >= max_len || obj1.y - obj2.y + y < 0)
                    continue;
                if (collision_box2[(int)(obj1.x - obj2.x + x)][(int)(obj1.y - obj2.y + y)])
                    return 1;
            }
        }
    }
    return 0;
}
#undef max_len

//restrict_in_LRUD: 0:check boundaries, 1: don't check boundaries, 2: go out then hidden
void move_object(MovableObject* obj, int* restrict_in_LRUD) {
    obj->x += obj->vx;
    obj->y += obj->vy;

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
    //collision detect
    int i, j;
    for (i = 0; i < MAX_SMALL_ENEMY; i++) {
        if (small_enemies[i].obj.hidden)
            continue;
        if (collision_detect(plane.obj, small_enemies[i].obj)) {
            plane.health--;
            score -= 10;
            small_enemies[i].obj.hidden = true;
        }
    }
    for (i = 0; i < MAX_MIDDLE_ENEMY; i++) {
        if (middle_enemies[i].obj.hidden)
            continue;
        if (collision_detect(plane.obj, middle_enemies[i].obj)) {
            plane.health--;
            score -= 10;
            middle_enemies[i].obj.hidden = true;
        }
    }
    for (i = 0; i < MAX_MIDDLE_BULLET; i++) {
        if (middle_bullets[i].obj.hidden)
            continue;
        if (collision_detect(plane.obj, middle_bullets[i].obj)) {
            plane.health--;
            score -= 10;
            middle_bullets[i].obj.hidden = true;
        }
    }

    if (plane.health <= 0)
        game_change_scene(scene_dead_create());

    for (i = 0; i < MAX_PLANE_BULLET; i++) {
        if (plane_bullets[i].obj.hidden)
            continue;
        for (j = 0; j < MAX_SMALL_ENEMY; j++) {
            if (small_enemies[j].obj.hidden)
                continue;
            if (collision_detect(plane_bullets[i].obj, small_enemies[j].obj)) {
                small_enemies[j].health--;
                plane_bullets[i].obj.hidden = true;
                if (small_enemies[j].health == 0) {
                    small_enemies[j].obj.hidden = true;
                    score += 10;
                }
            }
        }
        if (plane_bullets[i].obj.hidden)
            continue;
        for (j = 0; j < MAX_MIDDLE_ENEMY; j++) {
            if (middle_enemies[j].obj.hidden)
                continue;
            if (collision_detect(plane_bullets[i].obj, middle_enemies[j].obj)) {
                middle_enemies[j].health--;
                plane_bullets[i].obj.hidden = true;
                if (middle_enemies[j].health == 0) {
                    middle_enemies[j].obj.hidden = true;
                    score += 50;
                }
            }
        }
        if (plane_bullets[i].obj.hidden)
            continue;
        for (j = 0; j < MAX_MIDDLE_BULLET; j++) {
            if (middle_bullets[j].obj.hidden)
                continue;
            if (collision_detect(plane_bullets[i].obj, middle_bullets[j].obj)) {
                plane_bullets[i].obj.hidden = true;
                middle_bullets[j].obj.hidden = true;
            }
        }
    }

    //create and move objects
    plane.obj.vx = plane.obj.vy = 0;
    if (key_state[ALLEGRO_KEY_UP] || key_state[ALLEGRO_KEY_W])
        plane.obj.vy -= 1;
    if (key_state[ALLEGRO_KEY_DOWN] || key_state[ALLEGRO_KEY_S])
        plane.obj.vy += 1;
    if (key_state[ALLEGRO_KEY_LEFT] || key_state[ALLEGRO_KEY_A])
        plane.obj.vx -= 1;
    if (key_state[ALLEGRO_KEY_RIGHT] || key_state[ALLEGRO_KEY_D])
        plane.obj.vx += 1;

    plane.obj.vx *= 7 * (plane.obj.vy ? 0.71f : 1);
    plane.obj.vy *= 7 * (plane.obj.vx ? 0.71f : 1);
    move_object(&plane.obj, (int[]){0, 0, 0, 0});

    for (i = 0; i < MAX_PLANE_BULLET; i++) {
        if (plane_bullets[i].obj.hidden)
            continue;
        move_object(&plane_bullets[i].obj, (int[]) {2, 2, 2, 2});
    }

    int hidden_enemy_count = 0;
    int enemy_group_size = 5;
    
    for (i = 0; i < MAX_SMALL_ENEMY; i++) {
        if (small_enemies[i].obj.hidden) {
            hidden_enemy_count++;
            continue;
        }
        small_enemies[i].obj.vx = -5*sin(small_enemies[i].obj.y/10);
        move_object(&small_enemies[i].obj, (int[]) { 0, 0, 1, 2 });
    }

    for (i = 0; i < MAX_MIDDLE_ENEMY; i++) {
        if (middle_enemies[i].obj.hidden && !(rand()%(FPS*MAX_MIDDLE_ENEMY))) {
            middle_enemies[i].obj.hidden = false;
            middle_enemies[i].obj.x = (rand() % 2) * SCREEN_W;
            middle_enemies[i].obj.y = (rand() % 2) * SCREEN_H;
            middle_enemies[i].health = middle_enemies[i].max_health;
        }
        middle_enemies[i].obj.theta = atan2((double)plane.obj.y - (double)middle_enemies[i].obj.y, (double)plane.obj.x - (double)middle_enemies[i].obj.x + 0.001);
        if (middle_enemies[i].obj.theta < 0)
            middle_enemies[i].obj.theta += 2 * PI;
        middle_enemies[i].obj.vx = 2 * cos(middle_enemies[i].obj.theta);
        middle_enemies[i].obj.vy = 2 * sin(middle_enemies[i].obj.theta);
        move_object(&middle_enemies[i].obj, (int[]) { 0, 0, 0, 0 });
    }

    if (hidden_enemy_count >= enemy_group_size) {
        float x_pos = small_enemies[0].obj.w / 2 + rand() % (int)(SCREEN_W - small_enemies[0].obj.w);
        float y_pos = -small_enemies[0].obj.h/2;
        for (i = 0, j = 0; i < MAX_SMALL_ENEMY && j < enemy_group_size; i++) {
            if (small_enemies[i].obj.hidden) {
                small_enemies[i].obj.hidden = false;
                small_enemies[i].obj.y = y_pos;
                small_enemies[i].obj.x = x_pos + 50*cos(small_enemies[i].obj.y/10);
                small_enemies[i].obj.vy = 1;
                small_enemies[i].health = small_enemies[i].max_health;
                y_pos -= small_enemies[i].obj.h + 5;
                j++;
            }
        }
    }

    for (i = 0; i < MAX_MIDDLE_BULLET; i++) {
        if (middle_bullets[i].obj.hidden)
            continue;

        if (middle_bullets[i].obj.theta < 0)
            middle_bullets[i].obj.theta += 2 * PI;
        if (middle_bullets[i].obj.theta > 2 * PI)
            middle_bullets[i].obj.theta = fmod(middle_bullets[i].obj.theta, 2 * PI);

        double new_angle = atan2((double)plane.obj.y - (double)middle_bullets[i].obj.y, (double)plane.obj.x - (double)middle_bullets[i].obj.x + 0.00001);
        if (new_angle < 0)
            new_angle += 2 * PI;

        double dangle;
        if (fabs(new_angle - (double)middle_bullets[i].obj.theta) > PI)
            dangle = 2 * PI - fabs(new_angle - middle_bullets[i].obj.theta);
        else
            dangle = fabs(new_angle - middle_bullets[i].obj.theta);

        if (dangle > middle_bullet_max_turn_angle) {
            int sign = ((fmod(new_angle - middle_bullets[i].obj.theta + 2 * PI, 2*PI) > 0 && fmod(new_angle - middle_bullets[i].obj.theta + 2 * PI, 2 * PI) < PI) ? 1 : -1);
            middle_bullets[i].obj.theta += middle_bullet_max_turn_angle * sign;
        }
        else {
            middle_bullets[i].obj.theta = new_angle;
        }

        middle_bullets[i].obj.vx = 3 * cos(middle_bullets[i].obj.theta);
        middle_bullets[i].obj.vy = 3 * sin(middle_bullets[i].obj.theta);
        move_object(&middle_bullets[i].obj, (int[]) { 2, 2, 2, 2 });
    }

    //shoot bullets

    double now = al_get_time();
    if (key_state[75] && now - last_shoot_timestamp >= MAX_COOLDOWN) {
        for (i = 0; i < MAX_PLANE_BULLET; i++) {
            if (plane_bullets[i].obj.hidden) {
                last_shoot_timestamp = now;
                plane_bullets[i].obj.hidden = false;
                plane_bullets[i].obj.x = plane.obj.x;
                plane_bullets[i].obj.y = plane.obj.y;
                plane_bullets[i].obj.theta = -PI / 2;
                plane_bullets[i].obj.vx = 0;
                plane_bullets[i].obj.vy = -20;
                break;
            }
        }
    }

    for (i = 0; i < MAX_MIDDLE_ENEMY; i++) {
        if (!middle_enemies[i].obj.hidden && now - last_middle_shoot_timestamp[i] >= 5) {
            for (j = 0; j < MAX_MIDDLE_BULLET; j++) {
                if (middle_bullets[j].obj.hidden) {
                    last_middle_shoot_timestamp[i] = now;
                    middle_bullets[j].obj.hidden = false;
                    middle_bullets[j].obj.x = middle_enemies[i].obj.x;
                    middle_bullets[j].obj.y = middle_enemies[i].obj.y;
                    middle_bullets[j].obj.theta = middle_enemies[i].obj.theta;
                    break;
                }
            }
        }
    }

}

static void draw_movable_object(MovableObject obj, int pic_num) {
    if (obj.hidden)
        return;
    al_draw_rotated_bitmap(obj.img[pic_num], obj.w/2, obj.h/2, obj.x, obj.y, obj.theta, 0);
    if (draw_gizmos) {
        al_draw_rectangle(round((double)obj.x - (double)obj.w / 2), round((double)obj.y - (double)obj.h / 2),
            round((double)obj.x + (double)obj.w / 2) + 1, round((double)obj.y + (double)obj.h / 2) + 1, al_map_rgb(255, 0, 0), 0);
    }
}

static void draw(void) {
    int i;
    al_draw_bitmap(img_background, 0, 0, 0);
    for (i = 0; i < plane.health; i++)
        al_draw_bitmap(img_heart, SCREEN_W - 30 - i * 30, 10, 0);
    for (i = 0; i < MAX_PLANE_BULLET; i++)
        draw_movable_object(plane_bullets[i].obj, 0);
    draw_movable_object(plane.obj, 0);
    for (i = 0; i < MAX_SMALL_ENEMY; i++)
        draw_movable_object(small_enemies[i].obj, small_enemies[i].max_health- small_enemies[i].health);
    for (i = 0; i < MAX_MIDDLE_ENEMY; i++)
        draw_movable_object(middle_enemies[i].obj, 0);
    for (i = 0; i < MAX_MIDDLE_BULLET; i++)
        draw_movable_object(middle_bullets[i].obj, 0);
    float now = al_get_time();
    al_draw_textf(font_pirulen_24, al_map_rgb(0, 0, 0), 10, 10, 0, "Time Survived: %d:%d:%d", (int)((now - start_time) / 60), ((int)(now - start_time) % 60), (int)((now-start_time)*1000)%1000);
    al_draw_textf(font_pirulen_24, al_map_rgb(0, 0, 0), 10, 50, 0, "Score: %d", score);
}

static void destroy(void) {
    al_destroy_bitmap(img_background);
    al_destroy_bitmap(img_plane);
    al_destroy_bitmap(img_small_fighter);
    al_destroy_bitmap(img_small_fighter_broken1);
    al_destroy_bitmap(img_small_fighter_broken2);
    al_destroy_bitmap(img_plane_bullet);
    al_destroy_bitmap(img_middle_bullet);
    al_destroy_bitmap(img_middle_fighter);
    al_destroy_bitmap(img_heart);
    al_destroy_sample(bgm);
    stop_bgm(bgm_id);
    game_log("Start scene destroyed");
}

static void on_key_down(int keycode) {
    if (keycode == ALLEGRO_KEY_TAB)
        draw_gizmos = !draw_gizmos;
}

static void on_mouse_down(int btn, int x, int y, int dz) {
    if (btn == mouse_state[1]) {
        double now = al_get_time();
        if (now - last_shoot_timestamp >= MAX_COOLDOWN) {
            for (int i = 0; i < MAX_PLANE_BULLET; i++) {
                if (plane_bullets[i].obj.hidden) {
                    last_shoot_timestamp = now;
                    plane_bullets[i].obj.hidden = false;
                    plane_bullets[i].obj.x = plane.obj.x;
                    plane_bullets[i].obj.y = plane.obj.y;
                    plane_bullets[i].obj.theta = atan2(y - plane.obj.y, x - plane.obj.x);
                    plane_bullets[i].obj.vx = 20 * cos(plane_bullets[i].obj.theta);
                    plane_bullets[i].obj.vy = 20 * sin(plane_bullets[i].obj.theta);
                    break;
                }
            }
        }
    }
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
    scene.on_mouse_down = &on_mouse_down;
    // TODO: Register more event callback functions such as keyboard, mouse, ...
    game_log("Start scene created");
    return scene;
}