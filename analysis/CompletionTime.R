library(tidyverse)
library(ggrepel)

file_name <- "data/completion_time.csv"

data <- read_csv(file_name, col_names = TRUE, trim_ws = TRUE, col_types = 
                   cols(
                     Topology = col_character(),
                     NumberOfNodes = col_integer(),
                     Platform = col_character(),
                     Byzantine = col_integer(),
                     MapDuration = col_double(),
                     ReduceDuration = col_double(),
                     TotalDuration = col_double(),
                     Updated = col_logical(),
                     Config = col_character()
                   )
)

data <- mutate(data, CompleteTopology=paste(Topology, NumberOfNodes))

ggplot(data, aes(Byzantine, TotalDuration, color = Platform)) +
  facet_wrap(~CompleteTopology) +
  geom_point(aes(shape = Config), size=3) +
  geom_smooth() +
  labs(title = "Completion time increase with % byzantine nodes")

ggsave("completion_time.jpg")