
#include <stdio.h>
#include <stdlib.h>
#include <msg/msg.h>
#include <xbt/sysdep.h>
#include <xbt/log.h>
#include <xbt/asserts.h>
#include "common.h"
#include "worker.h"
#include "dfs.h"
#include "mrsg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "MRSG");

#define MAX_LINE_SIZE 256

int master(int argc, char *argv[]);
int worker(int argc, char *argv[]);

static void check_config(void);
static msg_error_t run_simulation(const char* platform_file,
		const char* deploy_file, const char* mr_config_file);
static void init_mr_config(const char* mr_config_file);
static void read_mr_config_file(const char* file_name);
static void init_config(void);
static void init_job(void);
static void init_stats(void);
static void free_global_mem(void);

int MRSG_main(const char* plat, const char* depl, const char* conf) {

	int argc = 8;
	char* argv[] = { "mrsg", "--cfg=tracing:1", "--cfg=tracing/buffer:1",
			"--cfg=tracing/filename:traces/tracefile.trace",
			"--cfg=tracing/categorized:1", "--cfg=tracing/uncategorized:1",
			"--cfg=viva/categorized:traces/cat.plist",
			"--cfg=viva/uncategorized:traces/uncat.plist"};
//	int argc = 3;
//	char* argv[] = { "mrsg", "--cfg=tracing:1", "--cfg=tracing/platform:1" };

	msg_error_t res = MSG_OK;

	config.initialized = 0;

	check_config();

	MSG_init(&argc, argv);
	res = run_simulation(plat, depl, conf);

	if (res == MSG_OK)
		return 0;
	else
		return 1;
}

/**
 * @brief Check if the user configuration is sound.
 */
static void check_config(void) {
	xbt_assert(user.task_cost_f != NULL, "Task cost function not specified.");
	xbt_assert(user.map_output_f != NULL, "Map output function not specified.");
}

/**
 * @param  platform_file   The path/name of the platform file.
 * @param  deploy_file     The path/name of the deploy file.
 * @param  mr_config_file  The path/name of the configuration file.
 */
static msg_error_t run_simulation(const char* platform_file,
		const char* deploy_file, const char* mr_config_file) {
	msg_error_t res = MSG_OK;

	read_mr_config_file(mr_config_file);

	MSG_create_environment(platform_file);

	// for tracing purposes..
	TRACE_category_with_color("MAP", "1 0 0");
	TRACE_category_with_color("REDUCE", "0 0 1");

	MSG_function_register("master", master);
	MSG_function_register("worker", worker);
	MSG_launch_application(deploy_file);

	init_mr_config(mr_config_file);

	res = MSG_main();

	free_global_mem();

	return res;
}

/**
 * @brief  Initialize the MapReduce configuration.
 * @param  mr_config_file  The path/name of the configuration file.
 */
static void init_mr_config(const char* mr_config_file) {
	srand(config.random_seed);
	init_config();
	init_stats();
	init_job();
	distribute_data();
}

/**
 * @brief  Read the MapReduce configuration file.
 * @param  file_name  The path/name of the configuration file.
 */
static void read_mr_config_file(const char* file_name) {
	char property[256];
	FILE* file;

	/* Set the default configuration. */
	config.chunk_size = 67108864;
	config.chunk_count = 0;
	config.chunk_replicas = 3;
	config.slots[MAP] = 2;
	config.amount_of_tasks[REDUCE] = 1;
	config.slots[REDUCE] = 2;
	config.byzantine = 0;
	config.real_byzantine = -1;
	config.block_size = 100;
	config.block_period = 15;


	/* Read the user configuration file. */
	file = fopen(file_name, "r");

	xbt_assert(file != NULL, "Error reading cofiguration file: %s", file_name);

	while (fscanf(file, "%256s", property) != EOF) {
		if (strcmp(property, "chunk_size") == 0) {
			fscanf(file, "%lg", &config.chunk_size);
			config.chunk_size *= 1024 * 1024; /* MB -> bytes */
		} else if (strcmp(property, "input_chunks") == 0) {
			fscanf(file, "%d", &config.chunk_count);
		} else if (strcmp(property, "dfs_replicas") == 0) {
			fscanf(file, "%d", &config.chunk_replicas);
		} else if (strcmp(property, "map_slots") == 0) {
			fscanf(file, "%d", &config.slots[MAP]);
		} else if (strcmp(property, "reduces") == 0) {
			fscanf(file, "%d", &config.amount_of_tasks[REDUCE]);
		} else if (strcmp(property, "reduce_slots") == 0) {
			fscanf(file, "%d", &config.slots[REDUCE]);
		} else if (strcmp(property, "byzantine") == 0) {
			fscanf(file, "%d", &config.byzantine);
		} else if (strcmp(property, "real_byzantine") == 0) {
					fscanf(file, "%d", &config.real_byzantine);
		} else if (strcmp(property, "block_period") == 0) {
			fscanf(file, "%d", &config.block_period);
		} else if (strcmp(property, "block_size") == 0) {
			fscanf(file, "%d", &config.block_size);
		} else if (strcmp(property, "random_seed") == 0) {
			fscanf(file, "%d", &config.random_seed);
		} else {
			printf("Error: Property %s is not valid. (in %s)", property,
					file_name);
			exit(1);
		}
	}

	fclose(file);

	/* Assert the configuration values. */

	xbt_assert(config.chunk_size > 0, "Chunk size must be greater than zero");
	xbt_assert(config.chunk_count > 0,
			"The amount of input chunks must be greater than zero");
	xbt_assert(config.chunk_replicas > 0,
			"The amount of chunk replicas must be greater than zero");
	xbt_assert(config.slots[MAP] > 0, "Map slots must be greater than zero");
	xbt_assert(config.amount_of_tasks[REDUCE] >= 0,
			"The number of reduce tasks can't be negative");
	xbt_assert(config.slots[REDUCE] > 0,
			"Reduce slots must be greater than zero");
	xbt_assert(config.byzantine >= 0,
				"Byzantine nodes percentage must be at least zero");
	xbt_assert(config.byzantine <= 100,
				"Byzantine nodes percentage must be at maximum 100");
	xbt_assert(config.block_period >= 0,
			"Period of block creation in seconds must be at least zero");
	xbt_assert(config.block_size >= 0,
			"Number of messages per block must be at least zero");
}

