/* Copyright (c) 2012. MRSG Team. All rights reserved. */

/* This file is part of MRSG.

 MRSG is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 MRSG is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with MRSG.  If not, see <http://www.gnu.org/licenses/>. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "worker.h"
#include "dfs.h"
#include "DLT.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(msg_test);

static void print_config(void);
static void print_stats(void);
static int is_straggler(msg_host_t worker);
static int task_time_elapsed(msg_task_t task);
static void set_speculative_tasks(msg_host_t worker);
static void send_scheduler_task(enum phase_e phase, size_t wid);
static void update_stats(enum task_type_e task_type);
static void send_task(enum phase_e phase, size_t tid, size_t data_src,
		size_t wid);
char* task_type_string(enum task_type_e task_type);
static void finish_all_task_copies(task_info_t ti);
static int enough_result_confirmation(task_info_t ti);
void scheduleFunction(void);
void processTaskCompletion(task_info_t ti, msg_host_t worker);

static int counter_created_instances = 0;

/** @brief  Main master function. */
int master(int argc, char* argv[]) {
//	heartbeat_t heartbeat;
	msg_error_t status;
	msg_host_t worker;
	msg_task_t msg = NULL, original_msg = NULL;
	DLT_block_t block = NULL;
	task_info_t ti;
	size_t worker_index;
	msg_process_t DLT_process;
	int tx_counter;


	TRACE_host_state_declare("MAP");
	TRACE_host_state_declare("REDUCE");

	TRACE_host_state_declare_value("MAP", "START", "0.7 0.7 0.7");
	TRACE_host_state_declare_value("MAP", "END", "0.7 0.7 0.7");
	TRACE_host_state_declare_value("REDUCE", "START", "0.1 0.7 0.1");
	TRACE_host_state_declare_value("REDUCE", "END", "0.1 0.7 0.1");

	/* Spawn a process to exchange data with other workers. */
	DLT_process = MSG_process_create("DLT", DLT, NULL, MSG_host_self());

	print_config();
	XBT_INFO("JOB BEGIN");
	XBT_INFO(" ");

	counter_created_instances = 0;

	// schedule all the tasks
	while(counter_created_instances < (config.amount_of_tasks[MAP]+config.amount_of_tasks[REDUCE]) * number_of_task_replicas()){
		for(worker_index = 0; worker_index < config.number_of_workers; worker_index++){

			send_scheduler_task(MAP, worker_index);
			send_scheduler_task(REDUCE, worker_index);
		}
		XBT_INFO(".... %d / %d", counter_created_instances, (config.amount_of_tasks[MAP]+config.amount_of_tasks[REDUCE]) * number_of_task_replicas());
	}

	XBT_INFO("Finished!");

	// while we have at least a task pending (MAP/REDUCE)
	while (job.tasks_pending[MAP] + job.tasks_pending[REDUCE] > 0) {
		// schedule the tasks
		scheduleFunction();

		if (MSG_task_listen(MASTER_MAILBOX)) {
			//a new block is here
			msg = NULL;
			status = receive(&msg, MASTER_MAILBOX);

			if (status == MSG_OK) {
				xbt_assert(message_is(msg, SMS_DLT_BLOCK), "Master received a message that is not from the DLT!");
				block = (DLT_block_t) MSG_task_get_data(msg);

				for(tx_counter = 0; tx_counter < block->size; tx_counter++){
					worker = block->original_senders[tx_counter];

					original_msg = block->original_messages[tx_counter];

					if (message_is(original_msg, SMS_TASK_DONE)) {
						ti = (task_info_t) MSG_task_get_data(original_msg);
						processTaskCompletion(ti, worker);
					}
					MSG_task_destroy(original_msg);
				}
				MSG_task_destroy(msg);
			}
		}

		MSG_process_sleep(1);
	}

	job.finished = 1;

	print_config();
	print_stats();

	MSG_process_kill(DLT_process);

	XBT_INFO("JOB END");

	xbt_free_f(capacity[MAP]);
	xbt_free_f(capacity[REDUCE]);

	return 0;
}

void processTaskCompletion(task_info_t ti, msg_host_t worker){
	// increment result confirmations
	job.task_confirmations[ti->phase][ti->id]++;

	if (enough_result_confirmation(ti)){
		// mark that task as done and finish everything
		// task finished, clean up and communicate
		if (job.task_status[ti->phase][ti->id] != T_STATUS_DONE) {
			job.task_status[ti->phase][ti->id] = T_STATUS_DONE;
			XBT_VERB("Completed %lu", ti->id);

			// finish_all_task_copies(ti); // pre-emption
			job.tasks_pending[ti->phase]--;

			if (job.tasks_pending[ti->phase] <= 0) {
				XBT_INFO(" ");
				XBT_INFO("%s PHASE DONE",
						(ti->phase == MAP ? "MAP" : "REDUCE"));
				XBT_INFO(" ");
				TRACE_host_set_state(MSG_host_get_name(MSG_host_self()), (ti->phase == MAP ? "MAP" : "REDUCE"), "END");
			}
		}
		xbt_free_ref(&ti);
	}
}

void scheduleFunction(void){
	msg_host_t worker;
	size_t worker_index;
	size_t index;

	for(worker_index = 0; worker_index < config.number_of_workers; worker_index++){
		worker = config.workers[worker_index];

		if (is_straggler(worker)) {
			set_speculative_tasks(worker);
		}

		//TODO to check that in this way we don't assign to the stragglers too many tasks
		// check if this worker can do more work

		send_scheduler_task(MAP, worker_index);
		send_scheduler_task(REDUCE, worker_index);

	}
}

/** @brief  Print the job configuration. */
static void print_config(void) {
	XBT_INFO("JOB CONFIGURATION:");
	XBT_INFO("slots: %d map, %d reduce", config.slots[MAP],
			config.slots[REDUCE]);
	XBT_INFO("chunk replicas: %d", config.chunk_replicas);
	XBT_INFO("chunk size: %.0f MB", config.chunk_size / 1024 / 1024);
	XBT_INFO("input chunks: %d", config.chunk_count);
	XBT_INFO("input size: %d MB",
			config.chunk_count * (int )(config.chunk_size / 1024 / 1024));
	XBT_INFO("maps: %d", config.amount_of_tasks[MAP]);
	XBT_INFO("reduces: %d", config.amount_of_tasks[REDUCE]);
	XBT_INFO("workers: %d", config.number_of_workers);
	XBT_INFO("grid power: %g flops", config.grid_cpu_power);
	XBT_INFO("average power: %g flops/s", config.grid_average_speed);
	XBT_INFO("heartbeat interval: %ds", config.heartbeat_interval);
	XBT_INFO("byzantine: %d", config.byzantine);
	XBT_INFO("Distributed Ledger average block time: %d", config.block_period);
	XBT_INFO("Distributed Ledger transactions per block: %d", config.block_size);
	XBT_INFO(" ");
}

/** @brief  Print job statistics. */
static void print_stats(void) {
	XBT_INFO("JOB STATISTICS:");
	XBT_INFO("local maps: %d", stats.map_local);
	XBT_INFO("non-local maps: %d", stats.map_remote);
	XBT_INFO("speculative maps (local): %d", stats.map_spec_l);
	XBT_INFO("speculative maps (remote): %d", stats.map_spec_r);
	XBT_INFO("total non-local maps: %d", stats.map_remote + stats.map_spec_r);
	XBT_INFO("total speculative maps: %d", stats.map_spec_l + stats.map_spec_r);
	XBT_INFO("normal reduces: %d", stats.reduce_normal);
	XBT_INFO("speculative reduces: %d", stats.reduce_spec);
	XBT_INFO("reduces with local map-output fetch: %d", stats.reduce_local_map_result);
	XBT_INFO("reduces with remote map-output fetch: %d", stats.reduce_remote_map_result);
	XBT_INFO(" ");
}

/**
 * @brief  Checks if a worker is a straggler.
 * @param  worker  The worker to be probed
 * @return 1 if true, 0 if false.
 */
static int is_straggler(msg_host_t worker) {
	int task_count;
	size_t wid;

	wid = get_worker_id(worker);

	/*
	 * Number of tasks currently executed (number of slots per node - slots available = slot occupied = task currently executed)
	 */

	task_count = (config.slots[MAP] + config.slots[REDUCE])
				- (capacity[MAP][wid] + capacity[REDUCE][wid]);

	if (MSG_get_host_speed(worker) < config.grid_average_speed
			&& task_count > 0)
		return TRUE;

	return FALSE;
}

/*
 *
 * BYZANTINE PARAMETERS SECTION
 *
 */

/**
 * @brief Tell if we have enough replicas of the ti result to be sure that we are BFT
 *
 * Other strategies are possible, like replication to be fault-tolerant
 */
static int enough_result_confirmation(task_info_t ti){
	int threshold_BFT = config.byzantine + 1;
	return min(config.number_of_workers, threshold_BFT) <= job.task_confirmations[ti->phase][ti->id];
}

/**
 * @brief  Returns for how long a task is running.
 * @param  task  The task to be probed.
 * @return The amount of seconds since the beginning of the computation.
 */
static int task_time_elapsed(msg_task_t task) {
	task_info_t ti;

	ti = (task_info_t) MSG_task_get_data(task);

	return (MSG_task_get_compute_duration(task)
			- MSG_task_get_remaining_computation(task))
			/ MSG_get_host_speed(config.workers[ti->wid]);
}

/**
 * @brief  Mark the tasks of a straggler as possible speculative tasks.
 * @param  worker  The straggler worker.
 */
static void set_speculative_tasks(msg_host_t worker) {
	size_t tid;
	size_t wid;
	int phases[2]; phases[0] = MAP; phases[1] = REDUCE;
	int phase_index, phase;
	int timeout_phases[2]; timeout_phases[0] = TRIGGER_TIMEOUT_SPECULATIVE_MAP; timeout_phases[1] = TRIGGER_TIMEOUT_SPECULATIVE_REDUCE;

	wid = get_worker_id(worker);

	// mark all the tasks of the straggler node as available for speculative tasks in case they have timeouted.
	for(phase_index = 0; phase_index < 2; phase_index++){
		phase = phases[phase_index];

		//if (job.heartbeats[wid].slots_av[phase] < config.slots[phase]) {
		if(capacity[phase][wid] < config.slots[phase]){
			for (tid = 0; tid < config.amount_of_tasks[phase]; tid++) {
				if (job.task_list[phase][tid][wid] != NULL && job.task_status[phase][tid] == T_STATUS_TIP) {
					//the task has to be already assigned to the straggler and it has to be in the running phase
					if (task_time_elapsed(job.task_list[phase][tid][wid]) > timeout_phases[phase]) {
						job.task_status[phase][tid] = T_STATUS_TIP_SLOW;
					}
				}
			}
		}
	}
}

static void send_scheduler_task(enum phase_e phase, size_t wid) {
	// it's the user scheduler that decides which task to execute
	size_t tid = user.scheduler_f(phase, wid);

	if (tid == NONE) {
		return;
	}

	enum task_type_e task_type = get_task_type(phase, tid, wid);
	size_t sid = NONE; // source of the chunk

	if (task_type == LOCAL || task_type == LOCAL_SPEC) {
		sid = wid;
	} else if (task_type == REMOTE || task_type == REMOTE_SPEC) {
		sid = find_random_chunk_owner(tid);
	}

	XBT_INFO("%s %zu assigned to %s %s", (phase == MAP ? "map" : "reduce"), tid,
			MSG_host_get_name(config.workers[wid]),
			task_type_string(task_type));

	send_task(phase, tid, sid, wid);

	counter_created_instances++;

	update_stats(task_type);
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

/**
 * @brief  Send a task to a worker.
 * @param  phase     The current job phase.
 * @param  tid       The task ID.
 * @param  data_src  The ID of the DataNode that owns the task data.
 * @param  wid       The destination worker id.
 */
static void send_task(enum phase_e phase, size_t tid, size_t data_src,
		size_t wid) {
	char mailbox[MAILBOX_ALIAS_SIZE];
	double cpu_required = 0.0;
	msg_task_t task = NULL;
	task_info_t task_info;
	msg_error_t status;

	// for fault-tolerance we don't want to reassign a task to the same node
	xbt_assert(job.task_list[phase][tid][wid] == NULL);

	cpu_required = user.task_cost_f(phase, tid, wid);

	task_info = xbt_new(struct task_info_s, 1);
	task = MSG_task_create(SMS_TASK, cpu_required, 0.0, (void*) task_info);

	task_info->phase = phase;
	task_info->id = tid;
	task_info->src = data_src;
	task_info->wid = wid;
	task_info->task = task;
	task_info->shuffle_end = 0.0;

	// for tracing purposes...
	MSG_task_set_category(task, (phase == MAP ? "MAP" : "REDUCE"));

	job.task_list[phase][tid][wid] = task;

	if (job.task_status[phase][tid] == T_STATUS_TIP_SLOW){
		job.task_replicas_instances[phase][tid]++;
	}else{
		job.task_instances[phase][tid]++;
		if(job.task_instances[phase][tid] >= number_of_task_replicas())
			job.task_status[phase][tid] = T_STATUS_TIP;
	}

	//job.heartbeats[wid].slots_av[phase]--; //okay, it just tell that there is a slot that is less available

	TRACE_host_set_state(MSG_host_get_name(MSG_host_self()), (phase == MAP ? "MAP" : "REDUCE"), "START");

	XBT_VERB("TX: %s > %s", SMS_TASK, MSG_host_get_name (config.workers[wid]));

	switch(phase){
	case MAP: sprintf(mailbox, MAP_TASKTRACKER_MAILBOX, wid); break;
	case REDUCE: sprintf(mailbox, REDUCE_TASKTRACKER_MAILBOX, wid); break;
	}

	MSG_task_send(task, mailbox);
}

static void update_stats(enum task_type_e task_type) {
	switch (task_type) {
	case LOCAL:
		stats.map_local++;
		break;

	case REMOTE:
		stats.map_remote++;
		break;

	case LOCAL_SPEC:
		stats.map_spec_l++;
		break;

	case REMOTE_SPEC:
		stats.map_spec_r++;
		break;

	case NORMAL:
		stats.reduce_normal++;
		break;

	case SPECULATIVE:
		stats.reduce_spec++;
		break;

	default:
		return;
	}
}

char* task_type_string(enum task_type_e task_type) {
	switch (task_type) {
	case REMOTE:
		return "(non-local)";

	case LOCAL_SPEC:
	case SPECULATIVE:
		return "(speculative)";

	case REMOTE_SPEC:
		return "(non-local, speculative)";

	default:
		return "";
	}
}

/**
 * @brief  Kill all copies of a task.
 * @param  ti  The task information of any task instance.
 */
static void finish_all_task_copies(task_info_t ti) {
	int i;
	int phase = ti->phase;
	size_t tid = ti->id;

	for (i = 0; i < config.number_of_workers; i++) {
		if (job.task_list[phase][tid][i] != NULL) {
			MSG_task_cancel(job.task_list[phase][tid][i]);
		}
	}
}

// vim: set ts=8 sw=4:
