#include "scheduling.h" // get_task_type
#include "common.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(msg_test); // @suppress("Unused variable declaration in file scope")

int number_max_speculative_copies (enum phase_e phase, size_t tid) {
	// as to have max 2f+1 replicas
	int max_number_of_replicas = config.byzantine + 1; // Standard

	// if we have received byzantine replies they increase the number of tasks we need
	max_number_of_replicas += job.task_byzantine_confirmations[phase][tid];

	if(job.task_status[phase][tid] == T_STATUS_TIP_SLOW){
		// if the task is slow we admit stragglers
		max_number_of_replicas += MAX_SPECULATIVE_COPIES;
	}
	return max_number_of_replicas;
}

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
	enum task_type_e task_type, best_task_type = NO_TASK;

	/*
	 * If there are no pending MAP tasks return none.
	 */
	if (job.tasks_pending[MAP] <= 0)
		return NONE;

	/* Look for a task for the worker.
	 *
	 * Return the first local chunk in order of chunk id
	 * or the second speculative task if it is remote, or speculative (with local speculative preferred to remote spec.)
	 * */
	for (map_id = 0; map_id < config.chunk_count; map_id++) {
		//check if the task have already been executed by this node
		if(job.task_list[MAP][map_id][wid] != NULL) continue;

		task_type = get_task_type(MAP, map_id, wid);

		int overall_number_of_instances = job.task_instances[MAP][map_id] + job.task_replicas_instances[MAP][map_id];
		if(overall_number_of_instances >= number_max_speculative_copies(MAP, map_id)) continue;

		if (task_type == LOCAL) {
			selected_task_id = map_id;
			break;
		} else if (task_type == REMOTE || task_type < best_task_type ){
			best_task_type = task_type;
			selected_task_id = map_id;
		}
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
	enum task_type_e task_type, best_task_type = NO_TASK;

	/*
	 * If there are no pending REDUCE tasks or
	 * there are more than 90% of tasks still to be executed
	 * Don't assign any reduce task
	 */
	if (job.tasks_pending[REDUCE] <= 0
			|| ((float) job.tasks_pending[MAP]) / config.amount_of_tasks[MAP] > 0.9)
		return NONE;

	/*
	 * Assign the job without notion of data locality, no optimization.
	 * If no matches, then it assign a speculative task
	 */
	for (reduce_id = 0; reduce_id < config.amount_of_tasks[REDUCE]; reduce_id++) {
		//check if the task have already been executed by this node
		if(job.task_list[REDUCE][reduce_id][wid] != NULL) continue;

		task_type = get_task_type(REDUCE, reduce_id, wid);

		int overall_number_of_instances = job.task_instances[REDUCE][reduce_id] + job.task_replicas_instances[REDUCE][reduce_id];
		if(overall_number_of_instances >= number_max_speculative_copies(REDUCE, reduce_id)) continue;

		if (task_type == NORMAL) {
			selected_task_id = reduce_id;
			break;
		} else if (task_type < best_task_type) {
			// TODO : MAX_SPECULATIVE_COPIES  = f + tolerance of failures
			best_task_type = task_type; // SPECULATIVE
			selected_task_id = reduce_id;
		}
	}

	return selected_task_id;
}

// vim: set ts=8 sw=4:
