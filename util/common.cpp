#include "common.h"

void init_split_context(int num_recs, struct split_context *split_ctx) {
	for(int i = 0; i < num_recs; i++) {
		split_ctx[i].split_stone = -1;
		split_ctx[i].split_action = -1;
	}
}

void init_stone_context(int num_recs, struct stone_context *stone_ctx) {
	for(int i = 0; i < num_recs; i++) {
		stone_ctx[i].stone_id = -1;
		stone_ctx[i].stone_action = -1;
		stone_ctx[i].split_stone = -1;
		stone_ctx[i].split_action = -1;
	}
}

void init_func_context(int num_recs, struct func_context *func_ctx) {
	for(int i = 0; i < num_recs; i++) {
		func_ctx[i].func_name = NULL;
		func_ctx[i].in_format = NULL;
		func_ctx[i].out_format = NULL;
		func_ctx[i].func_order = -1;
		func_ctx[i].func_level = -1;
	}
}

void init_mon_context(int num_recs, struct mon_context *mon_ctx) {
	for(int i = 0; i < num_recs; i++) {
		mon_ctx[i].mon_key = NULL;
		mon_ctx[i].mon_attr = NULL;
		mon_ctx[i].mon_type = NULL;
		mon_ctx[i].mon_resource_type = NULL;
	}
}

// action_type = add/modify/delete actions = metric, group, collect, sample, and their combinations
void init_sample_context(int num_recs, struct sample_context *sample_ctx) {
	for(int i = 0; i < num_recs; i++) {
		sample_ctx[i].sample_rate_sec = -1;
		sample_ctx[i].sample_rate_usec = -1;
		sample_ctx[i].sample_window_width = -1;
		sample_ctx[i].sample_window_unit = -1;
	}
}

void init_stone_ids(int num_recs, int *stone_ids) {
	for(int i = 0; i < num_recs; i++) {
		stone_ids[i] = -1;
	}
}

void clean_split_context(int num_recs, struct split_context *split_ctx) {
	for(int i = 0; i < num_recs; i++) {
		split_ctx[i].split_stone = -1;
		split_ctx[i].split_action = -1;
	}
}

void clean_stone_context(int num_recs, struct stone_context *stone_ctx) {
	for(int i = 0; i < num_recs; i++) {
		stone_ctx[i].stone_id = -1;
		stone_ctx[i].stone_action = -1;
		stone_ctx[i].split_stone = -1;
		stone_ctx[i].split_action = -1;
	}
}

void clean_func_context(int num_recs, struct func_context *func_ctx) {
	for(int i = 0; i < num_recs; i++) {
		func_ctx[i].func_name = NULL;
		func_ctx[i].in_format = NULL;
		func_ctx[i].out_format = NULL;
		func_ctx[i].func_order = -1;
		func_ctx[i].func_level = -1;
	}
}

void clean_mon_context(int num_recs, struct mon_context *mon_ctx) {
	for(int i = 0; i < num_recs; i++) {
		mon_ctx[i].mon_key = NULL;
		mon_ctx[i].mon_attr = NULL;
		mon_ctx[i].mon_type = NULL;
		mon_ctx[i].mon_resource_type = NULL;
	}
}

void clean_sample_context(int num_recs, struct sample_context *sample_ctx) {
	for(int i = 0; i < num_recs; i++) {
		sample_ctx[i].sample_rate_sec = -1;
		sample_ctx[i].sample_rate_usec = -1;
		sample_ctx[i].sample_window_width = -1;
		sample_ctx[i].sample_window_unit = -1;
	}
}

void clean_deploy_nodes_context(int num_recs, struct deploy_nodes_context *deploy_nodes_ctx) {
	for(int i = 0; i < num_recs; i++) {
		if(deploy_nodes_ctx[i].num_deploy_nodes != 0) {
			for(int j = 0; j < deploy_nodes_ctx[i].num_deploy_nodes; j++) {
				deploy_nodes_ctx[i].deploy_nodes_contact_list[j] = NULL;
			}
		}
	}
}

void clean_stone_ids(int num_recs, int *stone_ids) {
	for(int i = 0; i < num_recs; i++) {
		stone_ids[i] = -1;
	}
}
