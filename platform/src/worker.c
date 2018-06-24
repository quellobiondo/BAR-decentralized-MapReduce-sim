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

#include "common.h"
#include "dfs.h"
#include "worker.h"
#include "master.h"
#include "scheduling.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(msg_test); // @suppress("Unused variable declaration in file scope")

static void heartbeat(void);
static int listen(int argc, char* argv[]);
int compute(int argc, char* argv[]);
static void get_chunk(task_info_t ti);
static void get_map_output(task_info_t ti);
static void scheduleRoutine(size_t localhost_id);

size_t get_worker_id(msg_host_t worker) {
	w_info_t wi;

	wi = (w_info_t) MSG_host_get_data(worker);
	return wi->wid;
}

/**
 * @brief  Main worker function.
 *
 * This is the initial function of a worker node.
 * It creates other processes and runs a heartbeat loop.
 */
int worker(int argc, char* argv[]) {
	char mailbox[MAILBOX_ALIAS_SIZE];
	msg_host_t me;

	me = MSG_host_self();

	TRACE_host_state_declare("MAP-INPUT-FETCHING");
	TRACE_host_state_declare("REDUCE-INPUT-FETCHING");
	TRACE_host_state_declare("MAP-EXECUTION");
	TRACE_host_state_declare("REDUCE-EXECUTION");

	TRACE_host_state_declare_value("MAP-INPUT-FETCHING", "BEGIN", "0.7 0.7 0.7");
	TRACE_host_state_declare_value("MAP-INPUT-FETCHING", "END", "0.7 0.7 0.7");
	TRACE_host_state_declare_value("REDUCE-INPUT-FETCHING", "BEGIN", "0.1 0.7 0.1");
	TRACE_host_state_declare_value("REDUCE-INPUT-FETCHING", "END", "0.1 0.7 0.1");
	TRACE_host_state_declare_value("MAP-EXECUTION", "BEGIN", "0.1 0.7 0.1");
	TRACE_host_state_declare_value("MAP-EXECUTION", "END", "0.1 0.7 0.1");
	TRACE_host_state_declare_value("REDUCE-EXECUTION", "BEGIN", "0.1 0.7 0.1");
	TRACE_host_state_declare_value("REDUCE-EXECUTION", "END", "0.1 0.7 0.1");

	/* Spawn a process that listens for tasks. */
	MSG_process_create("listen", listen, NULL, me);
	/* Spawn a process to exchange data with other workers. */
	MSG_process_create("data-node", data_node, NULL, me);
	/* Start sending heartbeat signals to the master node. */
	heartbeat();

	sprintf(mailbox, DATANODE_MAILBOX, get_worker_id(me));
	send_sms(SMS_FINISH, mailbox);
	sprintf(mailbox, TASKTRACKER_MAILBOX, get_worker_id(me));
	send_sms(SMS_FINISH, mailbox);

	return 0;
}

static void scheduleRoutine(size_t localhost_id) {
	size_t index;
	size_t assigned = 0;

	assigned = 0;
	for(index = 0; index < capacity[MAP][localhost_id]; index++){
		if(send_scheduled_task(MAP, localhost_id)!= NONE){
			assigned ++;
		}
	}
	capacity[MAP][localhost_id] -= assigned;

	assigned = 0;
	for(index = 0; index < capacity[REDUCE][localhost_id]; index++){
		if(send_scheduled_task(REDUCE, localhost_id) != NONE){
			assigned ++;
		}
	}
	capacity[REDUCE][localhost_id] -= assigned;
}

/**
 * @brief  The heartbeat loop.
 */
static void heartbeat(void) {
	while (!job.finished) {
		//here we schedule as well...

//		if(config.amount_of_tasks[MAP] + config.amount_of_tasks[REDUCE] > task_completed){
//		anyway, we assign only the tasks to the ones that didn't executed that task yet


//		}

		// TODO to replace with BAR fault tolerant mechanism
		// send_sms(SMS_HEARTBEAT, DLT_MAILBOX);
		MSG_process_sleep(config.heartbeat_interval);
	}
}

/**
 * @brief  Process that listens for tasks.
 */
static int listen(int argc, char* argv[]) {
	char mailbox[MAILBOX_ALIAS_SIZE];
	msg_error_t status;
	msg_host_t localhost;
	msg_task_t current_task = NULL;
	task_info_t ti;
	size_t localhost_id;

	localhost = MSG_host_self();
	localhost_id = get_worker_id(localhost);
	sprintf(mailbox, TASKTRACKER_MAILBOX, localhost_id);
	scheduleRoutine(localhost_id); //first assignements
	while (!job.finished) {
		current_task = NULL;
		status = receive(&current_task, mailbox);

		if (message_is(current_task, SMS_FINISH)) {
			MSG_task_destroy(current_task);
			break;
		} else if (message_is(current_task, SMS_TASK_DONE)) {
			ti = (task_info_t) MSG_task_get_data(current_task);

			capacity[ti->phase][localhost_id]++;

			scheduleRoutine(localhost_id);

			MSG_task_destroy(current_task);
		}
	}

	return 0;
}


