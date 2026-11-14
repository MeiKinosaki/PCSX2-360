#include "game_library.h"

#include <ctype.h>
#include <string.h>

static int bounded_string_valid(const char *value, size_t capacity)
{
    return value != NULL && memchr(value, '\0', capacity) != NULL;
}

static int equal_case_insensitive(const char *left, const char *right)
{
    if (left == NULL || right == NULL) return 0;
    while (*left != '\0' && *right != '\0') {
        if (tolower((unsigned char)*left) != tolower((unsigned char)*right))
            return 0;
        ++left;
        ++right;
    }
    return *left == '\0' && *right == '\0';
}

static int contains_case_insensitive(const char *text, const char *needle)
{
    size_t i, j;
    if (needle == NULL || needle[0] == '\0') return 1;
    if (text == NULL) return 0;
    for (i = 0; text[i] != '\0'; ++i) {
        for (j = 0; needle[j] != '\0' && text[i + j] != '\0'; ++j) {
            if (tolower((unsigned char)text[i + j]) !=
                tolower((unsigned char)needle[j])) break;
        }
        if (needle[j] == '\0') return 1;
    }
    return 0;
}

void pcsx2_game_library_init(pcsx2_game_library_t *library)
{
    if (library != NULL) memset(library, 0, sizeof(*library));
}

int pcsx2_game_library_add(pcsx2_game_library_t *library,
                           const pcsx2_game_entry_t *entry)
{
    size_t i;
    if (library == NULL || entry == NULL ||
        library->count > PCSX2_GAME_LIBRARY_MAX ||
        !bounded_string_valid(entry->title, sizeof(entry->title)) ||
        !bounded_string_valid(entry->serial, sizeof(entry->serial)) ||
        !bounded_string_valid(entry->genre, sizeof(entry->genre)) ||
        !bounded_string_valid(entry->image_path, sizeof(entry->image_path)) ||
        !bounded_string_valid(entry->cover_path, sizeof(entry->cover_path)) ||
        entry->title[0] == '\0' || entry->serial[0] == '\0') return -1;
    for (i = 0; i < library->count; ++i) {
        if (equal_case_insensitive(library->entries[i].serial, entry->serial))
            return -2;
    }
    if (library->count == PCSX2_GAME_LIBRARY_MAX) return -3;
    library->entries[library->count] = *entry;
    library->entries[library->count].favorite =
        entry->favorite != 0u ? 1u : 0u;
    ++library->count;
    return 0;
}

int pcsx2_game_library_set_favorite(pcsx2_game_library_t *library,
                                   const char *serial, int favorite)
{
    size_t i;
    if (library == NULL || serial == NULL || serial[0] == '\0') return -1;
    for (i = 0; i < library->count; ++i) {
        if (equal_case_insensitive(library->entries[i].serial, serial)) {
            library->entries[i].favorite = favorite ? 1u : 0u;
            return 0;
        }
    }
    return -2;
}

size_t pcsx2_game_library_filter(const pcsx2_game_library_t *library,
                                 const pcsx2_game_filter_t *filter,
                                 size_t *indices, size_t capacity)
{
    size_t i, matched = 0u;
    if (library == NULL || filter == NULL ||
        (indices == NULL && capacity != 0u)) return 0u;
    for (i = 0; i < library->count; ++i) {
        const pcsx2_game_entry_t *entry = &library->entries[i];
        if (filter->favorites_only && !entry->favorite) continue;
        if (filter->genre != NULL && filter->genre[0] != '\0' &&
            !equal_case_insensitive(entry->genre, filter->genre)) continue;
        if (!contains_case_insensitive(entry->title, filter->text) &&
            !contains_case_insensitive(entry->serial, filter->text)) continue;
        if (matched < capacity) indices[matched] = i;
        ++matched;
    }
    return matched;
}
