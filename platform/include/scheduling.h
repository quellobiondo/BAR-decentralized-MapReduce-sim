

#ifndef SCHEDULING_H
#define SCHEDULING_H

#include "common.h"

/**
 * @brief  Chooses a map or reduce task and send it to a worker.
 * @param  phase  MAP or REDUCE.
 * @param  wid  Worker id.
 * @return Chosen task id.
 */
size_t default_scheduler_f (enum phase_e phase, size_t wid);
size_t choose_default_map_task (size_t wid);
size_t choose_default_reduce_task (size_t wid);

#endif /* !SCHEDULING_H */

// vim: set ts=8 sw=4:

