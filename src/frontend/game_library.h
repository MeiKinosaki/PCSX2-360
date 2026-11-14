#ifndef PCSX2_360_GAME_LIBRARY_H
#define PCSX2_360_GAME_LIBRARY_H

#include <stddef.h>
#include <stdint.h>

#define PCSX2_GAME_LIBRARY_MAX 256u
#define PCSX2_GAME_TITLE_MAX  96u
#define PCSX2_GAME_SERIAL_MAX 16u
#define PCSX2_GAME_GENRE_MAX  32u
#define PCSX2_GAME_PATH_MAX   260u

typedef struct pcsx2_game_entry {
    char title[PCSX2_GAME_TITLE_MAX];
    char serial[PCSX2_GAME_SERIAL_MAX];
    char genre[PCSX2_GAME_GENRE_MAX];
    char image_path[PCSX2_GAME_PATH_MAX];
    char cover_path[PCSX2_GAME_PATH_MAX];
    uint8_t favorite;
} pcsx2_game_entry_t;

typedef struct pcsx2_game_library {
    pcsx2_game_entry_t entries[PCSX2_GAME_LIBRARY_MAX];
    size_t count;
} pcsx2_game_library_t;

typedef struct pcsx2_game_filter {
    const char *text;
    const char *genre;
    int favorites_only;
} pcsx2_game_filter_t;

void pcsx2_game_library_init(pcsx2_game_library_t *library);
int pcsx2_game_library_add(pcsx2_game_library_t *library,
                           const pcsx2_game_entry_t *entry);
int pcsx2_game_library_set_favorite(pcsx2_game_library_t *library,
                                   const char *serial, int favorite);
size_t pcsx2_game_library_filter(const pcsx2_game_library_t *library,
                                 const pcsx2_game_filter_t *filter,
                                 size_t *indices, size_t capacity);

#endif
