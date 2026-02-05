#include "konata_logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static FILE *konata_fp = NULL;
static uint64_t current_cycle = 0;
static bool initialized = false;

int konata_init(const char *filename) {
    if (initialized) {
        fprintf(stderr, "Warning: Konata logger already initialized\n");
        return 0;
    }

    if (filename == NULL) {
        return -1;
    }

    konata_fp = fopen(filename, "w");
    if (konata_fp == NULL) {
        fprintf(stderr, "Error: Failed to open Konata log file: %s\n", filename);
        return -1;
    }

    // Write header
    fprintf(konata_fp, "Kanata\t%s\n", KONATA_VERSION);
    fflush(konata_fp);

    initialized = true;
    current_cycle = 0;

    printf("Konata logger initialized: %s\n", filename);
    return 0;
}

void konata_close(void) {
    if (konata_fp != NULL) {
        fflush(konata_fp);
        fclose(konata_fp);
        konata_fp = NULL;
    }
    initialized = false;
    printf("Konata logger closed\n");
}

bool konata_is_enabled(void) {
    return initialized && (konata_fp != NULL);
}

void konata_set_cycle(uint64_t cycle) {
    if (!konata_is_enabled()) return;

    fprintf(konata_fp, "C=\t%lu\n", cycle);
    current_cycle = cycle;
    fflush(konata_fp);
}

void konata_advance_cycle(uint64_t delta) {
    if (!konata_is_enabled()) return;
    if (delta == 0) return;

    fprintf(konata_fp, "C\t%lu\n", delta);
    current_cycle += delta;
    fflush(konata_fp);
}

void konata_init_insn(uint64_t insn_id, uint64_t global_id, uint32_t thread_id) {
    if (!konata_is_enabled()) return;

    fprintf(konata_fp, "I\t%lu\t%lu\t%u\n", insn_id, global_id, thread_id);
    fflush(konata_fp);
}

void konata_add_label(uint64_t insn_id, int label_type, const char *text) {
    if (!konata_is_enabled()) return;
    if (text == NULL) return;

    fprintf(konata_fp, "L\t%lu\t%d\t%s\n", insn_id, label_type, text);
    fflush(konata_fp);
}

void konata_start_stage(uint64_t insn_id, int lane_id, const char *stage_name) {
    if (!konata_is_enabled()) return;
    if (stage_name == NULL) return;

    fprintf(konata_fp, "S\t%lu\t%d\t%s\n", insn_id, lane_id, stage_name);
    fflush(konata_fp);
}

void konata_end_stage(uint64_t insn_id, int lane_id, const char *stage_name) {
    if (!konata_is_enabled()) return;
    if (stage_name == NULL) return;

    fprintf(konata_fp, "E\t%lu\t%d\t%s\n", insn_id, lane_id, stage_name);
    fflush(konata_fp);
}

void konata_retire_insn(uint64_t insn_id, uint64_t retire_id, int retire_type) {
    if (!konata_is_enabled()) return;

    fprintf(konata_fp, "R\t%lu\t%lu\t%d\n", insn_id, retire_id, retire_type);
    fflush(konata_fp);
}

void konata_add_dependency(uint64_t consumer_id, uint64_t producer_id, int dep_type) {
    if (!konata_is_enabled()) return;

    fprintf(konata_fp, "W\t%lu\t%lu\t%d\n", consumer_id, producer_id, dep_type);
    fflush(konata_fp);
}
