#include "scene_settings.h"
#include "scene_menu.h"
#include "utility.h"

static ALLEGRO_BITMAP* img_backgroud;
static ALLEGRO_BITMAP* img_back1;
static ALLEGRO_BITMAP* img_back2;


static void init(void) {
    img_backgroud = load_bitmap_resized("resources\\settings_backgroud.jpg", SCREEN_W, SCREEN_H);
    img_back1 = load_bitmap("resources\\back.png");
    img_back2 = load_bitmap("resources\\back2.png");
}

static void draw(void) {
    al_draw_bitmap(img_backgroud, 0, 0, 0);
    if (pnt_in_rect(mouse_x, mouse_y, 10, 10, 38, 38))
        al_draw_bitmap(img_back1, 10, 10, 0);
    else
        al_draw_bitmap(img_back2, 10, 10, 0);
}

static void destroy(void) {
    al_destroy_bitmap(img_backgroud);
    al_destroy_bitmap(img_back1);
    al_destroy_bitmap(img_back2);
}

static void on_key_down(int keycode) {
    if (keycode == ALLEGRO_KEY_BACKSPACE)
        game_change_scene(scene_menu_create());
}

static void on_mouse_down(int btn, int x, int y, int dz) {
    if (btn == mouse_state[1]) {
        if (pnt_in_rect(x, y, 10, 10, 38, 38))
            game_change_scene(scene_menu_create());
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