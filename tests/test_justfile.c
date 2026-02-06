/* Justfile Integration Tests */

#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char* read_file_text(const char* path) {
    struct stat st = {0};
    if (stat(path, &st) != 0) return NULL;

    FILE* file = fopen(path, "rb");
    if (!file) return NULL;

    char* text = (char*)malloc((size_t)st.st_size + 1);
    if (!text) {
        fclose(file);
        return NULL;
    }

    size_t read_n = fread(text, 1, (size_t)st.st_size, file);
    fclose(file);
    text[read_n] = '\0';
    return text;
}

void test_justfile_exists(void) {
    ASSERT_EQ(access("Justfile", F_OK), 0);
}

void test_justfile_exposes_core_recipes(void) {
    char* justfile = read_file_text("Justfile");
    ASSERT_NOT_NULL(justfile);

    ASSERT_TRUE(strstr(justfile, "debug:") != NULL);
    ASSERT_TRUE(strstr(justfile, "release:") != NULL);
    ASSERT_TRUE(strstr(justfile, "test:") != NULL);
    ASSERT_TRUE(strstr(justfile, "check:") != NULL);
    ASSERT_TRUE(strstr(justfile, "release-package:") != NULL);
    ASSERT_TRUE(strstr(justfile, "benchmark-report:") != NULL);

    ASSERT_TRUE(strstr(justfile, "make debug") != NULL);
    ASSERT_TRUE(strstr(justfile, "make check") != NULL);

    free(justfile);
}

void run_justfile_tests(void) {
    printf("\n=== Justfile Tests ===\n");
    TEST_RUN(test_justfile_exists);
    TEST_RUN(test_justfile_exposes_core_recipes);
}
