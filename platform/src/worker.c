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

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(msg_test); // @suppress("Unused variable declaration in file scope")

static void heartbeat(void);
static int listen(int argc, char* argv[]);
static int compute(int argc, char* argv[]);
static void get_chunk(task_info_t ti);
static void get_map_output(task_info_t ti);

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

/**
 * @brief  The heartbeat loop.
 */
static void heartbeat(void) {
	while (!job.finished) {

		// to replace with BAR fault tolerant mechanism
		send_sms(SMS_HEARTBEAT, MASTER_MAILBOX);
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

	localhost = MSG_host_self();
	sprintf(mailbox, TASKTRACKER_MAILBOX, get_worker_id(localhost));

	while (!job.finished) {
		current_task = NULL;
		status = receive(&current_task, mailbox);

		if (status == MSG_OK && message_is(current_task, SMS_TASK)) {
			MSG_process_create("compute", compute, current_task, localhost);
		} else if (message_is(current_task, SMS_FINISH)) {
			MSG_task_destroy(current_task);
			break;
		}
	}

	return 0;
}

/**
 * @brief  Process that computes a task.
 *
 * We assign to this function directly a single task to execute
 */
static int compute(int argc, char* argv[]) {
	msg_error_t status;
	msg_task_t task;
	task_info_t ti;
	xbt_ex_t e;

	task = (msg_task_t) MSG_process_get_data(MSG_process_self());
	ti = (task_info_t) MSG_task_get_data(task);
	ti->pid = MSG_process_self_PID();

	/*
	 * Obtain the data necessary for the computation
	 * */
	switch (ti->phase) {
	case MAP:
		get_chunk(ti);
		break;

	case REDUCE:
		get_map_output(ti);
		break;
	}

	/*
	 * Execution in case we didn't executed yet
	 *
	 * TODO: remove the check? We need f+1 checks
	 */
	if (job.task_status[ti->phase][ti->id] != T_STATUS_DONE) {

		TRY{
			status = MSG_task_execute(task);

			if (ti->phase == MAP && status == MSG_OK)
				update_intermediate_result_owner(get_worker_id(MSG_host_self()), ti->id);
			}
		CATCH(e){
			xbt_assert(e.category == cancel_error, "%s", e.msg);
			xbt_ex_free(e);
		}
	}

	/*
	 * How the heartbeats are incremented
	 * What is slots_av?
	 * */
	job.heartbeats[ti->wid].slots_av[ti->phase]++;

	// TODO: update, send just to the blockchain
	if (!job.finished)
		send(SMS_TASK_DONE, 0.0, 0.0, ti, MASTER_MAILBOX);

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
	must_copy = reduce_input_size(ti->id);

#ifdef VERBOSE
	XBT_INFO ("INFO: start copy");
#endif

	// I have to copy all the intermediate MAP results for my key
	for (map_index = 0; map_index < config.amount_of_tasks[MAP]; ) {

		// why is this done like this
		if (job.task_status[REDUCE][ti->id] == T_STATUS_DONE) {
			xbt_free_ref(&data_copied);
			return;
		}

		// check if I have it locally
		if(map_output_owner[map_index][ti->id][my_id]){
			data_copied[map_index] = user.map_output_f(map_index, ti->id);
			total_copied += user.map_output_f(map_index, ti->id);
			stats.reduce_local_map_result++;
		}else{
			//find someone else
			// if this worker (wid) has data that I need and I didn't downloaded yet, I'll do it right now
			// TODO: we have to modify the way in which we take the data from the nodes, there isn't always a 1:1 correspondance
			other_worker = find_random_intermediate_result_owner(ti->id);
			stats.reduce_remote_map_result++;
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

		if(job.map_output[map_index][ti->id] <= data_copied[map_index]){
			map_index++;
		}
	}

	xbt_assert(total_copied >= must_copy);

	XBT_INFO("Shuffle phase finished for %lu", my_id);

#ifdef VERBOSE
	XBT_INFO ("INFO: copy finished");
#endif
	ti->shuffle_end = MSG_get_clock();

	xbt_free_ref(&data_copied);
}

// vim: set ts=8 sw=4:
