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
#include "master.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(msg_test);

static FILE* tasks_log;

/** @brief  Main master function. */
int master(int argc, char* argv[]) {
//	heartbeat_t heartbeat;
	msg_error_t status;
	msg_host_t worker;
	msg_task_t msg = NULL, original_msg = NULL;
	DLT_block_t block = NULL;
	task_info_t ti;
	msg_process_t DLT_process;
	int tx_counter, i;

	capacity[MAP] = xbt_new0(int, config.number_of_workers);
	capacity[REDUCE] = xbt_new0(int, config.number_of_workers);
	for(i=0; i<config.number_of_workers; i++){
		capacity[MAP][i] = config.slots[MAP];
		capacity[REDUCE][i] = config.slots[REDUCE];
	}


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

	tasks_log = fopen("tasks.csv", "w");
	fprintf(tasks_log, "task_id,phase,worker_id,time,action,shuffle_end\n");

	// while we have at least a task pending (MAP/REDUCE)
	while (job.tasks_pending[MAP] + job.tasks_pending[REDUCE] > 0) {

		msg = NULL;
		status = receiveTimeout(&msg, MASTER_MAILBOX, 1);

		if (status == MSG_OK) {
			xbt_assert(message_is(msg, SMS_DLT_BLOCK), "Master received a message that is not from the DLT!");
			block = (DLT_block_t) MSG_task_get_data(msg);
			#ifdef VERBOSE
				XBT_INFO ("INFO: Received a BLOCK");
			#endif

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

	xbt_free_f(capacity[MAP]);
	xbt_free_f(capacity[REDUCE]);

	fclose(tasks_log);

	job.finished = 1;

	print_config();
	print_stats();

	MSG_process_kill(DLT_process);

	XBT_INFO("JOB END");

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

/** @brief  Print the job configuration. */
void print_config(void) {
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
void print_stats(void) {
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
int enough_result_confirmation(task_info_t ti){
	return config.number_of_workers;
}

void update_stats(enum task_type_e task_type) {
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

/**
 * @brief  Kill all copies of a task.
 * @param  ti  The task information of any task instance.
 */
void finish_all_task_copies(task_info_t ti) {
	int i;
	int phase = ti->phase;
	size_t tid = ti->id;

	for (i = 0; i < config.number_of_workers; i++) {
		if (job.task_list[phase][tid][i] != NULL) {
			MSG_task_cancel(job.task_list[phase][tid][i]);

			//FIXME: MSG_task_destroy (job.task_list[phase][tid][i]);
			//job.task_list[phase][tid][i] = NULL;
			fprintf(tasks_log, "%d_%zu_%d,%s,%zu,%.3f,END,%.3f\n", ti->phase,
					tid, i, (ti->phase == MAP ? "MAP" : "REDUCE"), ti->wid,
					MSG_get_clock(), ti->shuffle_end);
		}
	}
}

// vim: set ts=8 sw=4:
