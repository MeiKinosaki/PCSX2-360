#include "../src/frontend/game_library.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static pcsx2_game_entry_t game(const char *title, const char *serial,
                              const char *genre, const char *cover)
{
    pcsx2_game_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    (void)snprintf(entry.title, sizeof(entry.title), "%s", title);
    (void)snprintf(entry.serial, sizeof(entry.serial), "%s", serial);
    (void)snprintf(entry.genre, sizeof(entry.genre), "%s", genre);
    (void)snprintf(entry.cover_path, sizeof(entry.cover_path), "%s", cover);
    return entry;
}

int main(void)
{
    pcsx2_game_library_t library;
    pcsx2_game_entry_t a = game("God of War", "SCUS-97399", "Action", "covers/gow.png");
    pcsx2_game_entry_t b = game("Gran Turismo 4", "SCUS-97328", "Racing", "covers/gt4.png");
    size_t indices[4] = {99u, 99u, 99u, 99u};
    pcsx2_game_filter_t filter = {NULL, NULL, 0};

    pcsx2_game_library_init(&library);
    assert(pcsx2_game_library_add(&library, &a) == 0);
    assert(pcsx2_game_library_add(&library, &b) == 0);
    assert(pcsx2_game_library_add(&library, &a) == -2);
    assert(library.count == 2u);

    filter.genre = "action";
    assert(pcsx2_game_library_filter(&library, &filter, indices, 4u) == 1u);
    assert(indices[0] == 0u);

    filter.genre = NULL;
    filter.text = "GRAN";
    assert(pcsx2_game_library_filter(&library, &filter, indices, 4u) == 1u);
    assert(indices[0] == 1u);

    assert(pcsx2_game_library_set_favorite(&library, "scus-97399", 1) == 0);
    filter.text = NULL;
    filter.favorites_only = 1;
    assert(pcsx2_game_library_filter(&library, &filter, indices, 4u) == 1u);
    assert(indices[0] == 0u);
    assert(pcsx2_game_library_set_favorite(&library, "missing", 1) == -2);

    /* Return total matches even when the caller's output buffer is smaller. */
    filter.favorites_only = 0;
    assert(pcsx2_game_library_filter(&library, &filter, indices, 1u) == 2u);
    assert(indices[0] == 0u);
    assert(pcsx2_game_library_filter(NULL, &filter, indices, 1u) == 0u);
    puts("OK: game library add, duplicate protection, favorites and filters.");
    return 0;
}
