#include "common.h"
#include "dfs.h"
#include "mrsg.h"
#include "scheduling.h"

void MRSG_init (void)
{
    user.task_cost_f = NULL;
    user.dfs_f = default_dfs_f;
    user.map_output_f = NULL;
    user.scheduler_f = default_scheduler_f;
}

void MRSG_set_task_cost_f ( double (*f)(enum phase_e phase, size_t tid, size_t wid) )
{
    user.task_cost_f = f;
}

void MRSG_set_dfs_f ( void (*f)(char** dfs_matrix, size_t chunks, size_t workers, int replicas) )
{
    user.dfs_f = f;
}

void MRSG_set_map_output_f ( int (*f)(size_t mid, size_t rid) )
{
    user.map_output_f = f;
}

void MRSG_set_scheduler_f ( size_t (*f)(enum phase_e phase, size_t wid) )
{
    user.scheduler_f = f;
}

// vim: set ts=8 sw=4:
