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
//extern int score;
//extern float start_time;
float now;

static void init(void) {
    img_backgroud = load_bitmap_resized("resources\\dead-bg.jpg", SCREEN_W, SCREEN_H);
    now = al_get_time();
}

static void draw(void) {
    al_draw_bitmap(img_backgroud, 0, 0, 0);
    al_draw_text(font_pirulen_100, al_map_rgb(0, 0, 0), 400, 100, ALLEGRO_ALIGN_CENTRE, "You Died");
    al_draw_textf(font_pirulen_32, al_map_rgb(0, 0, 0), 400, 250, ALLEGRO_ALIGN_CENTRE, "Score: %d", score);
    al_draw_textf(font_pirulen_32, al_map_rgb(0, 0, 0), 400, 300, ALLEGRO_ALIGN_CENTRE, "Survival Time: %d:%d:%d", (int)((now - start_time) / 60), ((int)(now - start_time) % 60), (int)((now - start_time) * 1000) % 1000);
    al_draw_text(font_pirulen_24, al_map_rgb(0, 0, 0), 400, 500, ALLEGRO_ALIGN_CENTRE, "Press Enter to Continue");
}

static void destroy(void) {
    al_destroy_bitmap(img_backgroud);
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