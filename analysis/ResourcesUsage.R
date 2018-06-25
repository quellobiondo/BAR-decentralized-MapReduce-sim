library(tidyverse)
library(ggrepel)

file_name <- "data/cpu_usage.csv"

cpu_data <- read_csv(file_name, col_names = TRUE, trim_ws = TRUE, col_types = 
                   cols(
                     Topology = col_character(),
                     NumberOfNodes = col_integer(),
                     Platform = col_character(),
                     Byzantine = col_integer(),
                     Power = col_double(),
                     Time = col_double(),
                     Updated = col_logical(),
                     Config = col_character()
                   )
) %>%   mutate(PowerTime=Time*Power) %>%
        mutate(CompleteTopology=paste(Topology, NumberOfNodes)) 
# %>% filter(Config=="sumallcopies" | Config=="sumlowblocktime")

by_configuration <- group_by(cpu_data, CompleteTopology, Platform, Config, Byzantine)
cpu_active <- filter(by_configuration, Power>0) %>% summarise(total_cpu_time = sum(Time, na.rm = TRUE), total_power = sum(PowerTime, na.rm = TRUE))
cpu_idle <- filter(by_configuration, Power==0) %>% summarize(total_idle_time = sum(Time, na.rm = TRUE))

ggplot(cpu_active, aes(Platform, total_cpu_time, fill = Byzantine, alpha  = Config)) +
  facet_wrap(~CompleteTopology) +
  geom_bar(stat = 'identity', position = 'dodge2') +
  labs(x="Platforms", y="Total CPU Time",title = "CPU time increases with % of byzantine nodes")
ggsave("resource_usage.jpg")


ggplot(cpu_idle, aes(Platform, total_idle_time, fill = Byzantine, alpha = Config)) +
  facet_wrap(~CompleteTopology) +
  geom_bar(stat = 'identity', position = 'dodge2') +
  labs(x="Platforms", y="Total CPU Idle Time",title = "CPU idle time variation with % of byzantine nodes")
ggsave("resource_unused.jpg")

# Other data analysis on Idle time of a specificPlatform
specific_file_name="data/Cluster-10/variable.MARS_M-0-sum.csv"
# Variable, chimint-16.lille.grid5000.fr, power, 0.000000, 648.167829, 648.167829, 23530999808.000000
specific_cpu_data <- read_csv(specific_file_name, col_names = FALSE, trim_ws = TRUE, col_types = 
                       cols(
                         X1 = col_character(),
                         X2 = col_character(),
                         X3 = col_character(),
                         X4 = col_double(),
                         X5 = col_double(),
                         X6 = col_double(),
                         X7 = col_double()
                       )
) %>% filter(X3=="power_used" & X7==0) 

ggplot(specific_cpu_data, aes(X6, color = X2)) +
  geom_histogram(binwidth = 1, na.rm = TRUE) +
  labs(title = "Distribution of waiting time durations during the experiment, with blocktime = 15s and 100% Byzantines", x="Duration Idle Time", color="Node")

ggsave("MARS_M-0Byz-full-replicas-idle-histogram.jpg")

ggplot(specific_cpu_data, aes(X4, X6, color = X2)) +
  geom_jitter(size=1, na.rm = TRUE) +
  #coord_cartesian(ylim = c(0, 20), expand = TRUE) + 
  labs(title = "Distribution of waiting time durations during the experiment, with blocktime = 15s and 100% byzantine", x="Start Event Time", y="Idle Time Duration", color="Node")
 
ggsave("MARS_M-0Byz-full-replicas-idle-points.jpg")