#include "scene_start.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include "game.h"
#include "utility.h"
#include "scene_menu.h"
#include "scene_dead.h"
#include "scene_win.h"
#include "shared.h"
#include <math.h>

#define PI 3.14159265358979323846

static ALLEGRO_BITMAP* img_background;
static ALLEGRO_BITMAP* img_plane1;
static ALLEGRO_BITMAP* img_plane2;
static ALLEGRO_BITMAP* img_avatar1;
static ALLEGRO_BITMAP* img_avatar2;
static ALLEGRO_BITMAP* img_small_fighter;
static ALLEGRO_BITMAP* img_small_fighter_broken1;
static ALLEGRO_BITMAP* img_small_fighter_broken2;
static ALLEGRO_BITMAP* img_middle_fighter;
static ALLEGRO_BITMAP* img_plane_bullet1;
static ALLEGRO_BITMAP* img_plane_bullet2;
static ALLEGRO_BITMAP* img_middle_bullet;
static ALLEGRO_BITMAP* img_heart;

static ALLEGRO_SAMPLE* sound_laser;
static ALLEGRO_SAMPLE* sound_gun;
static ALLEGRO_SAMPLE* sound_explosion;
static ALLEGRO_SAMPLE* sound_metal;

static char hitbox_plane[2][101][101];
static char hitbox_small_fighter[101][101];
static char hitbox_middle_fighter[101][101];
static char hitbox_plane_bullet[2][101][101];
static char hitbox_middle_bullet[101][101];

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
    int type;
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
static void init_image();
static void init_hitbox();
static void init_plane();
static void init_sound();
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

static Player plane[2];
static Enemy small_enemies[MAX_SMALL_ENEMY];
static Enemy middle_enemies[MAX_MIDDLE_ENEMY];
static Bullet plane_bullets[2][MAX_PLANE_BULLET];
static Bullet middle_bullets[MAX_MIDDLE_BULLET];
static const float MAX_COOLDOWN[2] = { 0.25, 0.5 };
static double last_shoot_timestamp[2];
static double last_middle_shoot_timestamp[MAX_MIDDLE_ENEMY];
static double middle_bullet_max_turn_angle = 0.4 * PI / 60;
static ALLEGRO_SAMPLE* bgm;
static ALLEGRO_SAMPLE_ID bgm_id;
static bool draw_gizmos;
static int plane_start, plane_end;
int multiplayer, selected_plane;

static void init(void) {
    int i;
    start_time = al_get_time();
    score = 0;

    if (multiplayer) {
        plane_start = 0;
        plane_end = 2;
    }
    else if (selected_plane == 0) {
        plane_start = 0;
        plane_end = 1;
    }
    else if (selected_plane == 1) {
        plane_start = 1;
        plane_end = 2;
    }

    init_image();
    init_hitbox();
    init_plane();
    init_sound();

    for (i = 0; i < MAX_SMALL_ENEMY; i++) {
        init_enemy(&small_enemies[i], 0);
    }

    for (int i = 0; i < MAX_MIDDLE_ENEMY; i++) {
        init_enemy(&middle_enemies[i], 1);
    }

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < MAX_PLANE_BULLET; j++) {
            init_bullet(&plane_bullets[i][j], i);
        }
    }

    for (int i = 0; i < MAX_MIDDLE_BULLET; i++) {
        init_bullet(&middle_bullets[i], 2);
    }

    // Can be moved to shared_init to decrease loading time.
    bgm = load_audio("resources\\mythica.ogg");
    game_log("Start scene initialized");
    bgm_id = play_bgm(bgm, 1);
}

static void init_image() {
    img_background = load_bitmap_resized("resources\\start-bg.jpg", SCREEN_W, SCREEN_H);
    img_small_fighter = load_bitmap("resources\\smallfighter.png");
    img_small_fighter_broken1 = load_bitmap("resources\\smallfighter_broken1.png");
    img_small_fighter_broken2 = load_bitmap("resources\\smallfighter_broken2.png");
    img_middle_fighter = load_bitmap("resources\\rocket-3.png");
    img_middle_bullet = load_bitmap("resources\\missile.png");
    img_plane_bullet1 = load_bitmap("resources\\bullet.png");
    img_plane_bullet2 = load_bitmap("resources\\bullet2.png");
    img_plane1 = load_bitmap("resources\\plane.png");
    img_plane2 = load_bitmap("resources\\plane2.png");
    img_avatar1 = load_bitmap_resized("resources\\plane.png", 30, 30);
    img_avatar2 = load_bitmap_resized("resources\\plane2.png", 30, 30);
    img_heart = load_bitmap("resources\\hearts.png");
}

static void init_hitbox() {
    unsigned char pixel, _;

    al_lock_bitmap(img_plane1, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READWRITE);
    al_lock_bitmap(img_plane2, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READWRITE);
    al_lock_bitmap(img_small_fighter, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READWRITE);
    al_lock_bitmap(img_middle_fighter, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READWRITE);
    al_lock_bitmap(img_plane_bullet1, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READWRITE);
    al_lock_bitmap(img_plane_bullet2, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READWRITE);
    al_lock_bitmap(img_middle_bullet, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READWRITE);

    for (int x = 0; x < al_get_bitmap_width(img_plane1); x++) {
        for (int y = 0; y < al_get_bitmap_height(img_plane1); y++) {
            al_unmap_rgba(al_get_pixel(img_plane1, x, y), &_, &_, &_, &pixel);
            hitbox_plane[0][x][y] = (pixel != 0);
        }
    }
    for (int x = 0; x < al_get_bitmap_width(img_plane2); x++) {
        for (int y = 0; y < al_get_bitmap_height(img_plane2); y++) {
            al_unmap_rgba(al_get_pixel(img_plane2, x, y), &_, &_, &_, &pixel);
            hitbox_plane[1][x][y] = (pixel != 0);
        }
    }
    for (int x = 0; x < al_get_bitmap_width(img_small_fighter); x++) {
        for (int y = 0; y < al_get_bitmap_height(img_small_fighter); y++) {
            al_unmap_rgba(al_get_pixel(img_small_fighter, x, y), &_, &_, &_, &pixel);
            hitbox_small_fighter[x][y] = (pixel != 0);
        }
    }
    for (int x = 0; x < al_get_bitmap_width(img_middle_fighter); x++) {
        for (int y = 0; y < al_get_bitmap_height(img_middle_fighter); y++) {
            al_unmap_rgba(al_get_pixel(img_middle_fighter, x, y), &_, &_, &_, &pixel);
            hitbox_middle_fighter[x][y] = (pixel != 0);
        }
    }
    for (int x = 0; x < al_get_bitmap_width(img_plane_bullet1); x++) {
        for (int y = 0; y < al_get_bitmap_height(img_plane_bullet1); y++) {
            al_unmap_rgba(al_get_pixel(img_plane_bullet1, x, y), &_, &_, &_, &pixel);
            hitbox_plane_bullet[0][x][y] = (pixel != 0);
        }
    }
    for (int x = 0; x < al_get_bitmap_width(img_plane_bullet2); x++) {
        for (int y = 0; y < al_get_bitmap_height(img_plane_bullet2); y++) {
            al_unmap_rgba(al_get_pixel(img_plane_bullet2, x, y), &_, &_, &_, &pixel);
            hitbox_plane_bullet[1][x][y] = (pixel != 0);
        }
    }
    for (int x = 0; x < al_get_bitmap_width(img_middle_bullet); x++) {
        for (int y = 0; y < al_get_bitmap_height(img_middle_bullet); y++) {
            al_unmap_rgba(al_get_pixel(img_middle_bullet, x, y), &_, &_, &_, &pixel);
            hitbox_middle_bullet[x][y] = (pixel != 0);
        }
    }

    al_unlock_bitmap(img_plane1);
    al_unlock_bitmap(img_plane2);
    al_unlock_bitmap(img_small_fighter);
    al_unlock_bitmap(img_middle_fighter);
    al_unlock_bitmap(img_plane_bullet1);
    al_unlock_bitmap(img_plane_bullet2);
    al_unlock_bitmap(img_middle_bullet);
}

static void init_plane() {
    plane[0].obj.hidden = true;
    plane[1].obj.hidden = true;
    if (multiplayer) {
        plane[0].obj.img[0] = img_plane1;
        plane[0].obj.x = 300;
        plane[0].obj.y = 500;
        plane[0].obj.w = al_get_bitmap_width(plane[0].obj.img[0]);
        plane[0].obj.h = al_get_bitmap_height(plane[0].obj.img[0]);
        plane[0].obj.theta = 0;
        plane[0].obj.hidden = false;
        plane[0].type = 0;
        plane[0].max_health = 5;
        plane[0].health = plane[0].max_health;

        plane[1].obj.img[0] = img_plane2;
        plane[1].obj.x = 500;
        plane[1].obj.y = 500;
        plane[1].obj.w = al_get_bitmap_width(plane[1].obj.img[0]);
        plane[1].obj.h = al_get_bitmap_height(plane[1].obj.img[0]);
        plane[1].obj.theta = 0;
        plane[1].obj.hidden = false;
        plane[1].type = 1;
        plane[1].max_health = 5;
        plane[1].health = plane[1].max_health;
    }
    else {
        plane[selected_plane].obj.img[0] = (selected_plane == 0) ? img_plane1 : img_plane2;
        plane[selected_plane].obj.x = 400;
        plane[selected_plane].obj.y = 500;
        plane[selected_plane].obj.w = al_get_bitmap_width(plane[selected_plane].obj.img[0]);
        plane[selected_plane].obj.h = al_get_bitmap_height(plane[selected_plane].obj.img[0]);
        plane[selected_plane].obj.theta = 0;
        plane[selected_plane].obj.hidden = false;
        plane[selected_plane].type = selected_plane;
        plane[selected_plane].max_health = 5;
        plane[selected_plane].health = plane[selected_plane].max_health;
    }

}

