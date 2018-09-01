#include "scheduling.h" // get_task_type
#include "common.h"
#include "dfs.h"
#include "master.h"
#include "worker.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(msg_test); // @suppress("Unused variable declaration in file scope")

/**
 * @brief  Chooses a map or reduce task and send it to a worker.
 * @param  phase  MAP or REDUCE.
 * @param  wid  Worker id.
 * @return Chosen task id.
 */
size_t default_scheduler_f(enum phase_e phase, size_t wid) {
	switch (phase) {
	case MAP:
		return choose_default_map_task(wid);

	case REDUCE:
		return choose_default_reduce_task(wid);

	default:
		return NONE;
	}
}

/**
 * @brief  Choose a map task, and send it to a worker.
 * @param  wid  Worker id.
 * @return tid  Assigned Task Id
 *
 * TODO PUSH as much stuff as you can on the platform, and leave just the scheduling algo
 */
size_t choose_default_map_task(size_t wid /*, TODO list of tasks that need to be executed*/) {
	size_t map_id;
	size_t selected_task_id = NONE;

	/*
	 * If there are no pending MAP tasks return none.
	 */
	if (job.tasks_pending[MAP] <= 0)
		return NONE;

	/* Look for a task for the worker.
	 *
	 * Return the first chunk that the node hasn't done yet
	 * or the second speculative task if it is remote, or speculative (with local speculative preferred to remote spec.)
	 * */
	for (map_id = 0; map_id < config.chunk_count; map_id++) {
		//check if the task have already been executed by this node
		if(job.task_list[MAP][map_id][wid] != NULL) continue;
		selected_task_id = map_id;
		break;
	}

	return selected_task_id;
}

/**
 * @brief  Choose a reduce task, and send it to a worker.
 * @param  wid  Worker id.
 * @return tid  Selected Task id.
 */
size_t choose_default_reduce_task(size_t wid) {
	size_t reduce_id;
	size_t selected_task_id = NONE;

	/*
	 * If there are no pending REDUCE tasks or
	 * there are more than 90% of tasks still to be executed
	 * Don't assign any reduce task
	 */
	if (job.tasks_pending[REDUCE] <= 0
			|| ((float) job.tasks_pending[MAP]) / config.amount_of_tasks[MAP] > 0.9)
		return NONE;

	/*
	 * return the first task that this node hasn't done yet
	 */
	for (reduce_id = 0; reduce_id < config.amount_of_tasks[REDUCE]; reduce_id++) {
		//check if the task have already been executed by this node
		if(job.task_list[REDUCE][reduce_id][wid] != NULL) continue;

		selected_task_id = reduce_id;
		break;
	}

	return selected_task_id;
}


size_t send_scheduled_task(enum phase_e phase, size_t wid) {
	// it's the user scheduler that decides which task to execute
	size_t tid = user.scheduler_f(phase, wid);

	if (tid == NONE) {
		return tid;
	}

	enum task_type_e task_type = get_task_type(phase, tid, wid);
	size_t sid = NONE; // source of the chunk

	if (task_type == LOCAL) {
		sid = wid;
	} else if (task_type == REMOTE) {
		sid = find_random_chunk_owner(tid);
	}

	XBT_INFO("%s %zu assigned to %s %s", (phase == MAP ? "map" : "reduce"), tid,
			MSG_host_get_name(config.workers[wid]),
			task_type_string(task_type));

	execute_task(phase, tid, sid);

	update_stats(task_type);

	return tid;
}

/*
 * @brief Tell to the scheduler which type is the task that it is processing
 *		  The type is different for MAP and REDUCE tasks
 *		  MAP -> LOCAL, REMOTE, LOCAL_SPEC, REMOTE_SPEC
 *		  REDUCE -> NORMAL, SPECULATIVE
 *		  To suggest the type of execution, it uses the "tip" from the job status.
 *		  - PENDING -> It means no issues
 *		  - SLOW -> Requires a speculative task
 *		  - COMPLETED -> No job to do here
 */
enum task_type_e get_task_type(enum phase_e phase, size_t tid, size_t wid) {
	enum task_status_e task_status = job.task_status[phase][tid];

	switch (phase) {
	case MAP:
		switch (task_status) {
		case T_STATUS_PENDING:
			return chunk_owner[tid][wid] ? LOCAL : REMOTE;

		case T_STATUS_TIP_SLOW:
			return chunk_owner[tid][wid] ? LOCAL_SPEC : REMOTE_SPEC;

		case T_STATUS_TIP:
			return NO_TASK;

		case T_STATUS_DONE:
			return NO_TASK;

		default:
			xbt_die("Non treated task status: %d", task_status);
		}
		break;
	case REDUCE:
		switch (task_status) {
		case T_STATUS_PENDING:
			return NORMAL;

		case T_STATUS_TIP_SLOW:
			return SPECULATIVE;

		case T_STATUS_TIP:
			return NO_TASK;

		case T_STATUS_DONE:
			return NO_TASK;

		default:
			xbt_die("Non treated task status: %d", task_status);
		}
		break;
	}
	xbt_die("Non treated phase: %d", phase);
}

// vim: set ts=8 sw=4:
