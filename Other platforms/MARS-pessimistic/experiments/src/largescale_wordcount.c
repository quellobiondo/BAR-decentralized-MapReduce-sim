#include <mrsg.h>
#include <common.h>

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
	int bytes_per_map = 1024*1024*config.block_size;
	//homogeneous
	return bytes_per_map/21;
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
	    return 1e+11;
    }
}

int main (int argc, char* argv[])
{
    /* MRSG_init must be called before setting the user functions. */
    MRSG_init ();
    /* Set the task cost function. */
    MRSG_set_task_cost_f (my_task_cost_function);
    /* Set the map output function. */
    MRSG_set_map_output_f (my_map_output_function);
    /* Run the simulation. */
    MRSG_main ("platforms/g5k.xml", "deployments/medium_cluster.deploy.xml", "configurations/largescale_wordcount.conf");

    return 0;
}

