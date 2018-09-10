

#include <msg/msg.h>
#include "common.h"
#include "worker.h"
#include "dfs.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(msg_test);

static void send_data(msg_task_t msg);

/*
 * The assignement of chunk presence and nodes is made with a matrix.
 */
void distribute_data(void) {
	size_t chunk, map_task_index, reduce_task_index;

	/* Allocate memory for the mapping matrix. */
	chunk_owner = xbt_new(char*, config.chunk_count);
	for (chunk = 0; chunk < config.chunk_count; chunk++) {
		chunk_owner[chunk] = xbt_new0(char, config.number_of_workers);
	}

	/* Allocate memory for the Map Output matrix */
	map_output_owner = xbt_new(char**, config.amount_of_tasks[MAP]);
	for(map_task_index = 0; map_task_index < config.amount_of_tasks[MAP]; map_task_index++){
		map_output_owner[map_task_index] = xbt_new(char*, config.amount_of_tasks[REDUCE]);
		for(reduce_task_index = 0; reduce_task_index < config.amount_of_tasks[REDUCE]; reduce_task_index++){
			map_output_owner[map_task_index][reduce_task_index] = xbt_new0(char, config.number_of_workers);
		}
	}

	/* Call the distribution function. */
	user.dfs_f(chunk_owner, config.chunk_count, config.number_of_workers,
			config.chunk_replicas);
}

/*
 * Default data distribution function: all the chunks to the nodes without notions of locality.
 */
void default_dfs_f(char** dfs_matrix, size_t chunks, size_t workers,
		int replicas) {
	int r;
	size_t chunk;
	size_t owner;

	if (config.chunk_replicas >= config.number_of_workers) {
		/* All workers own every chunk. */
		for (chunk = 0; chunk < config.chunk_count; chunk++) {
			for (owner = 0; owner < config.number_of_workers; owner++) {
				chunk_owner[chunk][owner] = 1;
			}
		}
	} else {
		/* Ok, it's a typical distribution. */
		for (chunk = 0; chunk < config.chunk_count; chunk++) {
			for (r = 0; r < config.chunk_replicas; r++) {
				owner = ((chunk % config.number_of_workers)
						+ ((config.number_of_workers / config.chunk_replicas)*r))
								% config.number_of_workers;

				chunk_owner[chunk][owner] = 1;
			}
		}
	}
}

/*
 * Return one of the many chunk_owner, almost randomly
 * TODO Design a better algo...
 */
size_t find_random_chunk_owner(size_t cid) {
	int replica;
	size_t owner = NONE;
	size_t wid;

	replica = rand() % config.chunk_replicas;

	for (wid = 0; wid < config.number_of_workers; wid++) {
		if (chunk_owner[cid][wid]) {
			owner = wid;

			if (replica == 0)
				break;
			else
				replica--;
		}
	}

	xbt_assert(owner != NONE, "Aborted: chunk %lu is missing.", cid);

	return owner;
}

/*
 * Return a random node among the ones that have completed that key
 */
size_t find_random_intermediate_result_owner(size_t map_id, size_t reduce_id){
	int replica;
	size_t owner = NONE;
	size_t wid;

	replica = rand() % config.chunk_replicas;

	for (wid = 0; wid < config.number_of_workers; wid++) {
		if (map_output_owner[map_id][reduce_id][wid]) {
			owner = wid;

			if (replica == 0)
				break;
			else
				replica--;
		}
	}

	xbt_assert(owner != NONE, "Aborted: no-one has the intermediate data for the key %lu.", reduce_id);

	return owner;
}

void update_intermediate_result_owner(size_t map_id, size_t owner){
	size_t rid;
	// for each reduce task we assign the amount of data produced by this map
	// so we can say that they are like making an uniform distribution
	// however, the function map_output_f takes the map id and the reduce id to compute another function, definible inside the experiment
	for (rid = 0; rid < config.amount_of_tasks[REDUCE]; rid++){
		job.map_output[map_id][rid] = user.map_output_f(map_id, rid);
        map_output_owner[map_id][rid][owner] = TRUE;
	}
}

int data_node(int argc, char* argv[]) {
	char mailbox[MAILBOX_ALIAS_SIZE];
	msg_error_t status;
	msg_task_t msg = NULL;

	sprintf(mailbox, DATANODE_MAILBOX, get_worker_id(MSG_host_self()));

	while (!job.finished) {
		msg = NULL;
		status = receive(&msg, mailbox);
		if (status == MSG_OK) {
			if (message_is(msg, SMS_FINISH)) {
				MSG_task_destroy(msg);
				break;
			} else {
				send_data(msg);
			}
		}
	}

	return 0;
}

static void send_data(msg_task_t msg) {
	char mailbox[MAILBOX_ALIAS_SIZE];
	double data_size;
	task_info_t ti;

	//my_id = get_worker_id(MSG_host_self());
	sprintf(mailbox, TASK_MAILBOX, get_worker_id(MSG_task_get_source(msg)),
			MSG_process_get_PID(MSG_task_get_sender(msg)));

	if (message_is(msg, SMS_GET_CHUNK)) {
		// send a task (that is a message) to the mailbox that is acting as alias of the other physical node
		MSG_task_dsend(MSG_task_create("DATA-C", 0.0, config.chunk_size, NULL),
				mailbox, NULL);
	} else if (message_is(msg, SMS_GET_INTER_PAIRS)) {
		ti = (task_info_t) MSG_task_get_data(msg);

		// compute the amount of data that the reducer has already copied
		data_size = job.map_output[ti->map_id][ti->id] - ti->map_output_copied[ti->map_id];

		MSG_task_dsend(MSG_task_create("DATA-IP", 0.0, data_size, NULL),
				mailbox, NULL);
	}

	MSG_task_destroy(msg);
}

// vim: set ts=8 sw=4:
