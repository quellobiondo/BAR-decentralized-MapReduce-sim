#ifndef DFS_H
#define DFS_H

/** @brief  Matrix that maps chunks to workers. */
char**  chunk_owner;
/** @brief  Matrix that maps map output for each key and on which workers these intermediate results can be found. */
char***  map_output_owner;

/**
 * @brief  Distribute chunks (and replicas) to DataNodes.
 */
void distribute_data (void);

/**
 * @brief  Default data distribution algorithm.
 */
void default_dfs_f (char** dfs_matrix, size_t chunks, size_t workers, int replicas);

/**
 * @brief  Choose a random DataNode that owns a specific chunk.
 * @param  cid  The chunk ID.
 * @return The ID of the DataNode.
 */
size_t find_random_chunk_owner (size_t cid);

size_t find_random_intermediate_result_owner(size_t map_id, size_t reduce_id);

void update_intermediate_result_owner(size_t map_id, size_t owner);

/**
 * @brief  DataNode main function.
 *
 * Process that listens for data requests.
 */
int data_node (int argc, char *argv[]);

#endif /* !DFS_H */

// vim: set ts=8 sw=4:
