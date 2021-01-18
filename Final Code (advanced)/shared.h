
#ifndef SCENE_SHARED_H
#define SCENE_SHARED_H
#include <allegro5/allegro_font.h>

extern ALLEGRO_FONT* font_pirulen_100;
extern ALLEGRO_FONT* font_pirulen_32;
extern ALLEGRO_FONT* font_pirulen_24;
extern ALLEGRO_FONT* font_pirulen_18;

extern int score;
extern float start_time;
extern int selected_plane;
extern int mutiplayer;
extern int cont_bgm;

void shared_init(void);
void shared_destroy(void);

#endif