#ifndef __KONATA_LOGGER_H__
#define __KONATA_LOGGER_H__

#include <stdint.h>
#include <stdbool.h>

// Konata log format version
#define KONATA_VERSION "0004"

// Pipeline stage names
#define STAGE_FETCH   "F"
#define STAGE_DECODE  "D"
#define STAGE_EXECUTE "E"
#define STAGE_MEMORY  "M"
#define STAGE_WRITEBACK "W"

// Instruction retire types
#define RETIRE_TYPE_NORMAL 0
#define RETIRE_TYPE_FLUSH  1

// Lane IDs
#define LANE_NORMAL 0
#define LANE_STALL  1

// Initialize Konata logger
// filename: output log file path
// Returns: 0 on success, -1 on error
int konata_init(const char *filename);

// Close Konata logger
void konata_close(void);

// Check if Konata logging is enabled
bool konata_is_enabled(void);

// Set current cycle (absolute)
void konata_set_cycle(uint64_t cycle);

// Advance cycle (relative)
void konata_advance_cycle(uint64_t delta);

// Initialize a new instruction
// insn_id: unique ID in file (sequential)
// global_id: global instruction ID from simulator
// thread_id: thread ID (0 for single-threaded)
void konata_init_insn(uint64_t insn_id, uint64_t global_id, uint32_t thread_id);

// Add label to instruction
// insn_id: instruction ID
// label_type: 0=display label, 1=detail label, 2=stage label
// text: label text (e.g., "0x80000000 add x1,x2,x3")
void konata_add_label(uint64_t insn_id, int label_type, const char *text);

// Start a pipeline stage
// insn_id: instruction ID
// lane_id: lane ID (0=normal, 1=stall)
// stage_name: stage name (F/D/E/M/W)
void konata_start_stage(uint64_t insn_id, int lane_id, const char *stage_name);

// End a pipeline stage (optional)
// insn_id: instruction ID
// lane_id: lane ID
// stage_name: stage name
void konata_end_stage(uint64_t insn_id, int lane_id, const char *stage_name);

// Retire or flush instruction
// insn_id: instruction ID
// retire_id: retire ID (sequential)
// retire_type: 0=retire, 1=flush
void konata_retire_insn(uint64_t insn_id, uint64_t retire_id, int retire_type);

// Add dependency between instructions
// consumer_id: consumer instruction ID
// producer_id: producer instruction ID
// dep_type: dependency type (0=wake up)
void konata_add_dependency(uint64_t consumer_id, uint64_t producer_id, int dep_type);

#endif // __KONATA_LOGGER_H__
