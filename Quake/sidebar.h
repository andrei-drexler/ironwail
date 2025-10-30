#ifndef QUAKE_SIDEBAR_H
#define QUAKE_SIDEBAR_H

#include "q_stdinc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sidebar_menu_s {
    size_t count;
    const char **labels;
    const int *counts;
} sidebar_menu_t;

void Sidebar_Init(void);
void Sidebar_Shutdown(void);
void Sidebar_Setup(void);
void Sidebar_Clear(void);
void Sidebar_AddCount(const char *path, int count);
const char *Sidebar_GetFile(const char *path);
qboolean Sidebar_HasItems(void);
const sidebar_menu_t *Sidebar_GetMenu(void);

#ifdef __cplusplus
}
#endif

#endif /* QUAKE_SIDEBAR_H */