/**
 * @brief  Send a task to a worker.
 * @param  phase     The current job phase.
 * @param  tid       The task ID.
 * @param  data_src  The ID of the DataNode that owns the task data.
 * @param  wid       The destination worker id.
 */
void execute_task(enum phase_e phase, size_t tid, size_t data_src) {
	double cpu_required = 0.0;
	msg_task_t task = NULL;
	task_info_t task_info;
	msg_host_t localhost;
	size_t localhost_id;

	localhost = MSG_host_self();
	localhost_id = get_worker_id(localhost);

	//msg_error_t status;

	// for fault-tolerance we don't want to reassign a task to the same node
	xbt_assert(job.task_list[phase][tid][localhost_id] == NULL);

	cpu_required = user.task_cost_f(phase, tid, localhost_id);

	task_info = xbt_new(struct task_info_s, 1);
	task = MSG_task_create(SMS_TASK, cpu_required, 0.0, (void*) task_info);

	task_info->phase = phase;
	task_info->id = tid;
	task_info->src = data_src;
	task_info->wid = localhost_id;
	task_info->task = task;
	task_info->shuffle_end = 0.0;

	// for tracing purposes...
	MSG_task_set_category(task, (phase == MAP ? "MAP" : "REDUCE"));

	job.task_list[phase][tid][localhost_id] = task;

	if (job.task_status[phase][tid] == T_STATUS_TIP_SLOW){
		job.task_replicas_instances[phase][tid]++;
	}else{
		job.task_instances[phase][tid]++;
		if(job.task_instances[phase][tid] >= number_of_task_replicas())
			job.task_status[phase][tid] = T_STATUS_TIP;
	}

	TRACE_host_set_state(MSG_host_get_name(localhost), (phase == MAP ? "MAP" : "REDUCE"), "START");

#ifdef VERBOSE
	XBT_INFO ("TX: %s > %s", SMS_TASK, MSG_host_get_name (config.workers[wid]));
#endif

	MSG_process_create("compute", compute, task, localhost);
}

/**
 * @brief  Process that computes a task.
 *
 * We assign to this function directly a single task to execute
 */
int compute(int argc, char* argv[]) {
	msg_task_t task;
	task_info_t ti;
	xbt_ex_t e;
	char mailbox[MAILBOX_ALIAS_SIZE];
	msg_host_t localhost;
	size_t localhost_id;


	localhost = MSG_host_self();
	localhost_id = get_worker_id(localhost);


	task = (msg_task_t) MSG_process_get_data(MSG_process_self());
	ti = (task_info_t) MSG_task_get_data(task);
	ti->pid = MSG_process_self_PID();

	/*
	 * Obtain the data necessary for the computation
	 * */
	switch (ti->phase) {
	case MAP:

		TRACE_host_set_state(MSG_host_get_name(MSG_host_self()), "MAP-INPUT-FETCHING", "BEGIN");
		get_chunk(ti);
		TRACE_host_set_state(MSG_host_get_name(MSG_host_self()), "MAP-INPUT-FETCHING", "END");
		break;

	case REDUCE:
		TRACE_host_set_state(MSG_host_get_name(MSG_host_self()), "REDUCE-INPUT-FETCHING", "BEGIN");
		get_map_output(ti);
		TRACE_host_set_state(MSG_host_get_name(MSG_host_self()), "REDUCE-INPUT-FETCHING", "END");
		break;
	}

	#ifdef VERBOSE
		XBT_INFO ("INFO: start %s execution", ti->phase == MAP ? "MAP" : "REDUCE");
	#endif

	TRACE_host_set_state(MSG_host_get_name(MSG_host_self()), ti->phase == MAP ? "MAP-EXECUTION" : "REDUCE-EXECUTION", "BEGIN");
	if (job.task_status[ti->phase][ti->id] != T_STATUS_DONE) {

		TRY{
			MSG_task_execute(task);

			if (ti->phase == MAP)
				update_intermediate_result_owner(ti->id, get_worker_id(MSG_host_self()));
			}
		CATCH(e){
			xbt_assert(e.category == cancel_error, "%s", e.msg);
			xbt_ex_free(e);
		}
	}
	TRACE_host_set_state(MSG_host_get_name(MSG_host_self()), ti->phase == MAP ? "MAP-EXECUTION" : "REDUCE-EXECUTION", "END");


	if (!job.finished) {
		send(SMS_TASK_DONE, 0.0, 0.0, ti, DLT_MAILBOX);
		sprintf(mailbox, TASKTRACKER_MAILBOX, localhost_id);
		send(SMS_TASK_DONE, 0.0, 0.0, ti, mailbox);
	}
	return 0;
}

