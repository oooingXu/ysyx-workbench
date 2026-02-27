/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <common.h>

extern uint64_t g_nr_guest_inst;

#ifndef CONFIG_TARGET_AM
FILE *log_fp = NULL;

void init_log(const char *log_file) {
  log_fp = stdout;
  if (log_file != NULL) {
    FILE *fp = fopen(log_file, "w");
    Assert(fp, "Can not open '%s'", log_file);
    log_fp = fp;
  }
  Log("Log is written to %s", log_file ? log_file : "stdout");
}

FILE *log_nzsxfp = NULL;

void init_nzsxlog(const char *log_nzsxfile) {
  log_nzsxfp = stdout;
  if (log_nzsxfile != NULL) {
    FILE *fp = fopen(log_nzsxfile, "w");
    Assert(fp, "Can not open '%s'", log_nzsxfile);
    log_nzsxfp = fp;
  }
  Log("Log is written to %s", log_nzsxfile ? log_nzsxfile : "stdout");
}

FILE *log_nzoxfp = NULL;

void init_nzoxlog(const char *log_nzoxfile) {
  log_nzoxfp = stdout;
  if (log_nzoxfile != NULL) {
    FILE *fp = fopen(log_nzoxfile, "w");
    Assert(fp, "Can not open '%s'", log_nzoxfile);
    log_nzoxfp = fp;
  }
  Log("Log is written to %s", log_nzoxfile ? log_nzoxfile : "stdout");
}

bool log_enable() {
  return MUXDEF(CONFIG_TRACE, (g_nr_guest_inst >= CONFIG_TRACE_START) &&
         (g_nr_guest_inst <= CONFIG_TRACE_END), false);
}
#endif