/**
 * @brief  Initialize the config structure.
 */
static void init_config(void) {
	const char* process_name = NULL;
	msg_host_t host;
	msg_process_t process;
	size_t wid;
	unsigned int cursor;
	w_info_t wi;
	xbt_dynar_t process_list;

	/* Initialize hosts information. */

	config.number_of_workers = 0;

	process_list = MSG_processes_as_dynar();
	xbt_dynar_foreach (process_list, cursor, process)
	{
		process_name = MSG_process_get_name(process);
		if (strcmp(process_name, "worker") == 0)
			config.number_of_workers++;
	}

	config.workers = xbt_new(msg_host_t, config.number_of_workers);

	capacity[MAP] = xbt_new0(int, config.number_of_workers);
	capacity[REDUCE] = xbt_new0(int, config.number_of_workers);
	for(wid=0; wid<config.number_of_workers; wid++){
		capacity[MAP][wid] = config.slots[MAP];
		capacity[REDUCE][wid] = config.slots[REDUCE];
	}

	//update the real number of byzantine nodes
	config.byzantine = config.number_of_workers*config.byzantine/100;
	if(config.real_byzantine != -1) {
		config.real_byzantine = config.number_of_workers*config.real_byzantine/100;
	}else{
		config.real_byzantine = config.byzantine;
	}
	wid = 0;
	config.grid_cpu_power = 0.0;
	xbt_dynar_foreach (process_list, cursor, process)
	{
		process_name = MSG_process_get_name(process);
		host = MSG_process_get_host(process);
		if (strcmp(process_name, "worker") == 0) {
			config.workers[wid] = host;
			/* Set the worker ID as its data. */
			wi = xbt_new(struct w_info_s, 1);
			wi->wid = wid;
			MSG_host_set_data(host, (void*) wi);
			/* Add the worker's cpu power to the grid total. */
			config.grid_cpu_power += MSG_get_host_speed(host);
			wid++;
		}
	}
	config.grid_average_speed = config.grid_cpu_power
			/ config.number_of_workers;
	config.heartbeat_interval = maxval(HEARTBEAT_MIN_INTERVAL,
			config.number_of_workers / 100);
	config.amount_of_tasks[MAP] = config.chunk_count;

	config.initialized = 1;
}

/**
 * @brief  Initialize the job structure.
 */
