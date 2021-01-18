#include "scene_menu.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_acodec.h>
#include "game.h"
#include "utility.h"
#include "shared.h"
#include "scene_start.h"
#include "scene_settings.h"

// Variables and functions with 'static' prefix at the top level of a
// source file is only accessible in that file ("file scope", also
// known as "internal linkage"). If other files has the same variable
// name, they'll be different variables.

/* Define your static vars / function prototypes below. */

// TODO: More variables and functions that will only be accessed
// inside this scene. They should all have the 'static' prefix.

static const char* txt_title = "Space Shooter";
static const char* txt_info = "Press enter key to start";
static ALLEGRO_BITMAP* img_background;
static ALLEGRO_BITMAP* img_settings;
static ALLEGRO_BITMAP* img_settings2;

static ALLEGRO_SAMPLE* sound_click;
static ALLEGRO_SAMPLE* bgm;
static ALLEGRO_SAMPLE_ID bgm_id;


int cont_bgm;

static void init(void);
static void draw(void);
static void destroy(void);
static void on_key_down(int keycode);

static void init(void) {
    img_background = load_bitmap_resized("resources\\main-bg.jpg", SCREEN_W, SCREEN_H);
    img_settings = load_bitmap("resources\\settings.png");
    img_settings2 = load_bitmap("resources\\settings2.png");

    if (sound_click == NULL)
        sound_click = load_audio("resources\\click.mp3");
    if (!cont_bgm) {
        bgm = load_audio("resources\\S31-Night Prowler.ogg");
        bgm_id = play_bgm(bgm, 1);
    }
    cont_bgm = 0;
    game_log("Menu scene initialized");
}

static void draw(void) {
    al_draw_bitmap(img_background, 0, 0, 0);
    if (pnt_in_rect(mouse_x, mouse_y, SCREEN_W-48, 10, 38, 38))
        al_draw_bitmap(img_settings2, SCREEN_W - 48, 10, 0);
    else
        al_draw_bitmap(img_settings, SCREEN_W - 48, 10, 0);
    al_draw_text(font_pirulen_32, al_map_rgb(255, 255, 255), SCREEN_W / 2, 30, ALLEGRO_ALIGN_CENTER, txt_title);
    al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), 20, SCREEN_H - 50, 0, txt_info);
}

static void destroy(void) {
    al_destroy_bitmap(img_background);
    al_destroy_bitmap(img_settings);
    al_destroy_bitmap(img_settings2);
    if (!cont_bgm) {
        al_destroy_sample(bgm);
        stop_bgm(bgm_id);
    }
    game_log("Menu scene destroyed");
}

static void on_key_down(int keycode) {
    if (keycode == ALLEGRO_KEY_ENTER)
        game_change_scene(scene_start_create());
}

static void on_mouse_down(int btn, int x, int y, int dz) {
    if (btn == mouse_state[1]) {
        if (pnt_in_rect(x, y, SCREEN_W - 48, 10, 38, 38)) {
            play_audio(sound_click, 1);
            cont_bgm = 1;
            game_change_scene(scene_settings_create());
        }
    }
}

Scene scene_menu_create(void) {
    Scene scene;
    memset(&scene, 0, sizeof(Scene));
    scene.name = "Menu";
    scene.initialize = &init;
    scene.draw = &draw;
    scene.destroy = &destroy;
    scene.on_key_down = &on_key_down;
    scene.on_mouse_down = &on_mouse_down;
    game_log("Menu scene created");
    return scene;
}
