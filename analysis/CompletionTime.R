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
) %>% mutate(CompleteTopology=paste(Topology, NumberOfNodes))

# data <- filter(data, Config=="sumallcopies"|Config=="sumlowblocktime")

ggplot(data, aes(Byzantine, TotalDuration, color = Platform)) +
  facet_wrap(~CompleteTopology) +
  geom_point(aes(shape = Config), size=3, na.rm = TRUE) +
  geom_smooth(aes(linetype = Config), na.rm = TRUE, se = FALSE) +
  labs(title = "Completion time increases with % of byzantine nodes and block creation time", x="% Byzantine nodes", y="Job Duration")
ggsave("completion_time.jpg")

ggplot(data, aes(Byzantine, TotalDuration, color = Platform)) +
  facet_wrap(~CompleteTopology) +
  geom_point(aes(shape = Config), size=3, na.rm = TRUE) +
  geom_smooth(aes(linetype = Config), na.rm = TRUE, se = FALSE) +
  coord_cartesian(xlim = c(0, 30), ylim = c(0, 400), expand = TRUE) + 
  labs(title = "Completion time increase with % byzantine nodes and block creation time", x="% Byzantine nodes", y="Job Duration")
ggsave("completion_time_detail.jpg")