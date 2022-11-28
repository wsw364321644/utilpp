/**
 *   pkt_body.h
 */

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

    void pkt_free(void* data);
	void pkt_free_2d(char** data, uint32_t size);
// game side
    int pkt_parse_ack(const char* data, size_t size, int32_t* error, int32_t* request);
    char* pkt_build_ack(int32_t error, int32_t request, size_t* size);

    // game info
    int pkt_parse_game_info(const char* data, size_t size, uint32_t* version, char** dll_path, char** exe_name,char**sk_appid);
    char* pkt_build_game_info(uint32_t version, const char* dll_path, const char* exe_name,const char* sonkwo_appid, size_t* size);
    int pkt_parse_game_info_ack(const char* data, size_t size, uint32_t* game, uint32_t* user ,char** info,char**dlc);
	char* pkt_build_game_info_ack(uint32_t game_id, uint32_t user_id, const char* info, const char* dlcs, size_t* size);

    // stat and achievement
    struct pkt_achievement
    {
        char name[SK_ACHIEVEMENT_MAX];
        uint64_t unlock_time;
        int32_t value;
    };

    int pkt_parse_achievement(const char* data, size_t size, uint32_t* game, uint32_t* user, uint32_t* op, struct pkt_achievement** aches, uint32_t* ach_size);
    char* pkt_build_achievement(uint32_t game, uint32_t user, uint32_t op, struct pkt_achievement* aches, uint32_t ach_size, size_t* size);
    int pkt_parse_achievement_ack(const char* data, size_t size, uint32_t* op, uint32_t* result, struct pkt_achievement** aches, uint32_t* ach_size);
    char* pkt_build_achievement_ack(uint32_t op, uint32_t result, struct pkt_achievement* aches, uint32_t ach_size, size_t* size);


    // iap
    struct pkt_item
    {
        char name[SK_ITEM_MAX];
        uint32_t price;
        uint32_t count;
        uint32_t order;
    };

    int pkt_parse_item_query(const char* data, size_t size, char** name);
    char* pkt_build_item_query(const char* name, size_t* size);
    int pkt_parse_item_query_ack(const char* data, size_t size, uint32_t* result, struct pkt_item** items, uint32_t* item_count);
    char* pkt_build_item_query_ack(uint32_t result, struct pkt_item* items, uint32_t item_count, size_t* size);

    int pkt_parse_item_purchase(const char* data, size_t size, char** name, uint32_t* count);
    char* pkt_build_item_purchase(const char* name, uint32_t count, size_t* size);
    int pkt_parse_item_purchase_ack(const char* data, size_t size, uint32_t* result, uint32_t* order, char** name, uint32_t* count, uint32_t* price, uint32_t* amount);
    char* pkt_build_item_purchase_ack(uint32_t result, uint32_t order, const char* name, uint32_t count, uint32_t price, uint32_t amount, size_t* size);

    int pkt_parse_order_pay(const char* data, size_t size, uint32_t* order);
    char* pkt_build_order_pay(uint32_t order, size_t* size);
    int pkt_parse_order_pay_ack(const char* data, size_t size, uint32_t* result, uint32_t* order);
    char* pkt_build_order_pay_ack(uint32_t result, uint32_t order, size_t* size);

    int pkt_parse_order_checkout(const char* data, size_t size, uint32_t* order, char** token);
    char* pkt_build_order_checkout(uint32_t order, const char* token, size_t* size);
    int pkt_parse_order_checkout_ack(const char* data, size_t size, uint32_t* result, uint32_t* order);
    char* pkt_build_order_checkout_ack(uint32_t result, uint32_t order, size_t* size);

	// mm
	struct pkt_kv_data_x
	{
		char*		key;
		uint32_t	type;
		char*		value;
		uint32_t	ivalue;
	};
	struct kv_list_x
	{
		uint32_t kv_count;
		struct pkt_kv_data_x* kv_data;
	};
	struct pkt_user_data
	{
		uint32_t bAI;
		char* user;
		char* name;
		char* avatar;
		uint32_t state;
		uint32_t team_type;
		uint32_t chair;
		uint32_t    kv_count;
		struct pkt_kv_data_x* kv_list;
	};
	struct region_data
	{
		char* name;
		int32_t ttl;
	};
	struct region_data_list
	{
		uint32_t count;
		struct region_data* r_data;
	};
	int pkt_parse_mm_req(const char* data, size_t size, uint32_t* op, uint32_t* sub_op, struct kv_list_x* kv_para_list, struct kv_list_x* kv_data_list);
	char* pkt_build_mm_req(uint32_t op, uint32_t sub_op, struct kv_list_x* kv_para_list, struct kv_list_x* kv_data_list, size_t* size);

	int pkt_parse_mm_resp(const char* data, size_t size, uint32_t* op, uint32_t* sub_op, struct kv_list_x* kv_para_list, struct kv_list_x* kv_data_list, struct region_data_list* region_list, struct pkt_user_data** user_list, uint32_t* user_count);
	char* pkt_build_mm_resp(uint32_t op, uint32_t sub_op, struct kv_list_x* kv_para_list, struct kv_list_x* kv_data_list, struct region_data_list* region_list, struct pkt_user_data* user_list, uint32_t user_count, size_t* size);
	
