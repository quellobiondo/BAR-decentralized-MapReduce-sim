#ifndef MRSG_H
#define MRSG_H

#include <stdlib.h>

/** @brief  Possible execution phases. */
enum phase_e {
    MAP,
    REDUCE
};

void MRSG_init (void);

int MRSG_main (const char* plat, const char* depl, const char* conf);

void MRSG_set_task_cost_f ( double (*f)(enum phase_e phase, size_t tid, size_t wid) );

void MRSG_set_dfs_f ( void (*f)(char** dfs_matrix, size_t chunks, size_t workers, int replicas) );

void MRSG_set_map_output_f ( int (*f)(size_t mid, size_t rid) );

void MRSG_set_scheduler_f ( size_t (*f)(enum phase_e phase, size_t wid) );

#endif /* !MRSG_H */

// vim: set ts=8 sw=4:
