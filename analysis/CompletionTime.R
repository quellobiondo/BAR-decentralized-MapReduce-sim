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
                     Config = col_character(),
                     Seed = col_integer()
                   )
) %>% mutate(CompleteTopology=paste(Topology, NumberOfNodes), Id=paste(Platform, Config))

# data <- filter(data, Config=="sumallcopies"|Config=="sumlowblocktime")
# data <- filter(data, NumberOfNodes==10, Platform=="MARS_M" | Platform=="MARS_M_PESSIMISTIC" )
data10 <- filter(data, Number)

# Aggregate the information to get a list of values for each combination of Config-Platform-PercentageOfByz
agg_data10 <- data %>% group_by(CompleteTopology, Platform, Config, Byzantine, Id) %>% 
  summarize(TotalDurationMean=mean(TotalDuration), TotalDurationMax=max(TotalDuration), TotalDurationMin=min(TotalDuration))

ggplot(agg_data, aes(Byzantine, TotalDurationMean, color = Platform)) +
  facet_wrap(~CompleteTopology) +
  geom_point(aes(shape = Config), size=3, na.rm = TRUE) +
  geom_line(aes(linetype = Config), na.rm = TRUE) +
  geom_ribbon(aes(ymin = agg_data$TotalDurationMin, ymax=agg_data$TotalDurationMax, group = Id), linetype=2, alpha=0.1) +
  coord_cartesian(xlim = c(0, 40), ylim = c(0, 600), expand = TRUE) + 
  labs(title = "Completion time increases with % of byzantine nodes and block creation time", x="% Byzantine nodes", y="Job Duration")
ggsave("completion_time_10_with_interval_complete.jpg")

ggplot(agg_data, aes(Byzantine, TotalDurationMean, color = Platform)) +
  facet_wrap(~CompleteTopology) +
  geom_point(aes(shape = Config), size=3, na.rm = TRUE) +
  geom_line(aes(linetype = Config), na.rm = TRUE, se = FALSE) +
  coord_cartesian(xlim = c(0, 40), expand = TRUE) + 
  labs(title = "Completion time increases with % of byzantine nodes and block creation time", x="% Byzantine nodes", y="Job Duration")
ggsave("completion_time_10_no_interval_complete.jpg")

just_one_blocktime <- filter(agg_data, Config=="sum"|Config=="sum_no_real_byz")
ggplot(just_one_blocktime, aes(Byzantine, TotalDurationMean, color = Platform)) +
  facet_wrap(~CompleteTopology) +
  geom_point(aes(shape = Config), size=3, na.rm = TRUE) +
  geom_line(aes(linetype = Config), na.rm = TRUE) +
  geom_ribbon(aes(ymin = just_one_blocktime$TotalDurationMin, ymax=just_one_blocktime$TotalDurationMax, group = Id), linetype=2, alpha=0.1) +
  coord_cartesian(xlim = c(0, 40), ylim = c(0, 600), expand = TRUE) + 
  labs(title = "Completion time increases with % of byzantine nodes and block creation time", x="% Byzantine nodes", y="Job Duration")
ggsave("completion_time_10_with_interval_only_sum.jpg")

just_hadoop_sum <- filter(agg_data, (Config=="sum"|Config=="sum_no_real_byz")&(Platform=="HADOOP" | Platform=="HADOOP_BFT"))
just_lowest_blocktime <- filter(agg_data, (Config=="sumzeroblocktime"|Config=="sumzeroblocktime_no_real_byz")&(Platform!="HADOOP" & Platform!="HADOOP_BFT"))
hadoop_sum_with_blockchain_lowest_time <- rbind(just_hadoop_sum, just_lowest_blocktime)
ggplot(hadoop_sum_with_blockchain_lowest_time, aes(Byzantine, TotalDurationMean, color = Platform)) +
  facet_wrap(~CompleteTopology) +
  geom_point(aes(shape = Config), size=3, na.rm = TRUE) +
  geom_line(aes(linetype = Config), na.rm = TRUE) +
  geom_ribbon(aes(ymin = hadoop_sum_with_blockchain_lowest_time$TotalDurationMin, ymax=hadoop_sum_with_blockchain_lowest_time$TotalDurationMax, group = Id), linetype=2, alpha=0.1) +
  coord_cartesian(xlim = c(0, 40), expand = TRUE) + 
  labs(title = "Completion time increases with % of byzantine nodes and block creation time", x="% Byzantine nodes", y="Job Duration")
ggsave("completion_time_10_with_interval_hadoop_with_sum_blockchain_with_lowest_blocktime.jpg")

selection_agg_data <- filter(agg_data, Byzantine <= 30)
ggplot(selection_agg_data, aes(Byzantine, TotalDurationMean, color = Platform)) +
  facet_wrap(~CompleteTopology) +
  geom_point(aes(shape = Config), size=3, na.rm = TRUE) +
  geom_line(aes(linetype = Config), na.rm = TRUE, se = FALSE) +
  coord_cartesian(xlim = c(0, 30), ylim = c(0, 310), expand = TRUE) + 
  labs(title = "Completion time increase with % byzantine nodes and block creation time", x="% Byzantine nodes", y="Job Duration")
ggsave("completion_time_10_detail_no_interval_complete.jpg")

data100 <- filter(data, NumberOfNodes==100, TotalDuration > 0)

# Aggregate the information to get a list of values for each combination of Config-Platform-PercentageOfByz
agg_data100 <- data %>% group_by(CompleteTopology, Platform, Config, Byzantine, Id) %>% 
  summarize(TotalDurationMean=mean(TotalDuration), TotalDurationMax=max(TotalDuration), TotalDurationMin=min(TotalDuration))

ggplot(agg_data100, aes(Byzantine, TotalDurationMean, color = Platform)) +
  facet_wrap(~CompleteTopology) +
  geom_point(aes(shape = Config), size=3, na.rm = TRUE) +
  geom_line(aes(linetype = Config), na.rm = TRUE) +
  geom_ribbon(aes(ymin = agg_data100$TotalDurationMin, ymax=agg_data100$TotalDurationMax, group = Id), linetype=2, alpha=0.1) +
  coord_cartesian(xlim = c(0, 40), ylim = c(0, 600), expand = TRUE) + 
  labs(title = "Completion time increases with % of byzantine nodes and block creation time", x="% Byzantine nodes", y="Job Duration")
ggsave("completion_time_100_with_interval_complete.jpg")

ggplot(agg_data100, aes(Byzantine, TotalDurationMean, color = Platform)) +
  facet_wrap(~CompleteTopology) +
  geom_point(aes(shape = Config), size=3, na.rm = TRUE) +
  geom_line(aes(linetype = Config), na.rm = TRUE, se = FALSE) +
  coord_cartesian(xlim = c(0, 40), expand = TRUE) + 
  labs(title = "Completion time increases with % of byzantine nodes and block creation time", x="% Byzantine nodes", y="Job Duration")
ggsave("completion_time_100_no_interval_complete.jpg")

just_one_blocktime <- filter(agg_data100, Config=="sum"|Config=="sum_no_real_byz")
ggplot(just_one_blocktime, aes(Byzantine, TotalDurationMean, color = Platform)) +
  facet_wrap(~CompleteTopology) +
  geom_point(aes(shape = Config), size=3, na.rm = TRUE) +
  geom_line(aes(linetype = Config), na.rm = TRUE) +
  geom_ribbon(aes(ymin = just_one_blocktime$TotalDurationMin, ymax=just_one_blocktime$TotalDurationMax, group = Id), linetype=2, alpha=0.1) +
  coord_cartesian(xlim = c(0, 40), ylim = c(0, 600), expand = TRUE) + 
  labs(title = "Completion time increases with % of byzantine nodes and block creation time", x="% Byzantine nodes", y="Job Duration")
ggsave("completion_time_100_with_interval_only_sum.jpg")

just_hadoop_sum <- filter(agg_data100, (Config=="sum"|Config=="sum_no_real_byz")&(Platform=="HADOOP" | Platform=="HADOOP_BFT"))
just_lowest_blocktime <- filter(agg_data100, (Config=="sumzeroblocktime"|Config=="sumzeroblocktime_no_real_byz")&(Platform!="HADOOP" & Platform!="HADOOP_BFT"))
hadoop_sum_with_blockchain_lowest_time <- rbind(just_hadoop_sum, just_lowest_blocktime)
ggplot(hadoop_sum_with_blockchain_lowest_time, aes(Byzantine, TotalDurationMean, color = Platform)) +
  facet_wrap(~CompleteTopology) +
  geom_point(aes(shape = Config), size=3, na.rm = TRUE) +
  geom_line(aes(linetype = Config), na.rm = TRUE) +
  geom_ribbon(aes(ymin = hadoop_sum_with_blockchain_lowest_time$TotalDurationMin, ymax=hadoop_sum_with_blockchain_lowest_time$TotalDurationMax, group = Id), linetype=2, alpha=0.1) +
  coord_cartesian(xlim = c(0, 40), expand = TRUE) + 
  labs(title = "Completion time increases with % of byzantine nodes and block creation time", x="% Byzantine nodes", y="Job Duration")
ggsave("completion_time_100_with_interval_hadoop_with_sum_blockchain_with_lowest_blocktime.jpg")

selection_agg_data <- filter(agg_data100, Byzantine <= 30)
ggplot(selection_agg_data, aes(Byzantine, TotalDurationMean, color = Platform)) +
  facet_wrap(~CompleteTopology) +
  geom_point(aes(shape = Config), size=3, na.rm = TRUE) +
  geom_line(aes(linetype = Config), na.rm = TRUE, se = FALSE) +
  coord_cartesian(xlim = c(0, 30), ylim = c(0, 310), expand = TRUE) + 
  labs(title = "Completion time increase with % byzantine nodes and block creation time", x="% Byzantine nodes", y="Job Duration")
ggsave("completion_time_100_detail_30_no_interval_complete.jpg")

selection_agg_data <- filter(agg_data100, Byzantine <= 10)
ggplot(selection_agg_data, aes(Byzantine, TotalDurationMean, color = Platform)) +
  facet_wrap(~CompleteTopology) +
  geom_point(aes(shape = Config), size=3, na.rm = TRUE) +
  geom_line(aes(linetype = Config), na.rm = TRUE, se = FALSE) +
  coord_cartesian(xlim = c(0, 10), ylim = c(0, 310), expand = TRUE) + 
  labs(title = "Completion time increase with % byzantine nodes and block creation time", x="% Byzantine nodes", y="Job Duration")
ggsave("completion_time_100_detail_10_no_interval_complete.jpg")