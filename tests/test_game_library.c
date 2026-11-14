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
    /* Reject malformed fixed-size strings without a terminator. */
    {
        pcsx2_game_entry_t malformed;
        memset(&malformed, 'X', sizeof(malformed));
        assert(pcsx2_game_library_add(&library, &malformed) == -1);
    }

    /* Enforce the fixed library capacity without writing out of bounds. */
    {
        pcsx2_game_library_t full;
        pcsx2_game_entry_t item;
        unsigned int n;
        pcsx2_game_library_init(&full);
        for (n = 0; n < PCSX2_GAME_LIBRARY_MAX; ++n) {
            char title[PCSX2_GAME_TITLE_MAX];
            char serial[PCSX2_GAME_SERIAL_MAX];
            (void)snprintf(title, sizeof(title), "Game %u", n);
            (void)snprintf(serial, sizeof(serial), "S%05u", n);
            item = game(title, serial, "Test", "");
            assert(pcsx2_game_library_add(&full, &item) == 0);
        }
        item = game("Overflow", "OVERFLOW", "Test", "");
        assert(full.count == PCSX2_GAME_LIBRARY_MAX);
        assert(pcsx2_game_library_add(&full, &item) == -3);
        assert(full.count == PCSX2_GAME_LIBRARY_MAX);
    }

    /* Portable snapshot round-trip, checksum rejection and atomic load. */
    {
        uint8_t snapshot[PCSX2_GAME_LIBRARY_SNAPSHOT_VERSION == 1u ?
                         (16u + PCSX2_GAME_LIBRARY_MAX *
                          (PCSX2_GAME_TITLE_MAX + PCSX2_GAME_SERIAL_MAX +
                           PCSX2_GAME_GENRE_MAX + 2u * PCSX2_GAME_PATH_MAX + 1u)) : 1u];
        size_t written = 0u;
        pcsx2_game_library_t restored;
        pcsx2_game_library_init(&restored);
        assert(pcsx2_game_library_save(&library, snapshot, sizeof(snapshot), &written) == 0);
        assert(written < sizeof(snapshot));
        assert(pcsx2_game_library_load(&restored, snapshot, written) == 0);
        assert(restored.count == library.count);
        assert(restored.entries[0].favorite == library.entries[0].favorite);
        assert(strcmp(restored.entries[1].title, library.entries[1].title) == 0);
        snapshot[written - 1u] ^= 0x01u;
        assert(pcsx2_game_library_load(&restored, snapshot, written) == -6);
        assert(restored.count == library.count);
        assert(pcsx2_game_library_save(&library, snapshot, 1u, &written) == -2);
    }

    puts("OK: game library add, duplicate protection, favorites, filters, bounds and persistence.");
    return 0;
}
