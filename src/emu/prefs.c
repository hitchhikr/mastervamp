// -------------------------------------------------------
// Includes
#include <stdio.h>
#include "prefs.h"

// -------------------------------------------------------
// Save the preferences into a file
void prefs_save(const char *filename)
{
    FILE *cfg = fopen(filename, "wb");
    if(cfg)
    {
        fwrite(&prefs, 1, sizeof(PREFERENCES), cfg);
        fclose(cfg);
    }
}

// -------------------------------------------------------
// Load the preferences from a file
void prefs_load(const char *filename)
{
    // Load our config file
    FILE *cfg = fopen(filename, "rb");
    if(cfg)
    {
        fread(&prefs, 1, sizeof(PREFERENCES), cfg);
        fclose(cfg);
    }
}
