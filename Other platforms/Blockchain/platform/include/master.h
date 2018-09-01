#ifndef INCLUDE_MASTER_H_
#define INCLUDE_MASTER_H_


void print_config(void);
void print_stats(void);

void update_stats(enum task_type_e task_type);

void finish_all_task_copies(task_info_t ti);
int enough_result_confirmation(task_info_t ti);
void processTaskCompletion(task_info_t ti, msg_host_t worker);

int *capacity[2]; //[PHASE][WORKER] number of free slots in that worker

#endif /* INCLUDE_MASTER_H_ */
