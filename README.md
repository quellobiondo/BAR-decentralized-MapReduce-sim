# Simulation for the decentralized Map-Reduce in BAR environments 

Structure
- Platform -> simulation platform
- Experiments -> Experiment definitions 

To execute:
- Install simgrid (preferred 3.11)
- Make simulation platform
- Make Experiment definitions
- Execute the experiment

The easy way with [Docker]():
```
docker build -t simgrid:3.11.1 -f Tools/SimGrid.Dockerfile .
docker build -t experiment .
docker run -v "$PWD/analysis/traces:/home/experiment/experiments/traces" experiment bin/hello.bin
```

- Analyze data