// server side
    int pkt_parse_login(const char* data, size_t size, uint32_t* user, char** token);
    char* pkt_build_login(uint32_t user, const char* token, size_t* size);
    int pkt_parse_login_ack(const char* data, size_t size, uint32_t* user, uint32_t* result, char** token);
    char* pkt_build_login_ack(uint32_t user, uint32_t result, const char* token, size_t* size);
	
	int pkt_parse_login_access(const char* data, size_t size, char** user, uint32_t* game, char** token);
	char* pkt_build_login_access(const char* user, uint32_t game, const char* token, size_t* size);
	int pkt_parse_login_access_ack(const char* data, size_t size, char** user, uint32_t* game, uint32_t* result, char** token);
	char* pkt_build_login_access_ack(const char* user, uint32_t game, uint32_t result, const char* token, size_t* size);

	//int pkt_parse_check_game(const char* data, size_t size, char** user, char** user_name, char** user_avatar, uint32_t* game, uint32_t* product, char** token, uint32_t* reserved1, char** data1);
	//char* pkt_build_check_game(const char* user, const char* user_name, const char* user_avatar, uint32_t game, uint32_t product, const char* token, uint32_t reserved1, const char* data1, size_t* size);
	//int pkt_parse_check_game_ack(const char* data, size_t size, char** user, uint32_t* game, uint32_t* product, uint32_t* result, char** token);
	//char* pkt_build_check_game_ack(const char* user, uint32_t game, uint32_t product, uint32_t result, const char* token, size_t* size);

    int pkt_parse_fetch_achievements(const char* data, size_t size, uint32_t* game, uint32_t* user);
    char* pkt_build_fetch_achievements(uint32_t game, uint32_t user, size_t* size);
    int pkt_parse_fetch_achievements_ack(const char* data, size_t size, uint32_t* game, uint32_t* user, struct pkt_achievement** achievements, size_t* ach_num);
    char* pkt_build_fetch_achievements_ack(uint32_t game, uint32_t user, struct pkt_achievement* achievements, size_t ach_num, size_t* size);

    int pkt_parse_unlock_achievement(const char* data, size_t size, uint32_t* game, uint32_t* user, char** achievement);
    char* pkt_build_unlock_achievement(uint32_t game, uint32_t user, const char* achievement, size_t* size);
    int pkt_parse_unlock_achievement_ack(const char* data, size_t size, uint32_t* game, uint32_t* user, uint32_t* result, char** achievement, uint64_t* unlock);
    char* pkt_build_unlock_achievement_ack(uint32_t game, uint32_t user, uint32_t result, const char* achievement, uint64_t unlock, size_t* size);

    int pkt_parse_sync_file(const char* data, size_t size, uint32_t* game, uint32_t* user, uint32_t* op, char** file_name, char** checksum);
    char* pkt_build_sync_file(uint32_t game, uint32_t user, uint32_t op, const char* file_name, const char* checksum, size_t* size);
    int pkt_parse_sync_file_ack(const char* data, size_t size, uint32_t* game, uint32_t* user, uint32_t* op, uint32_t* result, char** file_name, char** file_url, char** token, 
        uint64_t* timestamp, char** x_file_name, char** x_sonkwo_token);
    char* pkt_build_sync_file_ack(uint32_t game, uint32_t user, uint32_t op, uint32_t result, const char* file_name, const char* file_url, const char* token, size_t* size);

    struct pkt_file_info
    {
        uint32_t id;
        char* file_name;
        uint64_t file_size;
        char* file_key;
        char* file_hash;
        char* file_url;
		uint64_t puttime;
    };

    int pkt_parse_fetch_save_files(const char* data, size_t size, uint32_t* game, uint32_t* user);
    char* pkt_build_fetch_save_files(uint32_t game, uint32_t user, size_t* size);
    int pkt_parse_fetch_save_files_ack(const char* data, size_t size, uint32_t* game, uint32_t* user, struct pkt_file_info** files, size_t* file_count);
    char* pkt_build_fetch_save_files_ack(uint32_t game, uint32_t user, struct pkt_file_info* files, size_t file_count, size_t* size);

    void pkt_free_file_info(struct pkt_file_info* files, size_t count);


    int pkt_parse_fetch_save_path(const char* data, size_t size, uint32_t* game, uint32_t* user);
    char* pkt_build_fetch_save_path(uint32_t game, uint32_t user, size_t* size);
    int pkt_parse_fetch_save_path_ack(const char* data, size_t size, uint32_t* game, uint32_t* result, char** root, char** directory, char** pattern);
    char* pkt_build_fetch_save_path_ack(uint32_t game, uint32_t result, const char* root, const char* directory, const char* pattern, size_t* size);


    int pkt_parse_set_stat_int(const char* data, size_t size, uint32_t* game, uint32_t* user, char** stat, int32_t* value);
    char* pkt_build_set_stat_int(uint32_t game, uint32_t user, const char* stat, int32_t value, size_t* size);
    int pkt_parse_set_stat_int_ack(const char* data, size_t size, uint32_t* game, uint32_t* result, char** stat, int32_t* value);
    char* pkt_build_set_stat_int_ack(uint32_t game, uint32_t result, const char* stat, int32_t value, size_t* size);


    int pkt_parse_item_op(const char* data, size_t size, uint32_t* game, uint32_t* user, uint32_t* op, struct pkt_item** items, uint32_t* item_count);
    char* pkt_build_item_op(uint32_t game, uint32_t user, uint32_t op, struct pkt_item* items, uint32_t item_count, size_t* size);
    int pkt_parse_item_op_ack(const char* data, size_t size, uint32_t* result, uint32_t* game, uint32_t* user, uint32_t* op, struct pkt_item** items, uint32_t* item_count);
    char* pkt_build_item_op_ack(uint32_t result, uint32_t game, uint32_t user, uint32_t op, struct pkt_item* items, uint32_t item_count, size_t* size);

    int pkt_parse_order_op(const char* data, size_t size, uint32_t* game, uint32_t* user, uint32_t* order, uint32_t* op, char** token);
    char* pkt_build_order_op(uint32_t game, uint32_t user, uint32_t order, uint32_t op, const char* token, size_t* size);
    int pkt_parse_order_op_ack(const char* data, size_t size, uint32_t* game, uint32_t* user, uint32_t* result, uint32_t* order, uint32_t* op);
    char* pkt_build_order_op_ack(uint32_t game, uint32_t user, uint32_t result, uint32_t order, uint32_t op, size_t* size);


	void pkt_parse_get_pubkey_ack(const char* data, size_t size, char** pubkey,uint32_t* version);
	char* pkt_build_send_key(const char* key, size_t* size);
	char* pkt_build_heartbeat(uint32_t user, uint32_t* games,uint32_t*gamesarea,char**appid ,int infosize,const char* region ,const char* token, size_t* size);
	void pkt_parse_heartbeat(const char* data, size_t size, uint32_t* result, uint32_t**games, int*item_count);
	char* pkt_build_pubkey_version( size_t* size, uint32_t version);
	char* pkt_build_get_gamelist(uint32_t user, const char* token, uint32_t location, size_t* size);
	void pkt_parse_get_gamelist_ack(const char* data, size_t size, uint32_t* result, size_t* game_count, char*** const status, uint32_t** const gameid, uint32_t** const install_type, char*** const key_type, char*** const sku_ename, char*** const appid);
	char* pkt_build_send_hash(const char* hash, const char* token,uint32_t user,size_t* size);
	void pkt_parse_get_sendhash_ack(const char* data, size_t size, uint32_t* gameid,uint32_t *product, uint32_t* result,uint32_t *location ,char** appid,char ** hash,char** dlcs);
	char* pkt_build_adult_set(uint32_t adult,uint32_t time, size_t* size);
	int pkt_parse_adult_set(const char* data, size_t size, uint32_t* adult, uint32_t *time);
	void pkt_parse_start_game_ack(const char* data, size_t size, uint32_t* gameid, uint32_t* time);
	char* pkt_build_checkgameid(uint32_t user, uint32_t* game, int gamesize, const char* token,  uint32_t location, size_t* size);
	void pkt_parse_checkgameidack(const char* data, size_t size, uint32_t* result, uint32_t* games, int* item_count, struct pkt_data_item** dlcid);
	// mm
	struct pkt_user_item
	{
		char user[SK_ITEM_MAX];
	};
	struct pkt_data_item
	{
		char* data;
		int   len;
	};

	int pkt_parse_kv(const char* data, size_t size, char** key, char** value);
	char* pkt_build_kv(const char* key, const char* value, size_t* size);

	int pkt_parse_kv_x(const char* data, size_t size, char** key, uint32_t* type, uint32_t* ivalue, char** value);
	char* pkt_build_kv_x(const char* key, const char* value, uint32_t type, uint32_t ivalue, size_t* size);

	int pkt_parse_mm_req_s(const char* data, size_t size, uint32_t* op, uint32_t* sub_op, char** user, uint32_t* product, uint32_t* game, struct kv_list_x* kv_para_list, struct kv_list_x* kv_data_list);
	char* pkt_build_mm_req_s(uint32_t op, uint32_t sub_op, const char* user, uint32_t product, uint32_t game, struct kv_list_x* kv_para_list, struct kv_list_x* kv_data_list, size_t* size);
	int pkt_parse_mm_resp_s(const char* data, size_t size, uint32_t* op, uint32_t* sub_op, char** user, uint32_t* product, uint32_t* game, struct kv_list_x* kv_para_list, struct kv_list_x* kv_data_list, struct region_data_list* region_list, struct pkt_data_item** users_data, uint32_t* user_count);
	char* pkt_build_mm_resp_s(uint32_t op, uint32_t sub_op, const char* user, uint32_t product, uint32_t game, struct kv_list_x* kv_para_list, struct kv_list_x* kv_data_list, struct region_data_list* region_list, struct pkt_data_item* users_data, uint32_t user_count, size_t* size);

	int pkt_parse_user_data(const char* data, size_t size, uint32_t* bAI, char** user, char** user_name, char** user_avatar, uint32_t* team_type, int32_t* chair, uint32_t* state, struct pkt_data_item** data_list, uint32_t* count);
	char* pkt_build_user_data(uint32_t bAI, const char* user, const char* user_name, const char* user_avatar, uint32_t team_type, int32_t chair, uint32_t state, struct pkt_data_item* data_list, uint32_t count, size_t* size);
	
	#define MAX_STRING_LONGTH 1024
	enum sonkwoerr
	{
		broke_down,
		validation_failed
	};
	typedef struct err_log_info
	{
		uint32_t userid;
		uint32_t type;
		char version[MAX_STRING_LONGTH];
		char time[MAX_STRING_LONGTH];
		char details[MAX_STRING_LONGTH];
	}ts_err_log_info, *pts_err_log_info;

	struct pkt_str_item
	{
		const char* data;
		int   len;
	};


	char* pkt_build_deletecloudfile(uint32_t userid, uint32_t productid, struct pkt_str_item * filename,uint32_t filecout, size_t* size);
	char* pkt_build_getticket(uint32_t userid, uint32_t gameid,const char* appid,size_t *size);
	void pkt_parse_ticket_ack(const char* data, size_t size, uint32_t *game,char** ticket);
	char* pkt_build_ticket_for_game(const char* ticket, size_t* size);
	void pkt_pares_ticket_for_game(const char* data, size_t size, char** ticket);
	char* pkt_build_get_friend_status(uint32_t userid, const char* token, size_t *size);
	void pkt_parse_update_friend_status(const char* data, size_t size, uint32_t* type, char**onlines, char**offlines);
	char* pkt_build_game_started_notice(uint32_t game, uint32_t user, const char* name,const char* appid,const char* userarea,uint32_t gamearea, size_t* size);
	char* pkt_build_activate_to_sonkwo(const char* sonkwoappid, uint32_t user, const char* location,const char* steamid,const char* steamappid,uint32_t gameid ,size_t* size);
	void pkt_parse_activate_ack(const char* data, size_t size, uint32_t *game, uint32_t* result, char**region);
	char* pkt_build_steamappid_to_gameid(const char * steamappid, const char * region,size_t*size);
	void pkt_pares_steamappid_to_gameid_ack(const char * data, size_t size, uint32_t* gameid,uint32_t* result,char**region);
	char* pkt_build_gameid_to_steamappid(uint32_t game, const char * region, size_t *size);
	void pkt_pares_gameid_tosteamappid_ack(const char * data, size_t size, char**steamappid, uint32_t* gameid, uint32_t* result, char**region);
	char* pkt_build_generate_order(uint32_t sku,uint32_t location,const char* token,const char* appid,size_t*size);
	void pkt_pares_generate_order_ack(const char * data, size_t size, uint32_t * result, uint32_t*sku, uint32_t *price, \
		char** name, char** appid);
	char* pkt_build_verify_code_request(const char* token,size_t*size);
	void pkt_pares_verify_code_request_ack(const char* data,size_t size,uint32_t* result);
	char* pkt_build_confirm_pay_request(const char* token,uint32_t price,const char*messagecode,uint32_t location,uint32_t sku,\
		const char* appid,size_t *size);
	void pkt_pares_confirm_pay_request_ack(const char* data, size_t size, uint32_t* result,uint32_t* sku,char** appid);
	char* pkt_build_get_item_response(uint32_t sku, uint32_t result, size_t* size);
	void pkt_pares_get_item_response(const char* data, size_t size, uint32_t *sku, uint32_t *result);
	char* pkt_build_demo_status(uint32_t userid,uint32_t gameid, const char* appid,const char* area ,const char* status, size_t* size);
#ifdef __cplusplus
}
#endif