static void init_job(void) {
	int i, byz_index;
	size_t wid;

	xbt_assert(config.initialized,
			"init_config has to be called before init_job");

	w_queue_workers = xbt_new(w_queue_worker_t, config.number_of_workers);

	// w_queue_workers = xbt_new(struct w_queues_worker, config.number_of_workers);
	for(i=0; i<config.number_of_workers; i++){
		w_queue_workers[i].map_tasks_queue = xbt_queue_new(config.amount_of_tasks[MAP], sizeof(msg_task_t));
		w_queue_workers[i].size_queue_map = 0;
		w_queue_workers[i].reduce_tasks_queue = xbt_queue_new(config.amount_of_tasks[REDUCE], sizeof(msg_task_t));
		w_queue_workers[i].size_queue_reduce = 0;
	}

	job.worker_active_count = 0;
	job.finished = 0;
	job.heartbeats = xbt_new(struct heartbeat_s, config.number_of_workers);
	for (wid = 0; wid < config.number_of_workers; wid++) {
		job.heartbeats[wid].slots_av[MAP] = config.slots[MAP];
		job.heartbeats[wid].slots_av[REDUCE] = config.slots[REDUCE];
	}

	job.byzantine_flag = xbt_new0(int, config.number_of_workers);
	// assign the byzantine nodes randomly
	for(i=0; i<config.real_byzantine; i++){
		byz_index = rand() % config.number_of_workers;
		if(job.byzantine_flag[byz_index]) {
			//already assigned byzantine node
			i--;
			continue;
		}else{
			job.byzantine_flag[byz_index] = 1;
		}
	}

	/* Initialize map information. */
	job.tasks_pending[MAP] = config.amount_of_tasks[MAP];
	job.task_status[MAP] = xbt_new0(int, config.amount_of_tasks[MAP]);
	job.task_instances[MAP] = xbt_new0(int, config.amount_of_tasks[MAP]);

	job.task_replicas_instances[MAP] = xbt_new0(int,
			config.amount_of_tasks[MAP]);
	job.task_confirmations[MAP] = xbt_new0(int, config.amount_of_tasks[MAP]);
    job.task_byzantine_confirmations[MAP] = xbt_new0(int, config.amount_of_tasks[MAP]);
	job.task_list[MAP] = xbt_new0(msg_task_t*, config.amount_of_tasks[MAP]);
	for (i = 0; i < config.amount_of_tasks[MAP]; i++)
		job.task_list[MAP][i] = xbt_new0(msg_task_t, config.number_of_workers);

	job.map_output = xbt_new(size_t*, config.amount_of_tasks[MAP]);
	for (i = 0; i < config.amount_of_tasks[MAP]; i++)
		job.map_output[i] = xbt_new0(size_t, config.amount_of_tasks[REDUCE]);

	/* Initialize reduce information. */
	job.tasks_pending[REDUCE] = config.amount_of_tasks[REDUCE];
	job.task_status[REDUCE] = xbt_new0(int, config.amount_of_tasks[REDUCE]);
	job.task_instances[REDUCE] = xbt_new0(int, config.amount_of_tasks[REDUCE]);
	job.task_replicas_instances[REDUCE] = xbt_new0(int,
			config.amount_of_tasks[REDUCE]);
	job.task_confirmations[REDUCE] = xbt_new0(int,
			config.amount_of_tasks[REDUCE]);
    job.task_byzantine_confirmations[REDUCE] = xbt_new0(int, config.amount_of_tasks[REDUCE]);
	job.task_list[REDUCE] = xbt_new0(msg_task_t*,
			config.amount_of_tasks[REDUCE]);
	for (i = 0; i < config.amount_of_tasks[REDUCE]; i++)
		job.task_list[REDUCE][i] = xbt_new0(msg_task_t,
				config.number_of_workers);

}

/**
 * @brief  Initialize the stats structure.
 */
static void init_stats(void) {
	xbt_assert(config.initialized,
			"init_config has to be called before init_stats");

	stats.map_local = 0;
	stats.map_remote = 0;
	stats.map_spec_l = 0;
	stats.map_spec_r = 0;
	stats.reduce_normal = 0;
	stats.reduce_spec = 0;
	stats.reduce_remote_map_result = 0;
	stats.reduce_local_map_result = 0;
}

/**
 * @brief  Free allocated memory for global variables.
 */
static void free_global_mem(void) {
	size_t i, j;
	int phases[2];
	phases[0] = MAP;
	phases[1] = REDUCE;
	int phase_index, phase;

	// free dfs memory
	for (i = 0; i < config.chunk_count; i++)
		xbt_free_ref(&chunk_owner[i]);
	xbt_free_ref(&chunk_owner);

	for (i = 0; i < config.amount_of_tasks[MAP]; i++) {
		for (j = 0; j < config.amount_of_tasks[REDUCE]; j++) {
			xbt_free_ref(&map_output_owner[i][j]);
		}
		xbt_free_ref(&map_output_owner[i]);
	}
	xbt_free_ref(&map_output_owner);
	/*
	for(i=0; i<config.number_of_workers; i++){
		xbt_queue_free(&w_queue_workers[i].map_tasks_queue);
		xbt_queue_free(&w_queue_workers[i].reduce_tasks_queue);
	}
	xbt_free_ref(&w_queue_workers);
	*/
	// free config memory
	xbt_free_ref(&config.workers);

	// free job memory
	for (phase_index = 0; phase_index < 2; phase_index++) {
		phase = phases[phase_index];
		xbt_free_ref(&job.task_status[phase]);
		xbt_free_ref(&job.task_instances[phase]);
		xbt_free_ref(&job.task_replicas_instances[phase]);
		xbt_free_ref(&job.task_confirmations[phase]);
	}

	xbt_free_ref(&job.heartbeats);

	// job->task list
	for (phase_index = 0; phase_index < 2; phase_index++) {
		phase = phases[phase_index];
		for (i = 0; i < config.amount_of_tasks[phase]; i++) {
			xbt_free_ref(&job.task_list[phase][i]);
		}
		xbt_free_ref(&job.task_list[phase]);
	}

	xbt_free_ref(&job.byzantine_flag);

	// job->map_output
	for (i = 0; i < config.amount_of_tasks[MAP]; i++) {
		xbt_free_ref(&job.map_output[i]);
	}
	xbt_free_ref(&job.map_output);

}

// vim: set ts=8 sw=4:
