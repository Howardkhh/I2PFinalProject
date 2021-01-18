#include "scene_menu.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_acodec.h>
#include "game.h"
#include "utility.h"
#include "shared.h"
#include "scene_start.h"

static ALLEGRO_BITMAP* img_backgroud;
static ALLEGRO_SAMPLE* bgm;
static ALLEGRO_SAMPLE_ID bgm_id;
static int alpha, sign;
float now;

static void init(void) {
    now = al_get_time();
    img_backgroud = load_bitmap_resized("resources\\dead-bg.jpg", SCREEN_W, SCREEN_H);
    bgm = load_audio("resources\\dead_music.mp3");
    bgm_id = play_bgm(bgm, 1);
    alpha = 0;
    sign = 5;
}

static void draw(void) {
    alpha += sign;
    if (alpha >= 254 || alpha <= 1)
        sign = -sign;
    al_draw_bitmap(img_backgroud, 0, 0, 0);
    al_draw_text(font_pirulen_100, al_map_rgb(0, 0, 0), 400, 100, ALLEGRO_ALIGN_CENTRE, "You Died");
    al_draw_textf(font_pirulen_32, al_map_rgb(0, 0, 0), 400, 250, ALLEGRO_ALIGN_CENTRE, "Score: %d", score);
    al_draw_textf(font_pirulen_32, al_map_rgb(0, 0, 0), 400, 300, ALLEGRO_ALIGN_CENTRE, "Survival Time: %d:%d:%d", (int)((now - start_time) / 60), ((int)(now - start_time) % 60), (int)((now - start_time) * 1000) % 1000);
    al_draw_text(font_pirulen_24, al_map_rgba(0, 0, 0, alpha), 400, 500, ALLEGRO_ALIGN_CENTRE, "Press Enter to Continue");
}

static void destroy(void) {
    al_destroy_bitmap(img_backgroud);
    stop_bgm(bgm_id);
    al_destroy_sample(bgm);
}

static void on_key_down(int key_code) {
    if (key_code == ALLEGRO_KEY_ENTER)
        game_change_scene(scene_menu_create());
}

Scene scene_dead_create(void) {
    Scene scene;
    memset(&scene, 0, sizeof(Scene));
    scene.name = "Dead";
    scene.initialize = &init;
    scene.draw = &draw;
    scene.destroy = &destroy;
    scene.on_key_down = &on_key_down;
    game_log("Dead scene created");
    return scene;
}