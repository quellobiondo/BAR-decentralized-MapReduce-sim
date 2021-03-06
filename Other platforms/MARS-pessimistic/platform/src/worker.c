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

	job.worker_active_count++;

	heartbeat();

	sprintf(mailbox, DATANODE_MAILBOX, get_worker_id(me));
	send_sms(SMS_FINISH, mailbox);

	return 0;
}

/**
 * @brief  The heartbeat loop.
 */
static void heartbeat(void) {
	while (!job.finished) {
		// TODO to replace with BAR fault tolerant mechanism
		// send_sms(SMS_HEARTBEAT, DLT_MAILBOX);
		MSG_process_sleep(config.heartbeat_interval);
	}
}

/**
 * @brief  Process that listens for tasks.
 */
static int listen(int argc, char* argv[]) {
	char mailbox_map[MAILBOX_ALIAS_SIZE];
	char mailbox_reduce[MAILBOX_ALIAS_SIZE];
	char mailbox_completed[MAILBOX_ALIAS_SIZE];

	MSG_mailbox_set_async(mailbox_map);
	MSG_mailbox_set_async(mailbox_reduce);

	msg_error_t status;
	msg_host_t localhost;
	msg_task_t current_task = NULL;
	task_info_t ti;
	size_t localhost_id;

	//xbt_queue_t map_tasks_queue = xbt_queue_new(config.amount_of_tasks[MAP], sizeof(msg_task_t));
	//xbt_queue_t reduce_tasks_queue = xbt_queue_new(config.amount_of_tasks[REDUCE], sizeof(msg_task_t));
	//int map_queue_size = 0;
	//int reduce_queue_size = 0;

	localhost = MSG_host_self();
	localhost_id = get_worker_id(localhost);

	sprintf(mailbox_completed, COMPLETED_TASKTRACKER_MAILBOX, localhost_id);
	sprintf(mailbox_map, MAP_TASKTRACKER_MAILBOX, localhost_id);
	sprintf(mailbox_reduce, REDUCE_TASKTRACKER_MAILBOX, localhost_id);

	while (!job.finished) {

		if(MSG_task_listen(mailbox_completed)){
			current_task = NULL;

			status = receive(&current_task, mailbox_completed);

			xbt_assert(status == MSG_OK);

			if (message_is(current_task, SMS_TASK_DONE_CORRECT)) {
				ti = (task_info_t) MSG_task_get_data(current_task);

				capacity[ti->phase][localhost_id]++;

				if(job.byzantine_flag[localhost_id]){
					send(SMS_TASK_DONE_BYZANTINE, 0.0, 0.0, ti, DLT_MAILBOX);
				}else{
					send(SMS_TASK_DONE_CORRECT, 0.0, 0.0, ti, DLT_MAILBOX);
				}
			} else {
				XBT_WARN("Received unexpected message");
			}
		}

		/*
		 *
		 if(MSG_task_listen(mailbox_map)){
			current_task = NULL;

			// there is a MAP task
			status = receive(&current_task, mailbox_map);

			xbt_assert(status == MSG_OK);

			if (message_is(current_task, SMS_TASK)) {
				xbt_queue_push(map_tasks_queue, &current_task);
				map_queue_size++;
			} else {
				XBT_WARN("Received unexpected message");
			}
		}
		*/
		while(capacity[MAP][localhost_id] > 0 && w_queue_workers[localhost_id].size_queue_map > 0){
			xbt_queue_pop(w_queue_workers[localhost_id].map_tasks_queue, &current_task);
			w_queue_workers[localhost_id].size_queue_map--;
			capacity[MAP][localhost_id]--;
			MSG_process_create("compute", compute, current_task, localhost);
		}
/*
		if(MSG_task_listen(mailbox_reduce)){
			current_task = NULL;

			// there is a REDUCE task
			status = receive(&current_task, mailbox_reduce);

			xbt_assert(status == MSG_OK);

			if (message_is(current_task, SMS_TASK)) {
				xbt_queue_push(reduce_tasks_queue, &current_task);
				reduce_queue_size++;
			} else {
				XBT_WARN("Received unexpected message");
			}
		}
*/
		while(capacity[REDUCE][localhost_id] > 0 && w_queue_workers[localhost_id].size_queue_reduce > 0){
			xbt_queue_pop(w_queue_workers[localhost_id].reduce_tasks_queue, &current_task);
			w_queue_workers[localhost_id].size_queue_reduce--;
			capacity[REDUCE][localhost_id]--;
			MSG_process_create("compute", compute, current_task, localhost);
		}
		MSG_process_sleep(0.5);

	}

	xbt_queue_free(&w_queue_workers[localhost_id].map_tasks_queue);
	xbt_queue_free(&w_queue_workers[localhost_id].reduce_tasks_queue);

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
		TRACE_host_set_state(MSG_host_get_name(localhost), "MAP-INPUT-FETCHING", "BEGIN");
		get_chunk(ti);
		TRACE_host_set_state(MSG_host_get_name(localhost), "MAP-INPUT-FETCHING", "END");
		break;

	case REDUCE:
		TRACE_host_set_state(MSG_host_get_name(localhost), "REDUCE-INPUT-FETCHING", "BEGIN");
		get_map_output(ti);
		TRACE_host_set_state(MSG_host_get_name(MSG_host_self()), "REDUCE-INPUT-FETCHING", "END");
		break;
	}

	XBT_VERB("INFO: start %s execution of task %lu", ti->phase == MAP ? "MAP" : "REDUCE", ti->id);

	TRACE_host_set_state(MSG_host_get_name(MSG_host_self()), ti->phase == MAP ? "MAP-EXECUTION" : "REDUCE-EXECUTION", "BEGIN");
	if (job.task_status[ti->phase][ti->id] != T_STATUS_DONE) {

		TRY{
			status = MSG_task_execute(task);

			if (ti->phase == MAP && status == MSG_OK)
				update_intermediate_result_owner(ti->id, get_worker_id(MSG_host_self()));
			}
		CATCH(e){
			xbt_assert(e.category == cancel_error, "%s", e.msg);
			xbt_ex_free(e);
		}
	}
	TRACE_host_set_state(MSG_host_get_name(MSG_host_self()), ti->phase == MAP ? "MAP-EXECUTION" : "REDUCE-EXECUTION", "END");

	/*
	 * How the heartbeats are incremented
	 * What is slots_av?
	 * */
	// NOT HERE, BUT IN THE SCHEDULER job.heartbeats[ti->wid].slots_av[ti->phase]++;

	// TODO: update, send just to the blockchain
	if (!job.finished) {
		sprintf(mailbox, COMPLETED_TASKTRACKER_MAILBOX, localhost_id);
		send(SMS_TASK_DONE_CORRECT, 0.0, 0.0, ti, mailbox);
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
			XBT_INFO("Reduce already completed");
			xbt_free_ref(&data_copied);
			return;
		}

		// check if this map is finished without producing any data
		if(job.map_output[map_index][ti->id] <= 0 && job.task_status[MAP][map_index] == T_STATUS_DONE){
			XBT_INFO("No results for this map output");
			continue; //nothing to download from this map_id
		}

		// We need to wait the moment in which the task has completed to copy the data...
		// Of course we can do this process more efficient, without blocking the whole queue first
		// secondly streaming the intermediate data...
		while(job.task_status[MAP][map_index] != T_STATUS_DONE){
			MSG_process_sleep(1);
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