/**
 * @brief  Get the chunk associated to a map task.
 * 		   It just notify to the namenode that it needs a chuck and waits for a reply...
 * @param  ti  The task information.
 */
static void get_chunk(task_info_t ti) {
	char mailbox[MAILBOX_ALIAS_SIZE];
	msg_error_t status;
	msg_task_t task = NULL;
	size_t my_id;

	my_id = get_worker_id(MSG_host_self());

	/*
	 * Here the model have to be changed, why?
	 * The idea was that in general I have to require the data-chunk to just one simple node... and then the recovery mechanism in case of fault.
	 * */
#ifdef VERBOSE
	XBT_INFO ("INFO: start chunk copy");
#endif

	/* Request the chunk to the source node. if it's not me */
	if (ti->src != my_id) {
		sprintf(mailbox, DATANODE_MAILBOX, ti->src);
		status = send_sms(SMS_GET_CHUNK, mailbox);
		if (status == MSG_OK) {
			sprintf(mailbox, TASK_MAILBOX, my_id, MSG_process_self_PID());
			status = receive(&task, mailbox);
			if (status == MSG_OK)
				MSG_task_destroy(task);
		}
	}
#ifdef VERBOSE
	XBT_INFO ("INFO: end chunk copy");
#endif
}

/**
 * SHUFFLE PHASE
 *
 * @brief  Copy the itermediary pairs for a reduce task.
 * @param  ti  The task information.
 */
static void get_map_output(task_info_t ti) {
	char mailbox[MAILBOX_ALIAS_SIZE];
	msg_error_t status;
	msg_task_t data = NULL;
	size_t total_copied, must_copy;
	size_t my_id;
	size_t other_worker;
	size_t map_index;
	size_t* data_copied;

	my_id = get_worker_id(MSG_host_self());
	data_copied = xbt_new0(size_t, config.amount_of_tasks[MAP]);
	ti->map_output_copied = data_copied;
	total_copied = 0;
	must_copy = reduce_input_size(ti->id); //get the overall input size for this key

#ifdef VERBOSE
	XBT_INFO ("INFO: start copy");
#endif

	// I have to copy all the intermediate MAP results for my key
	for (map_index = 0; map_index < config.amount_of_tasks[MAP]; map_index++) {
		ti -> map_id = map_index;

		// FIXME can be cleaned better, for example using a signal...
		if (job.task_status[REDUCE][ti->id] == T_STATUS_DONE) {
			XBT_WARN("This reduces has already completed %lu", ti->id);
			xbt_free_ref(&data_copied);
			return;
		}

		// check if this map is finished without producing any data
		if(job.map_output[map_index][ti->id] <= 0 && job.task_status[MAP][map_index] == T_STATUS_DONE){
			XBT_VERB("This map %lu has produced nothing...", map_index);
			continue; //nothing to download from this map_id
		}

		// We need to wait the moment in which the task has completed to copy the data...
		// Of course we can do this process more efficient, without blocking the whole queue first
		// secondly streaming the intermediate data...

		//TODO change here in the future to support distributed storage, we just need to be sure that we are the producers of that intermediate result
		while(!map_output_owner[map_index][ti->id][my_id]){
			XBT_VERB("Waiting that the map %lu completes on the localhost...", map_index);
			MSG_process_sleep(3);
		}

		// check if I have it locally
		if(map_output_owner[map_index][ti->id][my_id]){

			data_copied[map_index] = user.map_output_f(map_index, ti->id);
			total_copied += user.map_output_f(map_index, ti->id);

			stats.reduce_local_map_result++;
		}else{
			// if this map id has produced data that I need and I didn't downloaded yet, I'll do it right now
			// loop on this map task until we'll finish to download all the data needed
			while(job.map_output[map_index][ti->id] > data_copied[map_index]){

				other_worker = find_random_intermediate_result_owner(map_index, ti->id);
				sprintf(mailbox, DATANODE_MAILBOX, other_worker);
				status = send(SMS_GET_INTER_PAIRS, 0.0, 0.0, ti, mailbox);

				if (status == MSG_OK) {
					data = NULL;

					sprintf(mailbox, TASK_MAILBOX, my_id,
							MSG_process_self_PID());
					status = receive(&data, mailbox);
					if (status == MSG_OK) {
						// update the current situation
						data_copied[map_index] += MSG_task_get_data_size(data);
						total_copied += MSG_task_get_data_size(data);
						MSG_task_destroy(data);
					}
				}
			}

			stats.reduce_remote_map_result++;
		}
	}

	xbt_assert(total_copied >= must_copy);

#ifdef VERBOSE
	XBT_INFO ("INFO: copy finished");
#endif
	ti->shuffle_end = MSG_get_clock();

	// TODO generate event for shuffle phase finished

	xbt_free_ref(&data_copied);
}

// vim: set ts=8 sw=4:
