#ifndef FORMAT_MANAGER_H
#define FORMAT_MANAGER_H

#include "../util/common.h"

typedef struct resrc_rec {
	float key_srng;
	float key_erng;
	int num_recs;
	float *rec_val;
} resrc_rec, *resrc_rec_ptr;

typedef struct base_resrc_rec {
	float cpu_key_srng;
	float cpu_key_erng;
	int cpu_num_recs;
	float *cpu_rec_val;
	float mem_key_srng;
	float mem_key_erng;
	int mem_num_recs;
	float *mem_rec_val;
} base_resrc_rec, *base_resrc_rec_ptr;

typedef struct range_rec {
	char *attr_name;
	float key_srng;
	float key_erng;
	int num_recs;
	float *rec_val;
} range_rec, *range_rec_ptr;

typedef struct range_recs_list {
	int num_range_recs;
	struct range_rec *range_recs;
} range_recs_list, *range_recs_list_ptr;
	
typedef struct stats_rec {
	char *attr_name;
	float key_srng;
	float key_erng;
	int rec_int_cnt;
	float rec_float_cnt;
} stats_rec, *stats_rec_ptr;

typedef struct stats_recs_list {
	int num_stats_recs;
	struct stats_rec *stats_recs;
} stats_recs_list, *stats_recs_list_ptr;

typedef struct actuate_rec {
	int is_control;
	int query_id;
	char *actuate_graph;
	char *node_contact_list;
} actuate_rec, *actuate_rec_ptr;

typedef struct sensor_rec {
	double timestamp;
	char *attr_name;
	int rec_val_int;
	float rec_val_float;
	unsigned long rec_val;
} sensor_rec, *sensor_rec_ptr;

typedef struct sensor_recs {
	int sensor_recs_num;
	struct sensor_rec *sensor_recs;
} sensor_recs, *sensor_recs_ptr;

typedef struct count_sample_rec {
	char *metric_type;
	int sample_count;
} count_sample_rec, *count_sample_rec_ptr;

typedef struct count_sample_recs {
	int csample_recs_num;
	struct count_sample_rec *count_sample_recs;
} count_sample_recs, *count_sample_recs_ptr;

typedef struct sample_window_recs {
	char *swindow_recs_type;
	int swindow_recs_num;
	int *swindow_recs;
} sample_window_recs, *sample_window_recs_ptr;

typedef struct recs_entropy {
	char *recs_type;
	int web_recs_num;
	float web_recs_entropy;
	int serv_recs_num;
	float serv_recs_entropy;
	int db_recs_num;
	float db_recs_entropy;
} recs_entropy, *recs_entropy_ptr;

/*
typedef struct global_entropy {
	int recs_num;
	struct recs_entropy *local_entropy;
	float web_recs_entropy;
	float serv_recs_entropy;
	float db_recs_entropy;
	float global_total_entropy;
} global_entropy, *global_entropy_ptr;*/

typedef struct global_entropy {
	int global_recs_num;
	float global_entropy;
	float web_recs_entropy;
	float serv_recs_entropy;
	float db_recs_entropy;
} global_entropy, *global_entropy_ptr;

typedef struct recs {
	char *attr_name;
	int num_recs;
	float *rec_val;
} recs, *recs_ptr;

typedef struct recs_range {
	char *attr_name;
	float key_srng;
	float key_erng;
	int num_recs;
	float *rec_val;
} recs_range, *recs_range_ptr;

typedef struct recs_range_level {
	char *attr_name;
	float key_srng;
	float key_erng;
	int range_level;
	int num_recs;
	float *rec_val;
} recs_range_level, *recs_range_level_ptr;

typedef struct recs_range_level_list {
	int num_range_recs;
	struct recs_range_level *recs_range;
} recs_range_level_list, *recs_range_level_list_ptr;

typedef struct rec_stats {
	char *attr_name;
	float key_srng;
	float key_erng;
	int rec_int_cnt;
	float rec_float_cnt;
} rec_stats, *rec_stats_ptr;

typedef struct recs_stats_range_list {
	int num_stats_recs;
	struct rec_stats *recs_stats;
} recs_stats_range_list, *recs_stats_range_list_ptr;

typedef struct rec_stats_threshold {
	char *attr_name;
	float key_srng;
	float key_erng;
	int threshold_val;
	int rec_int_cnt;
	float rec_float_cnt;
} rec_stats_threshold, *rec_stats_threshold_ptr;

typedef struct recs_stats_threshold_range_list {
	int num_stats_recs;
	struct rec_stats_threshold *recs_stats_threshold;
} recs_stats_threshold_range_list, *recs_stats_threshold_range_list_ptr;

typedef struct recs_entropy_stats {
	char *attr_name;
	int total_recs;
	double recs_entropy;
} recs_entropy_stats, *recs_entropy_stats_ptr;

typedef struct recs_entropy_stats_threshold {
	char *attr_name;
	int total_recs;
	double recs_entropy;
	double recs_entropy_threshold;
} recs_entropy_stats_threshold, *recs_entropy_stats_threshold_ptr;

#endif
