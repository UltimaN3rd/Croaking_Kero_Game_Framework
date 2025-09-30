#include "framework.h"

#include "cereal.c"
#define EXPLORER_PRINT_ERRORS
#define FOLDERS_PRINT_ERRORS
#include "explorer.c"
#include "folders.c"
#include "log.c"
#include "menu.c"
#include "utilities.c"
#include "render.c"
#include "samples.c"
#include "sprite.c"
#include "update.c"

// The following are not included because they must be built separately on Mac as OBJC
// #include "osinterface.c"
// #include "sound.c"