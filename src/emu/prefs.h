#ifndef _PREFS_H_
#define _PREFS_H_

// -------------------------------------------------------
// Structures
typedef struct __attribute__((packed))
{
    char input_0[4];
    char input_1[4];
    char frequency;
    short audio_volume;
    char col_input[12];
    char vsync;
} PREFERENCES, *LPPREFERENCES; 

// -------------------------------------------------------
// Variables
extern PREFERENCES prefs;

// -------------------------------------------------------
// Functions
void prefs_save(const char *filename);
void prefs_load(const char *filename);

#endif /* _PREFS_H_ */
