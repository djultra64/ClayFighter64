/* Throwaway verification harness by djultra64; not an N64 game runtime. */
#include <stdio.h>
#include <string.h>
#include "recomp.h"
#include "funcs.h"

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    FILE *f = fopen(argv[1], "rb");
    if (!f) return 3;
    uint8_t *ram = calloc(1, 8 * 1024 * 1024);
    if (!ram || fseek(f, 0x1000, SEEK_SET)) return 4;
    /* Convert big endian ROM to the word-swapped RDRAM layout of recomp.h. */
    for (size_t offset = 0x400; offset < 0xF8180; offset += 4) {
        uint8_t word[4];
        if (fread(word, 1, 4, f) != 4) return 5;
        for (size_t byte = 0; byte < 4; byte++) ram[(offset + byte) ^ 3] = word[byte];
    }
    fclose(f);
    const char *expected[] = {
        "CHAR_BMF", "CHAR_BLOB", "CHAR_BONKER", "CHAR_BOOGER", "CHAR_KILN",
        "CHAR_EWJ", "CHAR_HIGHFIVE", "CHAR_HOBO", "CHAR_HOUNGAN", "CHAR_ICKY",
        "CHAR_KUNGPOW", "CHAR_LIBERTY", "CHAR_LOCKJAW", "CHAR_SUMO", "CHAR_TAFFY",
        "CHAR_THOPPY", "CHAR_ZAPPA", "<Illegal Character Index>",
        "<Illegal Character Index>"
    };
    for (unsigned test = 0; test < 19; test++) {
        unsigned id = test == 18 ? 255 : test;
        recomp_context ctx = {0};
        ctx.r4 = id;
        cf63_character_name(ram, &ctx);
        uint32_t address = (uint32_t)ctx.r2;
        if (address < 0x80000000 || address >= 0x80800000) return 6;
        char result[64] = {0};
        size_t offset = address - 0x80000000;
        for (size_t n = 0; n < sizeof(result) - 1; n++) {
            if (offset + n >= 8 * 1024 * 1024) return 7;
            result[n] = ram[(offset + n) ^ 3];
            if (!result[n]) break;
        }
        if (strcmp(expected[test], result)) {
            fprintf(stderr, "id %u: expected %s, got %s\n", id, expected[test], result);
            return 8;
        }
        printf("id %02u: %s PASS\n", id, result);
    }
    free(ram);
    puts("19/19 original MIPS -> native C character-name checks passed.");
    return 0;
}