static void init_sound() {
    if (sound_laser == NULL)
        sound_laser = al_load_sample("resources\\laser.mp3");
    if (sound_gun == NULL)
        sound_gun = al_load_sample("resources\\gun.mp3");
    if (sound_explosion == NULL)
        sound_explosion = al_load_sample("resources\\explosion.mp3");
    if (sound_metal == NULL)
        sound_metal = al_load_sample("resources\\metal.mp3");
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
    //type 0: plane bullets 1, 1: plane bullets 2, 2: mid fighter bullets
    switch (type) {
    case 0:
        bullet->obj.img[0] = img_plane_bullet1;
        bullet->obj.vx = 0;
        bullet->obj.vy = -20;
        break;

    case 1:
        bullet->obj.img[0] = img_plane_bullet2;
        bullet->obj.vx = 0;
        bullet->obj.vy = -10;
        break;

    case 2:
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
char collision_box1[max_len][max_len];
char collision_box2[max_len][max_len];
int collision_detect(MovableObject obj1, MovableObject obj2, char hitbox1[][101], char hitbox2[][101]) {
    if (sqrt(pow(obj1.x-obj2.x, 2)+pow(obj1.y-obj2.y, 2)) <= max(obj1.x,obj1.y) + max(obj2.x,obj2.y)) {

        unsigned char pixel, _;
        memset(collision_box1, 0, sizeof(char) * max_len * max_len);
        memset(collision_box2, 0, sizeof(char) * max_len * max_len);

        for (int x = 0; x < obj1.w; x++) {
            for (int y = 0; y < obj1.h; y++) {
                collision_box1[(int)((x - obj1.w / 2) * cos((double)obj1.theta) - (y - obj1.h / 2) * sin((double)obj1.theta) + (max_len-1)/2+1)][(int)((x - obj1.w / 2) * sin((double)obj1.theta) + (y - obj1.h / 2) * cos((double)obj1.theta) + (max_len-1)/2+1)] = hitbox1[x][y];
            }
        }
        for (int x = 0; x < obj2.w; x++) {
            for (int y = 0; y < obj2.h; y++) {
                collision_box2[(int)((x - obj2.w / 2) * cos((double)obj2.theta) - (y - obj2.h / 2) * sin((double)obj2.theta) + (max_len-1)/2+1)][(int)((x - obj2.w / 2) * sin((double)obj2.theta) + (y - obj2.h / 2) * cos((double)obj2.theta) + (max_len-1)/2+1)] = hitbox2[x][y];
            }
        }

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
        if (obj->x - obj->w / 2 < 0) {
            obj->x = obj->w / 2;
            obj->vx = 0;
        }
    }
    else if (restrict_in_LRUD[0] == 1);
    else if (restrict_in_LRUD[0] == 2){
        if (obj->x + obj->w / 2 < 0) {
            obj->hidden = true;
            return;
        }
    }
    if (restrict_in_LRUD[1] == 0) {
        if (obj->x + obj->w / 2 > SCREEN_W) {
            obj->x = SCREEN_W - obj->w / 2;
            obj->vx = 0;
        }
    }
    else if (restrict_in_LRUD[1] == 1);
    else if (restrict_in_LRUD[1] == 2) {
        if (obj->x - obj->w / 2 > SCREEN_W) {
            obj->hidden = true;
            return;
        }
    }
    if (restrict_in_LRUD[2] == 0) {
        if (obj->y - obj->h / 2 < 0) {
            obj->y = obj->h / 2;
            obj->vy = 0;
        }
    }
    else if (restrict_in_LRUD[2] == 1);
    else if (restrict_in_LRUD[2] == 2) {
        if (obj->y + obj->h / 2 < 0) {
            obj->hidden = true;
            return;
        }
    }
    if (restrict_in_LRUD[3] == 0) {
        if (obj->y + obj->h / 2 > SCREEN_H) {
            obj->y = SCREEN_H - obj->h / 2;
            obj->vy = 0;
        }
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
    //win?
    if (score >= 500)
        game_change_scene(scene_win_create());
    //collision detect
    int i, j, k;

    for (i = 0; i < MAX_SMALL_ENEMY; i++) {
        if (small_enemies[i].obj.hidden)
            continue;
        for (j = plane_start; j < plane_end; j++) {
            if (plane[j].obj.hidden)
                continue;
            if (collision_detect(plane[j].obj, small_enemies[i].obj, hitbox_plane[j], hitbox_middle_fighter)) {
                play_audio(sound_explosion, 1);
                plane[j].health--;
                score -= 50;
                small_enemies[i].obj.hidden = true;
            }
        }
    }
    for (i = 0; i < MAX_MIDDLE_ENEMY; i++) {
        if (middle_enemies[i].obj.hidden)
            continue;
        for (j = plane_start; j < plane_end; j++) {
            if (plane[j].obj.hidden)
                continue;
            if (collision_detect(plane[j].obj, middle_enemies[i].obj, hitbox_plane[j], hitbox_middle_fighter)) {
                play_audio(sound_explosion, 1);
                plane[j].health--;
                score -= 50;
                middle_enemies[i].obj.hidden = true;
            }
        }
    }
    for (i = 0; i < MAX_MIDDLE_BULLET; i++) {
        if (middle_bullets[i].obj.hidden)
            continue;
        for (j = plane_start; j < plane_end; j++) {
            if (plane[j].obj.hidden)
                continue;
            if (collision_detect(plane[j].obj, middle_bullets[i].obj, hitbox_plane[j], hitbox_middle_bullet)) {
                play_audio(sound_explosion, 1);
                plane[j].health--;
                score -= 50;
                middle_bullets[i].obj.hidden = true;
            }
        }
    }
    for (i = plane_start; i < plane_end; i++) {
        if (plane[i].health <= 0)
            plane[i].obj.hidden = true;
    }
    for (i = plane_start; i < plane_end; i++) {
        if (!plane[i].obj.hidden)
            break;
    }
    if (i == plane_end)
        game_change_scene(scene_dead_create());

    for (i = 0; i < MAX_PLANE_BULLET; i++) {
        for (k = plane_start; k < plane_end; k++) {
            if (plane_bullets[k][i].obj.hidden)
                continue;
            for (j = 0; j < MAX_SMALL_ENEMY; j++) {
                if (small_enemies[j].obj.hidden)
                    continue;
                if (collision_detect(plane_bullets[k][i].obj, small_enemies[j].obj, hitbox_plane_bullet[k], hitbox_small_fighter)) {
                    small_enemies[j].health -= ((k == 1) ? small_enemies[j].max_health : 1);
                    if (k == 0) {
                        play_audio(sound_metal, 1);
                        plane_bullets[k][i].obj.hidden = true;
                    }
                    if (small_enemies[j].health <= 0) {
                        play_audio(sound_explosion, 1);
                        small_enemies[j].obj.hidden = true;
                        score += 10;
                    }
                }
            }
            if (plane_bullets[k][i].obj.hidden)
                continue;
            for (j = 0; j < MAX_MIDDLE_ENEMY; j++) {
                if (middle_enemies[j].obj.hidden)
                    continue;
                if (collision_detect(plane_bullets[k][i].obj, middle_enemies[j].obj, hitbox_plane_bullet[k], hitbox_middle_fighter)) {
                    middle_enemies[j].health--;
                    if (k == 0)
                        plane_bullets[k][i].obj.hidden = true;
                    if (middle_enemies[j].health == 0) {
                        play_audio(sound_explosion, 1);
                        middle_enemies[j].obj.hidden = true;
                        score += 30;
                    }
                }
            }
            if (plane_bullets[k][i].obj.hidden)
                continue;
            for (j = 0; j < MAX_MIDDLE_BULLET; j++) {
                if (middle_bullets[j].obj.hidden)
                    continue;
                if (collision_detect(plane_bullets[k][i].obj, middle_bullets[j].obj, hitbox_plane_bullet[k], hitbox_middle_bullet)) {
                    if (k == 0)
                        plane_bullets[k][i].obj.hidden = true;
                    play_audio(sound_explosion, 1);
                    middle_bullets[j].obj.hidden = true;
                }
            }
        }
    }

    //create and move objects
    if (multiplayer) {
        plane[0].obj.vx = plane[0].obj.vy = plane[1].obj.vx = plane[1].obj.vy = 0;
        if (key_state[ALLEGRO_KEY_W])
            plane[0].obj.vy -= 1;
        if (key_state[ALLEGRO_KEY_S])
            plane[0].obj.vy += 1;
        if (key_state[ALLEGRO_KEY_A])
            plane[0].obj.vx -= 1;
        if (key_state[ALLEGRO_KEY_D])
            plane[0].obj.vx += 1;
        if (key_state[ALLEGRO_KEY_UP])
            plane[1].obj.vy -= 1;
        if (key_state[ALLEGRO_KEY_DOWN])
            plane[1].obj.vy += 1;
        if (key_state[ALLEGRO_KEY_LEFT])
            plane[1].obj.vx -= 1;
        if (key_state[ALLEGRO_KEY_RIGHT])
            plane[1].obj.vx += 1;

        plane[0].obj.vx *= 7 * (plane[0].obj.vy ? 0.71f : 1);
        plane[0].obj.vy *= 7 * (plane[0].obj.vx ? 0.71f : 1);
        plane[1].obj.vx *= 7 * (plane[1].obj.vy ? 0.71f : 1);
        plane[1].obj.vy *= 7 * (plane[1].obj.vx ? 0.71f : 1);
        move_object(&plane[0].obj, (int[]) { 0, 0, 0, 0 });
        move_object(&plane[1].obj, (int[]) { 0, 0, 0, 0 });
    }
    else {
        plane[selected_plane].obj.vx = plane[selected_plane].obj.vy = 0;
        if (key_state[ALLEGRO_KEY_UP] || key_state[ALLEGRO_KEY_W])
            plane[selected_plane].obj.vy -= 1;
        if (key_state[ALLEGRO_KEY_DOWN] || key_state[ALLEGRO_KEY_S])
            plane[selected_plane].obj.vy += 1;
        if (key_state[ALLEGRO_KEY_LEFT] || key_state[ALLEGRO_KEY_A])
            plane[selected_plane].obj.vx -= 1;
        if (key_state[ALLEGRO_KEY_RIGHT] || key_state[ALLEGRO_KEY_D])
            plane[selected_plane].obj.vx += 1;

        plane[selected_plane].obj.vx *= 7 * (plane[selected_plane].obj.vy ? 0.71f : 1);
        plane[selected_plane].obj.vy *= 7 * (plane[selected_plane].obj.vx ? 0.71f : 1);
        move_object(&plane[selected_plane].obj, (int[]) { 0, 0, 0, 0 });
    }

    for (i = plane_start; i < plane_end; i++) {
        for (j = 0; j < MAX_PLANE_BULLET; j++) {
            if (plane_bullets[i][j].obj.hidden)
                continue;
            move_object(&plane_bullets[i][j].obj, (int[]) { 2, 2, 2, 2 });
        }
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
        int closest = 0;
        if (multiplayer) {
            float closest_distant = SCREEN_W * SCREEN_W + SCREEN_H * SCREEN_H + 100;
            float distant;
            for (j = 0; j < 2; j++) {
                if (plane[j].obj.hidden)
                    continue;
                distant = pow(plane[j].obj.x - middle_enemies[i].obj.x, 2) + pow(plane[j].obj.y - middle_enemies[i].obj.y, 2);
                if (distant < closest_distant) {
                    closest = j;
                    closest_distant = distant;
                }
            }
        }
        else
            closest = selected_plane;

        middle_enemies[i].obj.theta = atan2((double)plane[closest].obj.y - (double)middle_enemies[i].obj.y, (double)plane[closest].obj.x - (double)middle_enemies[i].obj.x + 0.001);
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

        int closest = 0;
        if (multiplayer) {
            float closest_distant = SCREEN_W * SCREEN_W + SCREEN_H * SCREEN_H + 100;
            float distant;
            for (j = 0; j < 2; j++) {
                if (plane[j].obj.hidden)
                    continue;
                distant = pow(plane[j].obj.x - middle_enemies[i].obj.x, 2) + pow(plane[j].obj.y - middle_enemies[i].obj.y, 2);
                if (distant < closest_distant) {
                    closest = j;
                    closest_distant = distant;
                }
            }
        }
        else
            closest = selected_plane;

        double new_angle = atan2((double)plane[closest].obj.y - (double)middle_bullets[i].obj.y, (double)plane[closest].obj.x - (double)middle_bullets[i].obj.x + 0.00001);
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
    if (multiplayer) {
        int shoot_keys[2] = { ALLEGRO_KEY_SPACE, ALLEGRO_KEY_RSHIFT };
        for (i = 0; i < 2; i++) {
            if (key_state[shoot_keys[i]] && now - last_shoot_timestamp[i] >= MAX_COOLDOWN[i] && !plane[i].obj.hidden) {
                for (j = 0; j < MAX_PLANE_BULLET; j++) {
                    if (plane_bullets[i][j].obj.hidden) {
                        last_shoot_timestamp[i] = now;
                        plane_bullets[i][j].obj.hidden = false;
                        plane_bullets[i][j].obj.x = plane[i].obj.x;
                        plane_bullets[i][j].obj.y = plane[i].obj.y;
                        plane_bullets[i][j].obj.theta = -PI / 2;
                        plane_bullets[i][j].obj.vx = plane[i].obj.vx;
                        plane_bullets[i][j].obj.vy = -10 * (2 - i) + plane[i].obj.vy;
                        if (i == 0)
                            play_audio(sound_gun, 0.2);
                        else if (i == 1)
                            play_audio(sound_laser, 1);
                        break;
                    }
                }
            }
        }
    }
    else {
        if (key_state[ALLEGRO_KEY_SPACE] && now - last_shoot_timestamp[selected_plane] >= MAX_COOLDOWN[selected_plane]) {
            for (j = 0; j < MAX_PLANE_BULLET; j++) {
                if (plane_bullets[selected_plane][j].obj.hidden) {
                    last_shoot_timestamp[selected_plane] = now;
                    plane_bullets[selected_plane][j].obj.hidden = false;
                    plane_bullets[selected_plane][j].obj.x = plane[selected_plane].obj.x;
                    plane_bullets[selected_plane][j].obj.y = plane[selected_plane].obj.y;
                    plane_bullets[selected_plane][j].obj.theta = -PI / 2;
                    plane_bullets[selected_plane][j].obj.vx = plane[selected_plane].obj.vx;
                    plane_bullets[selected_plane][j].obj.vy = -10 * (2 - selected_plane) + plane[selected_plane].obj.vy;
                    if (selected_plane == 0)
                        play_audio(sound_gun, 0.2);
                    else if (selected_plane == 1)
                        play_audio(sound_laser, 1);
                    break;
                }
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
    int i, j;
    al_draw_bitmap(img_background, 0, 0, 0);
    if (multiplayer) {
        al_draw_bitmap(img_avatar1, SCREEN_W - 40 - plane[0].max_health * 30, 15, 0);
        al_draw_bitmap(img_avatar2, SCREEN_W - 40 - plane[0].max_health * 30, 55, 0);
        for (i = 0; i < plane[0].health; i++)
            al_draw_bitmap(img_heart, SCREEN_W - 30 - i * 30, 20, 0);
        for (i = 0; i < plane[1].health; i++)
            al_draw_bitmap(img_heart, SCREEN_W - 30 - i * 30, 60, 0);
    }
    else {
        for (i = 0; i < plane[selected_plane].health; i++)
            al_draw_bitmap(img_heart, SCREEN_W - 30 - i * 30, 10, 0);
    }
    for (i = plane_start; i < plane_end; i++) {
        for (j = 0; j < MAX_PLANE_BULLET; j++)
            draw_movable_object(plane_bullets[i][j].obj, 0);
        draw_movable_object(plane[i].obj, 0);
    }
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
    al_destroy_bitmap(img_plane1);
    al_destroy_bitmap(img_plane2);
    al_destroy_bitmap(img_small_fighter);
    al_destroy_bitmap(img_small_fighter_broken1);
    al_destroy_bitmap(img_small_fighter_broken2);
    al_destroy_bitmap(img_plane_bullet1);
    al_destroy_bitmap(img_plane_bullet2);
    al_destroy_bitmap(img_middle_bullet);
    al_destroy_bitmap(img_middle_fighter);
    al_destroy_bitmap(img_heart);
    stop_bgm(bgm_id);
    al_destroy_sample(bgm);
    game_log("Start scene destroyed");
}

static void on_key_down(int keycode) {
    if (keycode == ALLEGRO_KEY_TAB)
        draw_gizmos = !draw_gizmos;
}

static void on_mouse_down(int btn, int x, int y, int dz) {
    if (btn == mouse_state[1]) {
        double now = al_get_time();
        for (int i = plane_start; i < plane_end; i++) {
            if (now - last_shoot_timestamp[i] >= MAX_COOLDOWN[i] && !plane[i].obj.hidden) {
                for (int j = 0; j < MAX_PLANE_BULLET; j++) {
                    if (plane_bullets[i][j].obj.hidden) {
                        last_shoot_timestamp[i] = now;
                        plane_bullets[i][j].obj.hidden = false;
                        plane_bullets[i][j].obj.x = plane[i].obj.x;
                        plane_bullets[i][j].obj.y = plane[i].obj.y;
                        plane_bullets[i][j].obj.theta = atan2(y - plane[i].obj.y, x - plane[i].obj.x);
                        plane_bullets[i][j].obj.vx = 10 * (2-i) * cos(plane_bullets[i][j].obj.theta);
                        plane_bullets[i][j].obj.vy = 10 * (2-i) * sin(plane_bullets[i][j].obj.theta);
                        if (i == 0)
                            play_audio(sound_gun, 0.3);
                        else if (i == 1)
                            play_audio(sound_laser, 1);
                        break;
                    }
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