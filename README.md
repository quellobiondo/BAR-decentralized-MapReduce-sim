# Simulation for the decentralized Map-Reduce in BAR environments 

Structure
- Platform -> simulation platform
- Experiments -> Experiment definitions 

### Execute the experiment
To execute:
- Install simgrid (preferred 3.11)
- Make simulation platform
- Make Experiment definitions
- Execute the experiment

The easy way with [Docker](https://www.docker.com/):
```
docker build -t simgrid:3.11.1 -f Tools/SimGrid.Dockerfile .
docker build -t experiment .
docker run -v "$PWD/analysis/traces:/home/experiment/experiments/traces" experiment bin/hello.bin
```

### Analyze data
To analyze the data it is used [R](https://www.r-project.org/)

Dependencies
`apt install r-base r-cran-ggplot2 r-cran-dplyr pajeng`

Each experiment run produce a *tracefile.trace*, this are in the [Pajé format](), we need to convert it in csv. 

Preparing the traces manually
```
pj_dump tracefile.trace | grep State > tracefile.state.csv
pj_dump tracefile.trace | grep Link > tracefile.link.csv
pj_dump tracefile.trace | grep Container > tracefile.container.csv
```
