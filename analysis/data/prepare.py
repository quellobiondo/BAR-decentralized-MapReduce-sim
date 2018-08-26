#! /usr/bin/python3

import os
import subprocess
import re 
import io
import threading
import queue

def get_topologies():
    return [ { 'type': "Cluster", 'number_of_nodes': 10 }, { 'type': "Cluster", 'number_of_nodes': 100 }, { 'type': "P2P", 'number_of_nodes': 10 } ]

def get_traces(dirname):
    return [trace for trace in os.listdir(dirname) if trace.endswith(".trace")]
    

def create_result_table(filename, header):
    if os.path.exists(filename):
        os.remove(filename)
    table = open(filename, "x")
    table.write(header+"\n")
    return table

def process_trace(paje_tracefile):
    result = subprocess.check_output(["pj_dump", "--ignore-incomplete-links", paje_tracefile], universal_newlines=True)
    state = []
    variable = []
    container = []
    link = []

    for line in io.StringIO(result):
        if "State" in line:
            state.append(line)
        elif "Variable" in line:
            variable.append(line)
        elif "Link" in line:
            link.append(line)
        elif "Container" in line:
            container.append(line)

    return state, variable, container, link

def process_computing_resources(computing_logs):
    print("... computing CPU consumption")
    power_time = []
    for log in computing_logs:
        if "power_used" in log:
            partitions = log.split(",")
            power_time.append({"power": float(partitions[6]), "time": float(partitions[5]) })
    return power_time


def process_computing_time(event_logs):
    print("... computing Job duration")
    map_start = 0.0
    map_end = 0.0
    reduce_start = 0.0
    reduce_end = 0.0

    map_end_matcher = re.compile(".*MAP,.*END")
    reduce_start_matcher = re.compile(".*REDUCE,.*START")
    reduce_end_matcher = re.compile(".*REDUCE,.*END")

    map_end_found, reduce_start_found, reduce_end_found = False, False, False 

    print(event_logs[0])
    for log in event_logs:
        if((not map_end_found) and map_end_matcher.match(log)):
            map_end = float(log.split(",")[3]) 
            map_end_found = True

        if((not reduce_start_found) and reduce_start_matcher.match(log)):
            reduce_start = float(log.split(",")[3])
            reduce_start_found = True
    
        if((not reduce_end_found) and reduce_end_matcher.match(log)):
            reduce_end = float(log.split(",")[3])
            reduce_end_found = True
        
        if(reduce_end_found and reduce_start_found and map_end_found):
            break

    map_duration = map_end - map_start
    reduce_duration = reduce_end - reduce_start
    total_duration = reduce_end - map_start

    return map_duration, reduce_duration, total_duration


class WorkerThread(threading.Thread):
    def __init__(self, group=None, target=None, name=None, args=(), kwargs=None):
        super(WorkerThread, self).__init__(group=group, target=target, name=name)
        
        
        topology_type = args[0]
        topology_number_of_nodes = args[1]
        resource_queue = args[2]
        time_queue = args[3]
        
        print("Processing %s-%d" % (topology_type, topology_number_of_nodes))
        

        self.topology_type = topology_type
        self.topology_number_of_nodes = topology_number_of_nodes
        self.resource_queue=resource_queue
        self.time_queue = time_queue

    def run(self):
        topology_dir_name = "%s-%d" % (self.topology_type, self.topology_number_of_nodes)
        
        for trace in get_traces(topology_dir_name):
            platform, byzantines, configuration = os.path.splitext(trace)[0].split("-")
            byzantines = int(byzantines)

            print("Processing trace for platform: %s, byz: %d, config: %s" %(platform, byzantines, configuration))
            state, variable, container, link = process_trace("%s/%s" % (topology_dir_name, trace))
            
            
            map_duration, reduce_duration, total_duration = process_computing_time(state)

            self.time_queue.put({ 
                'type': self.topology_type,
                'number_of_nodes': self.topology_number_of_nodes,
                'platform': platform, 
                'byzantines': byzantines, 
                'map_duration': map_duration, 
                'reduce_duration': reduce_duration, 
                'total_duration': total_duration, 
                'configuration': configuration
            })
            
            power_times = process_computing_resources(variable)

            for power_time in power_times:
                self.resource_queue.put({
                        'type': self.topology_type,
                        'number_of_nodes': self.topology_number_of_nodes,
                        'platform': platform, 
                        'byzantines': byzantines, 
                        'power': power_time['power'], 
                        'time': power_time['time'], 
                        'configuration': configuration
                    })

def main():
    computing_time_file="completion_time.csv"
    computing_resources_file="cpu_usage.csv"

    time_table = create_result_table(computing_time_file, "Topology, NumberOfNodes, Platform, Byzantine, MapDuration, ReduceDuration, TotalDuration, Config")
    resources_table = create_result_table(computing_resources_file, "Topology, NumberOfNodes, Platform, Byzantine, Power, Time, Config")

    time_queue = queue.Queue()
    resource_queue = queue.Queue()

    threads = []
    
    for topology in get_topologies():
        thread = WorkerThread(args=(topology['type'], topology['number_of_nodes'], resource_queue, time_queue))
        threads.append(thread)
        thread.start()
        # thread.join()

    print("[Master] Waiting for %d threads to terminate" % len(threads))

    for thread in threads:
        thread.join()

    while not time_queue.empty():
        time_result = time_queue.get()
        time_table.write("%s, %d, %s, %d, %f, %f, %f, %s\n" % (time_result['type'], time_result['number_of_nodes'], time_result['platform'], time_result['byzantines'], 
            time_result['map_duration'], time_result['reduce_duration'], time_result['total_duration'], time_result['configuration']))    

    while not resource_queue.empty():
        resource_result = resource_queue.get()
        resources_table.write("%s, %d, %s, %d, %f, %f, %s\n" % (resource_result['type'], resource_result['number_of_nodes'], resource_result['platform'], resource_result['byzantines'], 
            resource_result['power'], resource_result['time'], resource_result['configuration'])) 

    time_table.close()
    resources_table.close()

if __name__ == "__main__":
    main()