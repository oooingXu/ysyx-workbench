#ifndef __PIPELINE_TRACKER_H__
#define __PIPELINE_TRACKER_H__

#include <stdint.h>
#include <stdbool.h>

// Maximum number of instructions in flight
#define MAX_INFLIGHT_INSNS 64

// Pipeline stage enum
typedef enum {
    STAGE_NONE = 0,
    STAGE_IF,   // Instruction Fetch
    STAGE_ID,   // Instruction Decode
    STAGE_EX,   // Execute
    STAGE_MEM,  // Memory Access
    STAGE_WB    // Write Back
} pipeline_stage_t;

// Instruction state in pipeline
typedef struct {
    bool valid;
    uint64_t insn_id;       // Konata instruction ID
    uint64_t global_id;     // Global instruction counter
    uint32_t pc;            // Program counter
    uint32_t inst;          // Instruction word
    pipeline_stage_t current_stage;
    bool retired;
    bool flushed;
} insn_state_t;

// Initialize pipeline tracker
void pipeline_tracker_init(void);

// Reset pipeline tracker
void pipeline_tracker_reset(void);

// Update pipeline state (called from DPI-C)
void pipeline_tracker_update(uint32_t pc, uint32_t dnpc, uint32_t inst, bool valid);

// Get instruction state by PC
insn_state_t* pipeline_tracker_get_by_pc(uint32_t pc);

// Retire instruction
void pipeline_tracker_retire(uint32_t pc);

// Flush instruction
void pipeline_tracker_flush(uint32_t pc);

#endif // __PIPELINE_TRACKER_H__
