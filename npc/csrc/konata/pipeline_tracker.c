#include "pipeline_tracker.h"
#include "konata_logger.h"
#include <stdio.h>
#include <string.h>

static insn_state_t inflight_insns[MAX_INFLIGHT_INSNS];
static uint64_t next_insn_id = 0;
static uint64_t next_retire_id = 0;
static uint64_t cycle_count = 0;
static uint64_t last_cycle = 0;

void pipeline_tracker_init(void) {
    memset(inflight_insns, 0, sizeof(inflight_insns));
    next_insn_id = 0;
    next_retire_id = 0;
    cycle_count = 0;
    last_cycle = 0;
}

void pipeline_tracker_reset(void) {
    pipeline_tracker_init();
}

// Find instruction by PC
static insn_state_t* find_insn_by_pc(uint32_t pc) {
    for (int i = 0; i < MAX_INFLIGHT_INSNS; i++) {
        if (inflight_insns[i].valid && inflight_insns[i].pc == pc) {
            return &inflight_insns[i];
        }
    }
    return NULL;
}

// Find free slot
static insn_state_t* find_free_slot(void) {
    for (int i = 0; i < MAX_INFLIGHT_INSNS; i++) {
        if (!inflight_insns[i].valid) {
            return &inflight_insns[i];
        }
    }
    return NULL;
}

// Disassemble instruction (simplified)
static void disasm_inst(uint32_t pc, uint32_t inst, char *buf, size_t size) {
    snprintf(buf, size, "0x%08x: 0x%08x", pc, inst);
}

void pipeline_tracker_update(uint32_t pc, uint32_t dnpc, uint32_t inst, bool valid) {
    if (!konata_is_enabled()) return;

    // Advance cycle
    cycle_count++;
    if (cycle_count > last_cycle) {
        konata_advance_cycle(cycle_count - last_cycle);
        last_cycle = cycle_count;
    }

    if (!valid) return;

    // Check if instruction already exists
    insn_state_t *insn = find_insn_by_pc(pc);

    if (insn == NULL) {
        // New instruction - allocate slot
        insn = find_free_slot();
        if (insn == NULL) {
            // Try to retire old instructions to free up slots
            for (int i = 0; i < MAX_INFLIGHT_INSNS; i++) {
                if (inflight_insns[i].valid &&
                    inflight_insns[i].current_stage == STAGE_WB &&
                    !inflight_insns[i].retired) {
                    // Auto-retire instructions that reached WB stage
                    konata_retire_insn(inflight_insns[i].insn_id, next_retire_id++, RETIRE_TYPE_NORMAL);
                    inflight_insns[i].retired = true;
                    inflight_insns[i].valid = false;
                }
            }

            // Try again to find a free slot
            insn = find_free_slot();
            if (insn == NULL) {
                // Still no slot, skip this instruction
                return;
            }
        }

        // Initialize new instruction
        insn->valid = true;
        insn->insn_id = next_insn_id++;
        insn->global_id = insn->insn_id;
        insn->pc = pc;
        insn->inst = inst;
        insn->current_stage = STAGE_IF;
        insn->retired = false;
        insn->flushed = false;

        // Log to Konata
        konata_init_insn(insn->insn_id, insn->global_id, 0);

        // Add label
        char label[128];
        disasm_inst(pc, inst, label, sizeof(label));
        konata_add_label(insn->insn_id, 0, label);

        // Start fetch stage
        konata_start_stage(insn->insn_id, LANE_NORMAL, STAGE_FETCH);
    } else {
        // Instruction progressing through pipeline
        // Determine next stage based on current stage
        pipeline_stage_t next_stage = (pipeline_stage_t)((int)insn->current_stage + 1);

        if (next_stage <= STAGE_WB && next_stage != insn->current_stage) {
            // Start next stage
            const char *stage_names[] = {"", "F", "D", "E", "M", "W"};
            if ((int)next_stage < (int)(sizeof(stage_names)/sizeof(stage_names[0]))) {
                konata_start_stage(insn->insn_id, LANE_NORMAL, stage_names[next_stage]);
                insn->current_stage = next_stage;

                // Auto-retire when reaching WB stage
                if (next_stage == STAGE_WB && !insn->retired) {
                    konata_retire_insn(insn->insn_id, next_retire_id++, RETIRE_TYPE_NORMAL);
                    insn->retired = true;
                    insn->valid = false;  // Free the slot
                }
            }
        }
    }
}

insn_state_t* pipeline_tracker_get_by_pc(uint32_t pc) {
    return find_insn_by_pc(pc);
}

void pipeline_tracker_retire(uint32_t pc) {
    if (!konata_is_enabled()) return;

    insn_state_t *insn = find_insn_by_pc(pc);
    if (insn == NULL) return;

    if (!insn->retired) {
        konata_retire_insn(insn->insn_id, next_retire_id++, RETIRE_TYPE_NORMAL);
        insn->retired = true;
        insn->valid = false;  // Free the slot
    }
}

void pipeline_tracker_flush(uint32_t pc) {
    if (!konata_is_enabled()) return;

    insn_state_t *insn = find_insn_by_pc(pc);
    if (insn == NULL) return;

    if (!insn->flushed && !insn->retired) {
        konata_retire_insn(insn->insn_id, next_retire_id++, RETIRE_TYPE_FLUSH);
        insn->flushed = true;
        insn->valid = false;  // Free the slot
    }
}
