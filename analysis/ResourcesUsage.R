library(tidyverse)
library(ggrepel)

file_name <- "data/cpu_usage.csv"

cpu_data <- read_csv(file_name, col_names = TRUE, trim_ws = TRUE, col_types = 
                   cols(
                     Topology = col_character(),
                     NumberOfNodes = col_integer(),
                     Platform = col_character(),
                     Byzantine = col_character(),
                     Power = col_double(),
                     Time = col_double(),
                     Updated = col_logical(),
                     Config = col_character()
                   )
)

cpu_data <- mutate(cpu_data, PowerTime=Time*Power)
cpu_data <- mutate(cpu_data, CompleteTopology=paste(Topology, NumberOfNodes))

by_configuration <- group_by(cpu_data, CompleteTopology, Platform, Config, Byzantine)
value <- summarise(by_configuration, total_cpu_time = sum(Time, na.rm = TRUE), total_power = sum(PowerTime, na.rm = TRUE))

ggplot(value, aes(Platform, total_cpu_time, fill = Byzantine)) +
  facet_wrap(~CompleteTopology) +
  geom_bar(stat = 'identity', position = 'dodge2') +
  labs(title = "CPU consumption increases with Byzantine nodes")

ggsave("resource_usage.jpg")
