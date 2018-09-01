#include <mrsg.h>
#include <stdio.h>
#include <string.h>

/**
 * User function that indicates the amount of bytes
 * that a map task will emit to a reduce task.
 *
 * @param  mid  The ID of the map task.
 * @param  rid  The ID of the reduce task.
 * @return The amount of data emitted (in bytes).
 */
int my_map_output_function (size_t mid, size_t rid)
{
    return 4*1024*1024;
}


/**
 * User function that indicates the cost of a task.
 *
 * @param  phase  The execution phase.
 * @param  tid    The ID of the task.
 * @param  wid    The ID of the worker that received the task.
 * @return The task cost in FLOPs.
 */
double my_task_cost_function (enum phase_e phase, size_t tid, size_t wid)
{
    switch (phase)
    {
	case MAP:
	    return 1e+11;

	case REDUCE:
	    return 5e+11;
    }
}

#define MAX_FILE_NAME_LENGTH 50

/*
 * Optional parameters
 * [ConfigurationFile] ["Topology"] ["Platform"]
 */
int main (int argc, char* argv[])
{
	char platform_file[MAX_FILE_NAME_LENGTH], deploy_file[MAX_FILE_NAME_LENGTH], conf_file[MAX_FILE_NAME_LENGTH];

	if(argc != 1){
		if(argc != 4){
			printf("Usage Error: hello [<ConfigurationFile> <Topology> <Platform>]\n");
			exit(1);
		}
		strncpy(conf_file, argv[1], MAX_FILE_NAME_LENGTH);
		strncpy(deploy_file, argv[2], MAX_FILE_NAME_LENGTH);
		strncpy(platform_file, argv[3], MAX_FILE_NAME_LENGTH);
	}else{
		strncpy(platform_file, "platforms/g5k.xml", MAX_FILE_NAME_LENGTH);
		strncpy(deploy_file, "deployments/Cluster-10.deploy.xml", MAX_FILE_NAME_LENGTH);
		strncpy(conf_file, "configurations/hello.conf", MAX_FILE_NAME_LENGTH);
	}

    /* MRSG_init must be called before setting the user functions. */
    MRSG_init ();
    /* Set the task cost function. */
    MRSG_set_task_cost_f (my_task_cost_function);
    /* Set the map output function. */
    MRSG_set_map_output_f (my_map_output_function);
    /* Run the simulation. */
    MRSG_main (platform_file, deploy_file, conf_file);

    return 0;
}

