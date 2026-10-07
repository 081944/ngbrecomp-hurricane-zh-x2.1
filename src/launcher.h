/*
 * launcher.h -- the settings screen shown before the game, and the settings
 * file behind it ($XDG_CONFIG_HOME/NinjaGaidenBlack/settings.ini).
 */
#ifndef NGB_LAUNCHER_H
#define NGB_LAUNCHER_H

enum { ASPECT_4_3, ASPECT_16_9, ASPECT_16_10, ASPECT_STRETCH, ASPECT_COUNT };

typedef struct NgbSettings {
    int aspect;          /* ASPECT_* */
    int width, height;   /* window size */
    int fullscreen;
    int scale;           /* render scale 1..4 */
    int vsync;
    int skip_intro;
    int show_launcher;   /* show the settings screen at startup */
} NgbSettings;

void ngb_settings_load(NgbSettings *s);
void ngb_settings_save(const NgbSettings *s);

/* Width/height of the aspect, for the presenter (negative: stretch). */
double ngb_settings_aspect_ratio(const NgbSettings *s);

/* Runs the settings screen in its own window. Returns 1 to start the game,
 * 0 to quit. Saves the settings when the game is started. */
int ngb_launcher_run(NgbSettings *s);

#endif
