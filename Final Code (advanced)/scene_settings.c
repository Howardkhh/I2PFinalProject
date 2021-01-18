#include <allegro5/allegro_primitives.h>
#include "scene_settings.h"
#include "scene_menu.h"
#include "utility.h"
#include "shared.h"

static ALLEGRO_BITMAP* img_backgroud;
static ALLEGRO_BITMAP* img_back1;
static ALLEGRO_BITMAP* img_back2;
static ALLEGRO_BITMAP* img_plane1;
static ALLEGRO_BITMAP* img_plane2;
static ALLEGRO_BITMAP* img_bullet1;
static ALLEGRO_BITMAP* img_bullet2;
static ALLEGRO_BITMAP* img_box_blue;
static ALLEGRO_BITMAP* img_box_red;
static ALLEGRO_BITMAP* img_1p;
static ALLEGRO_BITMAP* img_2p;

static ALLEGRO_SAMPLE* sound_click;
static ALLEGRO_SAMPLE* sound_switch;

int selected_plane;
int multiplayer;
int cont_bgm;

static void init(void) {
    img_backgroud = load_bitmap_resized("resources\\settings_backgroud.jpg", SCREEN_W, SCREEN_H);
    img_back1 = load_bitmap("resources\\back.png");
    img_back2 = load_bitmap("resources\\back2.png");
    img_plane1 = load_bitmap_resized("resources\\plane.png", 200, 200);
    img_plane2 = load_bitmap_resized("resources\\plane2.png", 200, 200);
    img_bullet1 = load_bitmap_resized("resources\\bullet.png", 50, 25);
    img_bullet2 = load_bitmap_resized("resources\\bullet2.png", 50, 50);
    img_box_blue = load_bitmap("resources\\box_blue.png");
    img_box_red = load_bitmap("resources\\box_red.png");
    img_1p = load_bitmap("resources\\1P.png");
    img_2p = load_bitmap("resources\\2P.png");

    if (sound_click == NULL);
        sound_click = load_audio("resources\\click.mp3");
    sound_switch = load_audio("resources\\switch.mp3");
}

static void draw(void) {
    al_draw_bitmap(img_backgroud, 0, 0, 0);
    al_draw_bitmap(img_plane1, 150, 240, 0);
    al_draw_bitmap(img_plane2, 450, 240, 0);
    al_draw_rotated_bitmap(img_bullet1, 25, 12.5, 250, 200, 4.7123889803835, 0);
    al_draw_bitmap(img_bullet2, 525, 175, 0);
    if (multiplayer) {
        al_draw_bitmap(img_box_blue, 100, 150, 0);
        al_draw_bitmap(img_box_blue, 400, 150, 0);
    }
    else if (selected_plane == 0)
        al_draw_bitmap(img_box_blue, 100, 150, 0);
    else if (selected_plane == 1)
        al_draw_bitmap(img_box_blue, 400, 150, 0);
    
    if (!multiplayer)
        al_draw_bitmap(img_1p, 300, 50, 0);
    else
        al_draw_bitmap(img_2p, 300, 50, 0);

    if (pnt_in_rect(mouse_x, mouse_y, 10, 10, 38, 38))
        al_draw_bitmap(img_back1, 10, 10, 0);
    else
        al_draw_bitmap(img_back2, 10, 10, 0);

    if (pnt_in_rect(mouse_x, mouse_y, 140, 170, 220, 280))
        al_draw_bitmap(img_box_red, 100, 150, 0);
    if (pnt_in_rect(mouse_x, mouse_y, 440, 170, 220, 280))
        al_draw_bitmap(img_box_red, 400, 150, 0);

    al_draw_filled_rounded_rectangle(110, 485, 390, 535, 10, 10, al_map_rgb(0, 0, 0));
    al_draw_filled_rounded_rectangle(410, 485, 690, 535, 10, 10, al_map_rgb(0, 0, 0));
    al_draw_rounded_rectangle(110, 485, 390, 535, 10, 10, al_map_rgb(255, 255, 255), 5);
    al_draw_rounded_rectangle(410, 485, 690, 535, 10, 10, al_map_rgb(255, 255, 255), 5);
    al_draw_text(font_pirulen_18, al_map_rgb(255, 255, 255), 250, 500, ALLEGRO_ALIGN_CENTER, "Dark Obliterator");
    al_draw_text(font_pirulen_18, al_map_rgb(255, 255, 255), 550, 500, ALLEGRO_ALIGN_CENTER, "Deadly Crimson");
}

static void destroy(void) {
    al_destroy_bitmap(img_backgroud);
    al_destroy_bitmap(img_back1);
    al_destroy_bitmap(img_back2);
    al_destroy_bitmap(img_plane1);
    al_destroy_bitmap(img_plane2);
    al_destroy_bitmap(img_bullet1);
    al_destroy_bitmap(img_bullet2);
    al_destroy_bitmap(img_box_blue);
    al_destroy_bitmap(img_box_red);
    al_destroy_bitmap(img_1p);
    al_destroy_sample(sound_switch);
}

static void on_key_down(int keycode) {
    if (keycode == ALLEGRO_KEY_BACKSPACE) {
        play_audio(sound_click, 1);
        game_change_scene(scene_menu_create());
    }
}

static void on_mouse_down(int btn, int x, int y, int dz) {
    if (btn == mouse_state[1]) {
        if (pnt_in_rect(x, y, 10, 10, 38, 38)) {
            play_audio(sound_click, 1);
            game_change_scene(scene_menu_create());
        }
        else if (pnt_in_rect(x, y, 140, 170, 220, 280)) {
            play_audio(sound_switch, 1);
            selected_plane = 0;
        }
        else if (pnt_in_rect(x, y, 440, 170, 220, 280)) {
            play_audio(sound_switch, 1);
            selected_plane = 1;
        }
        else if (pnt_in_rect(x, y, 300, 50, 200, 50)) {
            play_audio(sound_switch, 1);
            multiplayer = !multiplayer;
        }
    }
}

Scene scene_settings_create(void) {
    Scene scene;
    memset(&scene, 0, sizeof(Scene));
    scene.initialize = &init;
    scene.name = "Start";
    scene.draw = &draw;
    scene.destroy = &destroy;
    scene.on_key_down = &on_key_down;
    scene.on_mouse_down = &on_mouse_down;
    game_log("Settings scene created");
    return scene;
}