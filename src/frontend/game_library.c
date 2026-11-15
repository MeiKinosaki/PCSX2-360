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

#define GAME_LIBRARY_MAGIC UINT32_C(0x4c324750)
#define GAME_LIBRARY_HEADER_SIZE 16u
#define GAME_LIBRARY_ENTRY_SIZE (PCSX2_GAME_TITLE_MAX + PCSX2_GAME_SERIAL_MAX + PCSX2_GAME_GENRE_MAX + PCSX2_GAME_PATH_MAX + PCSX2_GAME_PATH_MAX + 1u)

static void write_u32_le(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static uint32_t read_u32_le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint32_t library_checksum(const uint8_t *data, size_t size)
{
    uint32_t hash = UINT32_C(2166136261);
    size_t i;
    for (i = 0; i < size; ++i) {
        hash ^= data[i];
        hash *= UINT32_C(16777619);
    }
    return hash;
}

size_t pcsx2_game_library_snapshot_size(void)
{
    return GAME_LIBRARY_HEADER_SIZE +
           (size_t)GAME_LIBRARY_ENTRY_SIZE * PCSX2_GAME_LIBRARY_MAX;
}

int pcsx2_game_library_save(const pcsx2_game_library_t *library,
                            uint8_t *out, size_t capacity, size_t *written)
{
    size_t i, j, offset = GAME_LIBRARY_HEADER_SIZE;
    size_t payload_size, total_size;
    if (written != NULL) *written = 0u;
    if (library == NULL || out == NULL || written == NULL ||
        library->count > PCSX2_GAME_LIBRARY_MAX) return -1;

    /* Validate every record before modifying the caller's output buffer. */
    for (i = 0; i < library->count; ++i) {
        const pcsx2_game_entry_t *entry = &library->entries[i];
        if (!bounded_string_valid(entry->title, sizeof(entry->title)) ||
            !bounded_string_valid(entry->serial, sizeof(entry->serial)) ||
            !bounded_string_valid(entry->genre, sizeof(entry->genre)) ||
            !bounded_string_valid(entry->image_path, sizeof(entry->image_path)) ||
            !bounded_string_valid(entry->cover_path, sizeof(entry->cover_path)) ||
            entry->title[0] == '\0' || entry->serial[0] == '\0' ||
            entry->favorite > 1u) return -3;
        for (j = 0; j < i; ++j) {
            if (equal_case_insensitive(library->entries[j].serial,
                                       entry->serial)) return -4;
        }
    }

    payload_size = (size_t)GAME_LIBRARY_ENTRY_SIZE * library->count;
    total_size = GAME_LIBRARY_HEADER_SIZE + payload_size;
    if (capacity < total_size) return -2;
    write_u32_le(out, GAME_LIBRARY_MAGIC);
    write_u32_le(out + 4, PCSX2_GAME_LIBRARY_SNAPSHOT_VERSION);
    write_u32_le(out + 8, (uint32_t)library->count);
    for (i = 0; i < library->count; ++i) {
        const pcsx2_game_entry_t *entry = &library->entries[i];
        memcpy(out + offset, entry->title, sizeof(entry->title)); offset += sizeof(entry->title);
        memcpy(out + offset, entry->serial, sizeof(entry->serial)); offset += sizeof(entry->serial);
        memcpy(out + offset, entry->genre, sizeof(entry->genre)); offset += sizeof(entry->genre);
        memcpy(out + offset, entry->image_path, sizeof(entry->image_path)); offset += sizeof(entry->image_path);
        memcpy(out + offset, entry->cover_path, sizeof(entry->cover_path)); offset += sizeof(entry->cover_path);
        out[offset++] = entry->favorite;
    }
    write_u32_le(out + 12, library_checksum(out + GAME_LIBRARY_HEADER_SIZE, payload_size));
    *written = total_size;
    return 0;
}

int pcsx2_game_library_load(pcsx2_game_library_t *library,
                            const uint8_t *data, size_t size)
{
    uint32_t count;
    size_t expected, offset, i, j;
    if (library == NULL || data == NULL) return -1;
    if (size < GAME_LIBRARY_HEADER_SIZE || read_u32_le(data) != GAME_LIBRARY_MAGIC)
        return -2;
    if (read_u32_le(data + 4) != PCSX2_GAME_LIBRARY_SNAPSHOT_VERSION) return -3;
    count = read_u32_le(data + 8);
    if (count > PCSX2_GAME_LIBRARY_MAX) return -4;
    expected = GAME_LIBRARY_HEADER_SIZE + (size_t)count * GAME_LIBRARY_ENTRY_SIZE;
    if (size != expected) return -5;
    if (library_checksum(data + GAME_LIBRARY_HEADER_SIZE,
                         size - GAME_LIBRARY_HEADER_SIZE) != read_u32_le(data + 12))
        return -6;

    /* Validate all records before mutating the destination library. */
    offset = GAME_LIBRARY_HEADER_SIZE;
    for (i = 0; i < count; ++i) {
        const uint8_t *record = data + offset;
        const char *title = (const char *)record;
        const char *serial = title + PCSX2_GAME_TITLE_MAX;
        const char *genre = serial + PCSX2_GAME_SERIAL_MAX;
        const char *image = genre + PCSX2_GAME_GENRE_MAX;
        const char *cover = image + PCSX2_GAME_PATH_MAX;
        const uint8_t favorite = (uint8_t)cover[PCSX2_GAME_PATH_MAX];
        if (!bounded_string_valid(title, PCSX2_GAME_TITLE_MAX) ||
            !bounded_string_valid(serial, PCSX2_GAME_SERIAL_MAX) ||
            !bounded_string_valid(genre, PCSX2_GAME_GENRE_MAX) ||
            !bounded_string_valid(image, PCSX2_GAME_PATH_MAX) ||
            !bounded_string_valid(cover, PCSX2_GAME_PATH_MAX) ||
            title[0] == '\0' || serial[0] == '\0' || favorite > 1u)
            return -7;
        for (j = 0; j < i; ++j) {
            const uint8_t *previous = data + GAME_LIBRARY_HEADER_SIZE +
                                      j * GAME_LIBRARY_ENTRY_SIZE;
            const char *previous_serial =
                (const char *)previous + PCSX2_GAME_TITLE_MAX;
            if (equal_case_insensitive(previous_serial, serial)) return -8;
        }
        offset += GAME_LIBRARY_ENTRY_SIZE;
    }

    pcsx2_game_library_init(library);
    offset = GAME_LIBRARY_HEADER_SIZE;
    for (i = 0; i < count; ++i) {
        pcsx2_game_entry_t *entry = &library->entries[i];
        memcpy(entry->title, data + offset, sizeof(entry->title)); offset += sizeof(entry->title);
        memcpy(entry->serial, data + offset, sizeof(entry->serial)); offset += sizeof(entry->serial);
        memcpy(entry->genre, data + offset, sizeof(entry->genre)); offset += sizeof(entry->genre);
        memcpy(entry->image_path, data + offset, sizeof(entry->image_path)); offset += sizeof(entry->image_path);
        memcpy(entry->cover_path, data + offset, sizeof(entry->cover_path)); offset += sizeof(entry->cover_path);
        entry->favorite = data[offset++];
    }
    library->count = count;
    return 0;
}
