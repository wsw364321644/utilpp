/**
 *  pkg_body.c 
 */

#include <string.h>
#include "pkt_body.h"
#include "mpack/mpack.h"

static char* pkt_parse_str(mpack_node_t root, const char* name)
{
    mpack_node_t cur;
    size_t size;
    const char* str;
    char* result;
    cur = mpack_node_map_cstr(root, name);
    if (mpack_node_type(cur) == mpack_type_nil)
        return NULL;
    
    size = mpack_node_strlen(cur);
    str = mpack_node_str(cur);
    result = calloc(1, size + 1);
    memcpy(result, str, size);
    return result;
}

void pkt_free(void* data)
{
	if (data)
		free(data);
}

void pkt_free_2d(char** data,uint32_t size)
{
	for (uint32_t i = 0; i < size; i++) {
		if(data[i])
			free(data[i]);
	}
	if (data)
		free(data);
}

int pkt_parse_game_info(const char* data, size_t size, uint32_t* version, char** dll_path, char** exe_name,char**sk_appid)
{
    mpack_tree_t tree;
    mpack_node_t root, cur;
    
    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);

    cur = mpack_node_map_cstr(root, CLIENT_VERSION);
    *version = mpack_node_u32(cur);

    *dll_path = pkt_parse_str(root, API_DLL_PATH);

    *exe_name = pkt_parse_str(root, EXE_NAME);

	*sk_appid = pkt_parse_str(root, APPID);
    mpack_tree_destroy(&tree);

    return 0;
}

char* pkt_build_game_info(uint32_t version, const char* dll_path, const char* exe_name,const char * sonkwo_appid, size_t* size)
{
    char* data;
    *size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 4);

    mpack_write_cstr(&writer, CLIENT_VERSION);
    mpack_write_u32(&writer, version);

    mpack_write_cstr(&writer, API_DLL_PATH);
    mpack_write_str(&writer, dll_path, strlen(dll_path));

    mpack_write_cstr(&writer, EXE_NAME);
    mpack_write_str(&writer, exe_name, strlen(exe_name));

	mpack_write_cstr(&writer, APPID);
	mpack_write_str(&writer, sonkwo_appid, strlen(sonkwo_appid));

    mpack_finish_map(&writer);

    mpack_writer_destroy(&writer);
    return data;
}

int pkt_parse_game_info_ack(const char* data, size_t size, uint32_t* game_id, uint32_t* user_id,char** userinfo,char**dlc)
{
    mpack_tree_t tree;
    mpack_node_t root, cur;
    
    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);
    cur = mpack_node_map_cstr(root, GAME);
    *game_id = mpack_node_u32(cur);
    cur = mpack_node_map_cstr(root, EXE_NAME);
    *user_id = mpack_node_u32(cur);
	*userinfo = pkt_parse_str(root, USER_INFO);
	*dlc = pkt_parse_str(root, DLC);
    mpack_tree_destroy(&tree);

    return 0;
}

char* pkt_build_game_info_ack(uint32_t game_id, uint32_t user_id,const char* info , const char* dlcs,size_t* size)
{
    char* data;
    *size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 4);
    mpack_write_cstr(&writer, GAME);
    mpack_write_u32(&writer, game_id);
    mpack_write_cstr(&writer, EXE_NAME);
    mpack_write_u32(&writer, user_id);
	mpack_write_cstr(&writer, USER_INFO);
	mpack_write_str(&writer, info,strlen(info));
	mpack_write_cstr(&writer, DLC);
	mpack_write_str(&writer, dlcs, strlen(dlcs));
    mpack_finish_map(&writer);

    mpack_writer_destroy(&writer);
    return data; 
}


int pkt_parse_achievement(const char* data, size_t size, uint32_t* game, uint32_t* user, uint32_t* op, struct pkt_achievement** aches, uint32_t* aches_size)
{
    mpack_tree_t tree;
    mpack_node_t root, cur, ach_root, ach_cur;
    size_t str_size, ach_size, i;
    const char* str;
    struct pkt_achievement* ptr;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);
    cur = mpack_node_map_cstr(root, GAME);
    *game = mpack_node_u32(cur);
    cur = mpack_node_map_cstr(root, USER);
    *user = mpack_node_u32(cur);
    cur = mpack_node_map_cstr(root, ACH_OPER);
    *op = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, ACHIEVEMENT_LIST);
    ach_size = mpack_node_array_length(cur);
    ptr = calloc(ach_size * sizeof(struct pkt_achievement), 1);
    for (i = 0; i < ach_size; i++) {
        ach_root = mpack_node_array_at(cur, i);
        ach_cur = mpack_node_map_cstr(ach_root, NAME);
        str_size = mpack_node_strlen(ach_cur);
        str = mpack_node_str(ach_cur);
        memcpy(ptr[i].name, str, str_size);
        ach_cur = mpack_node_map_cstr(ach_root, ACH_TIME);
        ptr[i].unlock_time = mpack_node_u64(ach_cur);
        ach_cur = mpack_node_map_cstr(ach_root, ACH_VALUE);
        ptr[i].value = mpack_node_u32(ach_cur);
    }
    mpack_tree_destroy(&tree);
    *aches = ptr;
    *aches_size = ach_size;
    return 0;
}

char* pkt_build_achievement(uint32_t game, uint32_t user, uint32_t op, struct pkt_achievement* aches, uint32_t aches_size, size_t* size)
{
    char* data;
    *size = 0;
    uint32_t i;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 4);
    mpack_write_cstr(&writer, GAME);
    mpack_write_u32(&writer, game);
    mpack_write_cstr(&writer, USER);
    mpack_write_u32(&writer, user);
    mpack_write_cstr(&writer, ACH_OPER);
    mpack_write_u32(&writer, op);

    mpack_write_cstr(&writer, ACHIEVEMENT_LIST);
    mpack_start_array(&writer, aches_size);
    for (i = 0; i < aches_size; i++) {
        mpack_start_map(&writer, 3);
        mpack_write_cstr(&writer, NAME);
        mpack_write_str(&writer, aches[i].name, strlen(aches[i].name));
        mpack_write_cstr(&writer, ACH_TIME);
        mpack_write_u64(&writer, aches[i].unlock_time);
        mpack_write_cstr(&writer, ACH_VALUE);
        mpack_write_u32(&writer, aches[i].value);
        mpack_finish_map(&writer);
    }
    mpack_finish_array(&writer);
    mpack_finish_map(&writer);

    mpack_writer_destroy(&writer);
    return data;
}

int pkt_parse_achievement_ack(const char* data, size_t size, uint32_t* op, uint32_t* result, struct pkt_achievement** aches, uint32_t* aches_size)
{
    mpack_tree_t tree;
    mpack_node_t root, cur, ach_root, ach_cur;
    size_t str_size, ach_size, i;
    const char* str;
    struct pkt_achievement* ptr;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);
    cur = mpack_node_map_cstr(root, ACH_OPER);
    *op = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, RESULT);
    *result = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, ACHIEVEMENT_LIST);
    ach_size = mpack_node_array_length(cur);
    ptr = calloc(ach_size * sizeof(struct pkt_achievement), 1);
    for (i = 0; i < ach_size; i++) {
        ach_root = mpack_node_array_at(cur, i);
        ach_cur = mpack_node_map_cstr(ach_root, NAME);
        str_size = mpack_node_strlen(ach_cur);
        str = mpack_node_str(ach_cur);
        memcpy(ptr[i].name, str, str_size);
        ach_cur = mpack_node_map_cstr(ach_root, ACH_TIME);
        ptr[i].unlock_time = mpack_node_u64(ach_cur);
        ach_cur = mpack_node_map_cstr(ach_root, ACH_VALUE);
        ptr[i].value = mpack_node_u32(ach_cur);
    }
    mpack_tree_destroy(&tree);
    *aches = ptr;
    *aches_size = ach_size;
    return 0;
}

char* pkt_build_achievement_ack(uint32_t op, uint32_t result, struct pkt_achievement* aches, uint32_t aches_size, size_t* size)
{
    char* data;
    *size = 0;
    uint32_t i;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 3);
    mpack_write_cstr(&writer, ACH_OPER);
    mpack_write_u32(&writer, op);
    mpack_write_cstr(&writer, RESULT);
    mpack_write_u32(&writer, result);

    mpack_write_cstr(&writer, ACHIEVEMENT_LIST);
    mpack_start_array(&writer, aches_size);
    for (i = 0; i < aches_size; i++) {
        mpack_start_map(&writer, 3);
        mpack_write_cstr(&writer, NAME);
        mpack_write_str(&writer, aches[i].name, strlen(aches[i].name));
        mpack_write_cstr(&writer, ACH_TIME);
        mpack_write_u64(&writer, aches[i].unlock_time);
        mpack_write_cstr(&writer, ACH_VALUE);
        mpack_write_u64(&writer, aches[i].value);
        mpack_finish_map(&writer);
    }
    mpack_finish_array(&writer);
    mpack_finish_map(&writer);

    mpack_writer_destroy(&writer);
    return data;
}

int pkt_parse_item_query(const char* data, size_t size, char** name)
{
    mpack_tree_t tree;
    mpack_node_t root;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);
    *name = pkt_parse_str(root, ITEM_NAME);

    mpack_tree_destroy(&tree);

    return 0;
}

char* pkt_build_item_query(const char* name, size_t* size)
{
    char* data;
    *size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 2);

    mpack_write_cstr(&writer, ITEM_NAME);
    mpack_write_str(&writer, name, strlen(name));

    mpack_finish_map(&writer);

    mpack_writer_destroy(&writer);
    return data;
}

int pkt_parse_item_query_ack(const char* data, size_t size, uint32_t* result, struct pkt_item** items, uint32_t* item_count)
{
    mpack_tree_t tree;
    mpack_node_t root, cur, item_root, item_cur;
    size_t str_size, item_size, i;
    const char* str;
    struct pkt_item* ptr;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);
    cur = mpack_node_map_cstr(root, RESULT);
    *result = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, ITEM_LIST);
    item_size = mpack_node_array_length(cur);
    ptr = calloc(item_size * sizeof(struct pkt_item), 1);
    for (i = 0; i < item_size; i++) {
        item_root = mpack_node_array_at(cur, i);
        item_cur = mpack_node_map_cstr(item_root, ITEM_NAME);
        str_size = mpack_node_strlen(item_cur);
        str = mpack_node_str(item_cur);
        memcpy(ptr[i].name, str, str_size);
        item_cur = mpack_node_map_cstr(item_root, ITEM_PRICE);
        ptr[i].price = mpack_node_u32(item_cur);
    }
    mpack_tree_destroy(&tree);
    *items = ptr;
    *item_count = item_size;
    return 0;
}

char* pkt_build_item_query_ack(uint32_t result, struct pkt_item* items, uint32_t item_count, size_t* size)
{
    char* data;
    *size = 0;
    uint32_t i;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, RESULT);
    mpack_write_u32(&writer, result);

    mpack_write_cstr(&writer, ITEM_LIST);
    mpack_start_array(&writer, item_count);
    for (i = 0; i < item_count; i++) {
        mpack_start_map(&writer, 2);
        mpack_write_cstr(&writer, ITEM_NAME);
        mpack_write_str(&writer, items[i].name, strlen(items[i].name));
        mpack_write_cstr(&writer, ITEM_PRICE);
        mpack_write_u32(&writer, items[i].price);
        mpack_finish_map(&writer);
    }
    mpack_finish_array(&writer);
    mpack_finish_map(&writer);

    mpack_writer_destroy(&writer);
    return data;
}

int pkt_parse_item_purchase(const char* data, size_t size, char** name, uint32_t* count)
{
    mpack_tree_t tree;
    mpack_node_t root, cur;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);
    *name = pkt_parse_str(root, ITEM_NAME);

    cur = mpack_node_map_cstr(root, ITEM_COUNT);
    *count = mpack_node_u32(cur);
    mpack_tree_destroy(&tree);

    return 0;
}

char* pkt_build_item_purchase(const char* name, uint32_t count, size_t* size)
{
    char* data;
    *size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 2);

    mpack_write_cstr(&writer, ITEM_NAME);
    mpack_write_str(&writer, name, strlen(name));

    mpack_write_cstr(&writer, ITEM_COUNT);
    mpack_write_u32(&writer, count);

    mpack_finish_map(&writer);

    mpack_writer_destroy(&writer);
    return data;
}

int pkt_parse_item_purchase_ack(const char* data, size_t size, uint32_t* result, uint32_t* order, char** name, uint32_t* count, uint32_t* price, uint32_t* total)
{
    mpack_tree_t tree;
    mpack_node_t root, cur;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);

    cur = mpack_node_map_cstr(root, RESULT);
    *result = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, ORDER_ID);
    *order = mpack_node_u32(cur);

    *name = pkt_parse_str(root, ITEM_NAME);

    cur = mpack_node_map_cstr(root, ITEM_COUNT);
    *count = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, ITEM_PRICE);
    *price = mpack_node_u32(cur);


    cur = mpack_node_map_cstr(root, ITEM_TOTAL);
    *total = mpack_node_u32(cur);

    mpack_tree_destroy(&tree);
    return 0;
}

char* pkt_build_item_purchase_ack(uint32_t result, uint32_t order, const char* name, uint32_t count, uint32_t price, uint32_t total, size_t* size)
{
    char* data;
    *size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 6);

    mpack_write_cstr(&writer, RESULT);
    mpack_write_u32(&writer, result);
    mpack_write_cstr(&writer, ORDER_ID);
    mpack_write_u32(&writer, order);
    mpack_write_cstr(&writer, ITEM_COUNT);
    mpack_write_u32(&writer, count);
    mpack_write_cstr(&writer, ITEM_NAME);
    mpack_write_str(&writer, name, strlen(name));
    mpack_write_cstr(&writer, ITEM_PRICE);
    mpack_write_u32(&writer, price);
    mpack_write_cstr(&writer, ITEM_TOTAL);
    mpack_write_u32(&writer, total);

    mpack_finish_map(&writer);

    mpack_writer_destroy(&writer);
    return data;
}

int pkt_parse_order_pay(const char* data, size_t size, uint32_t* order)
{
    mpack_tree_t tree;
    mpack_node_t root, cur;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);

    cur = mpack_node_map_cstr(root, ORDER_ID);
    *order = mpack_node_u32(cur);

    mpack_tree_destroy(&tree);
    return 0;
}

char* pkt_build_order_pay(uint32_t order, size_t* size)
{
    char* data;
    *size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 1);

    mpack_write_cstr(&writer, ORDER_ID);
    mpack_write_u32(&writer, order);

    mpack_finish_map(&writer);

    mpack_writer_destroy(&writer);
    return data;

}

int pkt_parse_order_pay_ack(const char* data, size_t size, uint32_t* result, uint32_t* order)
{
    mpack_tree_t tree;
    mpack_node_t root, cur;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);

    cur = mpack_node_map_cstr(root, RESULT);
    *result = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, ORDER_ID);
    *order = mpack_node_u32(cur);

    mpack_tree_destroy(&tree);
    return 0;

}

char* pkt_build_order_pay_ack(uint32_t result, uint32_t order, size_t* size)
{
    char* data;
    *size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 2);

    mpack_write_cstr(&writer, RESULT);
    mpack_write_u32(&writer, result);

    mpack_write_cstr(&writer, ORDER_ID);
    mpack_write_u32(&writer, order);

    mpack_finish_map(&writer);

    mpack_writer_destroy(&writer);
    return data;

}

int pkt_parse_order_checkout(const char* data, size_t size, uint32_t* order, char** token)
{
    mpack_tree_t tree;
    mpack_node_t root, cur;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);

    cur = mpack_node_map_cstr(root, ORDER_ID);
    *order = mpack_node_u32(cur);

    *token = pkt_parse_str(root, ORDER_TOKEN);

    mpack_tree_destroy(&tree);
    return 0;
}

char* pkt_build_order_checkout(uint32_t order, const char* token, size_t* size)
{
    char* data;
    *size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 2);

    mpack_write_cstr(&writer, ORDER_ID);
    mpack_write_u32(&writer, order);
    mpack_write_cstr(&writer, ORDER_TOKEN);
    mpack_write_str(&writer, token, strlen(token));

    mpack_finish_map(&writer);

    mpack_writer_destroy(&writer);
    return data;
}

int pkt_parse_order_checkout_ack(const char* data, size_t size, uint32_t* result, uint32_t* order)
{
    mpack_tree_t tree;
    mpack_node_t root, cur;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);

    cur = mpack_node_map_cstr(root, RESULT);
    *result = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, ORDER_ID);
    *order = mpack_node_u32(cur);

    mpack_tree_destroy(&tree);
    return 0;
}

char* pkt_build_order_checkout_ack(uint32_t result, uint32_t order, size_t* size)
{
    char* data;
    *size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 2);

    mpack_write_cstr(&writer, RESULT);
    mpack_write_u32(&writer, result);

    mpack_write_cstr(&writer, ORDER_ID);
    mpack_write_u32(&writer, order);

    mpack_finish_map(&writer);

    mpack_writer_destroy(&writer);
    return data;
}


int pkt_parse_mm_req(const char* data, size_t size, uint32_t* op, uint32_t* sub_op, struct kv_list_x* kv_para_list, struct kv_list_x* kv_data_list)
{
	mpack_tree_t tree;
	mpack_node_t root, cur, item_root;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);

	cur = mpack_node_map_cstr(root, K_PARA_OP);
	*op = mpack_node_u32(cur);
	cur = mpack_node_map_cstr(root, K_PARA_SUB_OP);
	*sub_op = mpack_node_u32(cur);

	cur = mpack_node_map_cstr(root, K_PARA_LIST);
	size_t item_size = mpack_node_array_length(cur);
	if (item_size > 0)
	{
		kv_para_list->kv_count = item_size;
		kv_para_list->kv_data = calloc(item_size * sizeof(struct pkt_kv_data_x), 1);
		size_t i;
		for (i = 0; i < item_size; i++)
		{
			item_root = mpack_node_array_at(cur, i);
			kv_para_list->kv_data[i].key = pkt_parse_str(item_root, K_KEY);

			mpack_node_t sub_cur = mpack_node_map_cstr(item_root, K_TYPE);
			kv_para_list->kv_data[i].type = mpack_node_u32(sub_cur);
			if (kv_para_list->kv_data[i].type == PVT_INT)
			{
				sub_cur = mpack_node_map_cstr(item_root, K_IVALUE);
				kv_para_list->kv_data[i].ivalue = mpack_node_u32(sub_cur);
			}
			else
				kv_para_list->kv_data[i].value = pkt_parse_str(item_root, K_VALUE);
		}
	}
	else
	{
		kv_para_list->kv_data = NULL;
		kv_para_list->kv_count = 0;
	}

	cur = mpack_node_map_cstr(root, K_DATA_LIST);
	item_size = mpack_node_array_length(cur);
	if (item_size > 0)
	{
		kv_data_list->kv_count = item_size;
		kv_data_list->kv_data = calloc(item_size * sizeof(struct pkt_kv_data_x), 1);
		size_t i;
		for (i = 0; i < item_size; i++)
		{
			item_root = mpack_node_array_at(cur, i);
			kv_data_list->kv_data[i].key = pkt_parse_str(item_root, K_KEY);

			mpack_node_t sub_cur = mpack_node_map_cstr(item_root, K_TYPE);
			kv_data_list->kv_data[i].type = mpack_node_u32(sub_cur);
			if (kv_data_list->kv_data[i].type == PVT_INT)
			{
				sub_cur = mpack_node_map_cstr(item_root, K_IVALUE);
				kv_data_list->kv_data[i].ivalue = mpack_node_u32(sub_cur);
			}
			else
				kv_data_list->kv_data[i].value = pkt_parse_str(item_root, K_VALUE);
		}
	}
	else
	{
		kv_data_list->kv_data = NULL;
		kv_data_list->kv_count = 0;
	}

	mpack_tree_destroy(&tree);
	return 0;
}

char* pkt_build_mm_req(uint32_t op, uint32_t sub_op, struct kv_list_x* kv_para_list, struct kv_list_x* kv_data_list, size_t* size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);

	mpack_start_map(&writer, 4);

	mpack_write_cstr(&writer, K_PARA_OP);
	mpack_write_u32(&writer, op);
	mpack_write_cstr(&writer, K_PARA_SUB_OP);
	mpack_write_u32(&writer, sub_op);

	if (kv_para_list && kv_para_list->kv_data)
	{
		size_t i;
		mpack_write_cstr(&writer, K_PARA_LIST);
		mpack_start_array(&writer, kv_para_list->kv_count);
		for (i = 0; i < kv_para_list->kv_count; i++) {
			mpack_start_map(&writer, 3);
			mpack_write_cstr(&writer, K_KEY);
			mpack_write_str(&writer, kv_para_list->kv_data[i].key, strlen(kv_para_list->kv_data[i].key));

			mpack_write_cstr(&writer, K_TYPE);
			mpack_write_u32(&writer, kv_para_list->kv_data[i].type);

			if (kv_para_list->kv_data[i].type == PVT_INT)
			{
				mpack_write_cstr(&writer, K_IVALUE);
				mpack_write_u32(&writer, kv_para_list->kv_data[i].ivalue);
			}
			else
			{
				mpack_write_cstr(&writer, K_VALUE);
				mpack_write_str(&writer, kv_para_list->kv_data[i].value, strlen(kv_para_list->kv_data[i].value));
			}
			mpack_finish_map(&writer);
		}
		mpack_finish_array(&writer);
	}
	else
	{
		mpack_write_cstr(&writer, K_PARA_LIST);
		mpack_start_array(&writer, 0);
		mpack_finish_array(&writer);
	}

	if (kv_data_list && kv_data_list->kv_data)
	{
		size_t i;
		mpack_write_cstr(&writer, K_DATA_LIST);
		mpack_start_array(&writer, kv_data_list->kv_count);
		for (i = 0; i < kv_data_list->kv_count; i++) {
			mpack_start_map(&writer, 3);
			mpack_write_cstr(&writer, K_KEY);
			mpack_write_str(&writer, kv_data_list->kv_data[i].key, strlen(kv_data_list->kv_data[i].key));

			mpack_write_cstr(&writer, K_TYPE);
			mpack_write_u32(&writer, kv_data_list->kv_data[i].type);

			if (kv_data_list->kv_data[i].type == PVT_INT)
			{
				mpack_write_cstr(&writer, K_IVALUE);
				mpack_write_u32(&writer, kv_data_list->kv_data[i].ivalue);
			}
			else
			{
				mpack_write_cstr(&writer, K_VALUE);
				mpack_write_str(&writer, kv_data_list->kv_data[i].value, strlen(kv_data_list->kv_data[i].value));
			}
			mpack_finish_map(&writer);
		}
		mpack_finish_array(&writer);
	}
	else
	{
		mpack_write_cstr(&writer, K_DATA_LIST);
		mpack_start_array(&writer, 0);
		mpack_finish_array(&writer);
	}

	mpack_finish_map(&writer);

	mpack_writer_destroy(&writer);
	return data;
}

int pkt_parse_mm_resp(const char* data, size_t size, uint32_t* op, uint32_t* sub_op, struct kv_list_x* kv_para_list, struct kv_list_x* kv_data_list, struct region_data_list* region_list, struct pkt_user_data** user_list, uint32_t* user_count)
{
	mpack_tree_t tree;
	mpack_node_t root, cur, item_root, item_cur, sub_item_root;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);

	cur = mpack_node_map_cstr(root, K_PARA_OP);
	*op = mpack_node_u32(cur);
	cur = mpack_node_map_cstr(root, K_PARA_SUB_OP);
	*sub_op = mpack_node_u32(cur);

	cur = mpack_node_map_cstr(root, K_USER_LIST);
	size_t item_size = mpack_node_array_length(cur);
	*user_count = item_size;
	if (item_size > 0)
	{
		size_t i, j;
		struct pkt_user_data* tmp_user_list = calloc(item_size * sizeof(struct pkt_user_data), 1);
		for (i = 0; i < item_size; i++)
		{
			item_root = mpack_node_array_at(cur, i);

			item_cur = mpack_node_map_cstr(item_root, K_BOOL_AI);
			tmp_user_list[i].bAI = mpack_node_u32(item_cur);
			item_cur = mpack_node_map_cstr(item_root, K_TYPE);
			tmp_user_list[i].team_type = mpack_node_u32(item_cur);
			item_cur = mpack_node_map_cstr(item_root, K_CHAIR);
			tmp_user_list[i].chair = mpack_node_u32(item_cur);
			item_cur = mpack_node_map_cstr(item_root, K_STATE);
			tmp_user_list[i].state = mpack_node_u32(item_cur);

			tmp_user_list[i].user = pkt_parse_str(item_root, USER);
			tmp_user_list[i].name = pkt_parse_str(item_root, K_USER_NAME);
			tmp_user_list[i].avatar = pkt_parse_str(item_root, K_USER_AVATAR);

			item_cur = mpack_node_map_cstr(item_root, K_KV_COUNT);
			tmp_user_list[i].kv_count = mpack_node_u32(item_cur);
			if (tmp_user_list[i].kv_count > 0)
			{
				item_cur = mpack_node_map_cstr(item_root, K_SUB_DATA_LIST);
				size_t sub_item_size = mpack_node_array_length(item_cur);
				tmp_user_list[i].kv_list = calloc(sub_item_size * sizeof(struct pkt_kv_data_x), 1);
				for (j = 0; j < sub_item_size; j++)
				{
					sub_item_root = mpack_node_array_at(item_cur, j);
					tmp_user_list[i].kv_list[j].key = pkt_parse_str(sub_item_root, K_KEY);
					tmp_user_list[i].kv_list[j].value = pkt_parse_str(sub_item_root, K_VALUE);
					tmp_user_list[i].kv_list[j].type = PVT_STRING;
				}
			}
			else
			{
				tmp_user_list[i].kv_list = NULL;
				tmp_user_list[i].kv_count = 0;
			}
		}
		
		*user_list = tmp_user_list;
	}
	else
		*user_list = NULL;

	cur = mpack_node_map_cstr(root, K_PARA_LIST);
	item_size = mpack_node_array_length(cur);
	if (item_size > 0)
	{
		kv_para_list->kv_count = item_size;
		kv_para_list->kv_data = calloc(item_size * sizeof(struct pkt_kv_data_x), 1);
		size_t i;
		for (i = 0; i < item_size; i++)
		{
			item_root = mpack_node_array_at(cur, i);
			kv_para_list->kv_data[i].key = pkt_parse_str(item_root, K_KEY);

			mpack_node_t sub_cur = mpack_node_map_cstr(item_root, K_TYPE);
			kv_para_list->kv_data[i].type = mpack_node_u32(sub_cur);
			if (kv_para_list->kv_data[i].type == PVT_INT)
			{
				sub_cur = mpack_node_map_cstr(item_root, K_IVALUE);
				kv_para_list->kv_data[i].ivalue = mpack_node_u32(sub_cur);
			}
			else
				kv_para_list->kv_data[i].value = pkt_parse_str(item_root, K_VALUE);
		}
	}
	else
	{
		kv_para_list->kv_data = NULL;
		kv_para_list->kv_count = 0;
	}

	cur = mpack_node_map_cstr(root, K_DATA_LIST);
	item_size = mpack_node_array_length(cur);
	if (item_size > 0)
	{
		kv_data_list->kv_count = item_size;
		kv_data_list->kv_data = calloc(item_size * sizeof(struct pkt_kv_data_x), 1);
		size_t i;
		for (i = 0; i < item_size; i++)
		{
			item_root = mpack_node_array_at(cur, i);
			kv_data_list->kv_data[i].key = pkt_parse_str(item_root, K_KEY);

			mpack_node_t sub_cur = mpack_node_map_cstr(item_root, K_TYPE);
			kv_data_list->kv_data[i].type = mpack_node_u32(sub_cur);
			if (kv_data_list->kv_data[i].type == PVT_INT)
			{
				sub_cur = mpack_node_map_cstr(item_root, K_IVALUE);
				kv_data_list->kv_data[i].ivalue = mpack_node_u32(sub_cur);
			}
			else
				kv_data_list->kv_data[i].value = pkt_parse_str(item_root, K_VALUE);
		}
	}
	else
	{
		kv_data_list->kv_data = NULL;
		kv_data_list->kv_count = 0;
	}

	cur = mpack_node_map_cstr(root, K_REGION_LIST);
	item_size = mpack_node_array_length(cur);
	if (item_size > 0)
	{
		region_list->count = item_size;
		region_list->r_data = calloc(item_size * sizeof(struct region_data), 1);
		size_t i;
		for (i = 0; i < item_size; i++)
		{
			item_root = mpack_node_array_at(cur, i);
			region_list->r_data[i].name = pkt_parse_str(item_root, K_REGION_NAME);
			mpack_node_t sub_cur = mpack_node_map_cstr(item_root, K_TTL);
			region_list->r_data[i].ttl = mpack_node_i32(sub_cur);
		}
	}
	else
	{
		region_list->r_data = NULL;
		region_list->count = 0;
	}

	mpack_tree_destroy(&tree);
	return 0;
}
char* pkt_build_mm_resp(uint32_t op, uint32_t sub_op, struct kv_list_x* kv_para_list, struct kv_list_x* kv_data_list, struct region_data_list* region_list, struct pkt_user_data* user_list, uint32_t user_count, size_t* size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);

	mpack_start_map(&writer, 6);

	mpack_write_cstr(&writer, K_PARA_OP);
	mpack_write_u32(&writer, op);
	mpack_write_cstr(&writer, K_PARA_SUB_OP);
	mpack_write_u32(&writer, sub_op);

	size_t i, j;
	mpack_write_cstr(&writer, K_USER_LIST);
	mpack_start_array(&writer, user_count);
	for (i = 0; i < user_count; i++)
	{
		mpack_start_map(&writer, 9);

		mpack_write_cstr(&writer, K_BOOL_AI);
		mpack_write_u32(&writer, user_list[i].bAI);
		mpack_write_cstr(&writer, K_TYPE);
		mpack_write_u32(&writer, user_list[i].team_type);
		mpack_write_cstr(&writer, K_CHAIR);
		mpack_write_u32(&writer, user_list[i].chair);
		mpack_write_cstr(&writer, K_STATE);
		mpack_write_u32(&writer, user_list[i].state);
		mpack_write_cstr(&writer, USER);
		mpack_write_str(&writer, user_list[i].user, strlen(user_list[i].user));
		mpack_write_cstr(&writer, K_USER_NAME);
		mpack_write_str(&writer, user_list[i].name, strlen(user_list[i].name));
		mpack_write_cstr(&writer, K_USER_AVATAR);
		mpack_write_str(&writer, user_list[i].avatar, strlen(user_list[i].avatar));
		mpack_write_cstr(&writer, K_KV_COUNT);
		mpack_write_u32(&writer, user_list[i].kv_count);
		{
			mpack_write_cstr(&writer, K_SUB_DATA_LIST);
			mpack_start_array(&writer, user_list[i].kv_count);
			for (j = 0; j < user_list[i].kv_count; j++)
			{
				mpack_start_map(&writer, 2);
				mpack_write_cstr(&writer, K_KEY);
				mpack_write_str(&writer, user_list[i].kv_list[j].key, strlen(user_list[i].kv_list[j].key));
				mpack_write_cstr(&writer, K_VALUE);
				mpack_write_str(&writer, user_list[i].kv_list[j].value, strlen(user_list[i].kv_list[j].value));
				mpack_finish_map(&writer);
			}
			mpack_finish_array(&writer);
		}
		mpack_finish_map(&writer);
	}
	mpack_finish_array(&writer);

	if (kv_para_list && kv_para_list->kv_data)
	{
		size_t i;
		mpack_write_cstr(&writer, K_PARA_LIST);
		mpack_start_array(&writer, kv_para_list->kv_count);
		for (i = 0; i < kv_para_list->kv_count; i++) {
			mpack_start_map(&writer, 3);
			mpack_write_cstr(&writer, K_KEY);
			mpack_write_str(&writer, kv_para_list->kv_data[i].key, strlen(kv_para_list->kv_data[i].key));

			mpack_write_cstr(&writer, K_TYPE);
			mpack_write_u32(&writer, kv_para_list->kv_data[i].type);

			if (kv_para_list->kv_data[i].type == PVT_INT)
			{
				mpack_write_cstr(&writer, K_IVALUE);
				mpack_write_u32(&writer, kv_para_list->kv_data[i].ivalue);
			}
			else
			{
				mpack_write_cstr(&writer, K_VALUE);
				mpack_write_str(&writer, kv_para_list->kv_data[i].value, strlen(kv_para_list->kv_data[i].value));
			}
			mpack_finish_map(&writer);
		}
		mpack_finish_array(&writer);
	}
	else
	{
		mpack_write_cstr(&writer, K_PARA_LIST);
		mpack_start_array(&writer, 0);
		mpack_finish_array(&writer);
	}

	if (kv_data_list && kv_data_list->kv_data)
	{
		size_t i;
		mpack_write_cstr(&writer, K_DATA_LIST);
		mpack_start_array(&writer, kv_data_list->kv_count);
		for (i = 0; i < kv_data_list->kv_count; i++) {
			mpack_start_map(&writer, 3);
			mpack_write_cstr(&writer, K_KEY);
			mpack_write_str(&writer, kv_data_list->kv_data[i].key, strlen(kv_data_list->kv_data[i].key));

			mpack_write_cstr(&writer, K_TYPE);
			mpack_write_u32(&writer, kv_data_list->kv_data[i].type);

			if (kv_data_list->kv_data[i].type == PVT_INT)
			{
				mpack_write_cstr(&writer, K_IVALUE);
				mpack_write_u32(&writer, kv_data_list->kv_data[i].ivalue);
			}
			else
			{
				mpack_write_cstr(&writer, K_VALUE);
				mpack_write_str(&writer, kv_data_list->kv_data[i].value, strlen(kv_data_list->kv_data[i].value));
			}
			mpack_finish_map(&writer);
		}
		mpack_finish_array(&writer);
	}
	else
	{
		mpack_write_cstr(&writer, K_DATA_LIST);
		mpack_start_array(&writer, 0);
		mpack_finish_array(&writer);
	}

	if (region_list && region_list->r_data)
	{
		size_t i;
		mpack_write_cstr(&writer, K_REGION_LIST);
		mpack_start_array(&writer, region_list->count);
		for (i = 0; i < region_list->count; i++) {
			mpack_start_map(&writer, 2);

			mpack_write_cstr(&writer, K_REGION_NAME);
			mpack_write_str(&writer, region_list->r_data[i].name, strlen(region_list->r_data[i].name));
			mpack_write_cstr(&writer, K_TTL);
			mpack_write_i32(&writer, region_list->r_data[i].ttl);
			mpack_finish_map(&writer);
		}
		mpack_finish_array(&writer);
	}
	else
	{
		mpack_write_cstr(&writer, K_REGION_LIST);
		mpack_start_array(&writer, 0);
		mpack_finish_array(&writer);
	}

	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);
	return data;
}

int pkt_parse_ack(const char* data, size_t size, int32_t* error, int32_t* request)
{
    mpack_tree_t tree;
    mpack_node_t root, cur;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);

    cur = mpack_node_map_cstr(root, ACK_ERROR);
    *error = mpack_node_i32(cur);

    cur = mpack_node_map_cstr(root, ACK_REQUEST);
    *request = mpack_node_i32(cur);

    mpack_tree_destroy(&tree);
    return 0;
}

char* pkt_build_ack(int32_t error, int32_t request, size_t* size)
{
    char* data;
    *size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, ACK_ERROR);
    mpack_write_i32(&writer, error);
    mpack_write_cstr(&writer, ACK_REQUEST);
    mpack_write_i32(&writer, request);
    mpack_finish_map(&writer);

    mpack_writer_destroy(&writer);
    return data;
}

int pkt_parse_login(const char* data, size_t size, uint32_t* user, char** token)
{
    mpack_tree_t tree;
    mpack_node_t root, cur;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);

    cur = mpack_node_map_cstr(root, USER);
    *user = mpack_node_u32(cur);

    *token = pkt_parse_str(root, REFRESH_TOKEN);

    mpack_tree_destroy(&tree);
    return 0;
}

char* pkt_build_login(uint32_t user, const char* token, size_t* size)
{
    char* data;
    *size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 2);

    mpack_write_cstr(&writer, USER);
    mpack_write_u32(&writer, user);

    mpack_write_cstr(&writer, REFRESH_TOKEN);
    mpack_write_str(&writer, token, strlen(token));

    mpack_finish_map(&writer);

    mpack_writer_destroy(&writer);
    return data;
}

int pkt_parse_login_ack(const char* data, size_t size, uint32_t* user, uint32_t* result, char** token)
{
    mpack_tree_t tree;
    mpack_node_t root, cur;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);

    cur = mpack_node_map_cstr(root, USER);
    *user = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, RESULT);
    *result = mpack_node_u32(cur);

    *token = pkt_parse_str(root, PERMERNENT_TOKEN);

    mpack_tree_destroy(&tree);
    return 0;
}

char* pkt_build_login_ack(uint32_t user, uint32_t result, const char* token, size_t* size)
{
    char* data;
    *size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 3);

    mpack_write_cstr(&writer, USER);
    mpack_write_u32(&writer, user);

    mpack_write_cstr(&writer, RESULT);
    mpack_write_u32(&writer, result);

    mpack_write_cstr(&writer, PERMERNENT_TOKEN);
    mpack_write_str(&writer, token, strlen(token));

    mpack_finish_map(&writer);

    mpack_writer_destroy(&writer);
    return data;
}

int pkt_parse_fetch_achievements(const char* data, size_t size, uint32_t* game, uint32_t* user)
{
    return -1;
}

char* pkt_build_fetch_achievements(uint32_t game, uint32_t user, size_t* size)
{
    char* data;
    *size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 2);

    mpack_write_cstr(&writer, GAME);
    mpack_write_u32(&writer, game);
    mpack_write_cstr(&writer, USER);
    mpack_write_u32(&writer, user);

    mpack_finish_map(&writer);

    mpack_writer_destroy(&writer);
    return data;
}

int pkt_parse_fetch_achievements_ack(const char* data, size_t size, uint32_t* game, uint32_t* user, struct pkt_achievement** achievements, size_t* ach_num)
{
    mpack_tree_t tree;
    mpack_node_t root, cur, ach_root, ach_cur;
    size_t ach_size;
    struct pkt_achievement* ptr;
    size_t i;
    const char* str;
    size_t str_size;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);

    cur = mpack_node_map_cstr(root, GAME);
    *game = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, USER);
    *user = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, ACHIEVEMENT_LIST);
    ach_size = mpack_node_array_length(cur);
    ptr = calloc(ach_size * sizeof(struct pkt_achievement), 1);
    for (i = 0; i < ach_size; i++) {
        ach_root = mpack_node_array_at(cur, i);
        ach_cur = mpack_node_map_cstr(ach_root, NAME);
        str_size = mpack_node_strlen(ach_cur);
        str = mpack_node_str(ach_cur);
        memcpy(ptr[i].name, str, str_size);
        ach_cur = mpack_node_map_cstr(ach_root, ACH_TIME);
        ptr[i].unlock_time = mpack_node_u64(ach_cur);
        ach_cur = mpack_node_map_cstr(ach_root, ACH_VALUE);
        ptr[i].value = mpack_node_u32(ach_cur);
    }

    mpack_tree_destroy(&tree);

    *achievements = ptr;
    *ach_num = ach_size;
    return 0;
}

char* pkt_build_fetch_achievements_ack(uint32_t game, uint32_t user, struct pkt_achievement* achievements, size_t ach_size, size_t* size)
{
    return NULL;
}

int pkt_parse_unlock_achievement(const char* data, size_t size, uint32_t* game, uint32_t* user, char** achievement)
{
    return -1;
}

char* pkt_build_unlock_achievement(uint32_t game, uint32_t user, const char* achievement, size_t* size)
{
    char* data;
    *size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 3);

    mpack_write_cstr(&writer, GAME);
    mpack_write_u32(&writer, game);
    mpack_write_cstr(&writer, USER);
    mpack_write_u32(&writer, user);
    mpack_write_cstr(&writer, ACHIEVEMENT);
    mpack_write_str(&writer, achievement, strlen(achievement));

    mpack_finish_map(&writer);

    mpack_writer_destroy(&writer);
    return data;
}

int pkt_parse_unlock_achievement_ack(const char* data, size_t size, uint32_t* game, uint32_t* user, uint32_t* result, char** achievement, uint64_t* unlock)
{
    mpack_tree_t tree;
    mpack_node_t root, cur;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);

    cur = mpack_node_map_cstr(root, GAME);
    *game = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, USER);
    *user = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, RESULT);
    *result = mpack_node_u32(cur);

    *achievement = pkt_parse_str(root, ACHIEVEMENT); 

    cur = mpack_node_map_cstr(root, ACH_TIME);
    *unlock = mpack_node_u64(cur);
    
    mpack_tree_destroy(&tree);
    return 0;
}

char* pkt_build_unlock_achievement_ack(uint32_t game, uint32_t user, uint32_t result, const char* achievement, uint64_t unlock, size_t* size)
{
    return NULL;
}

int pkt_parse_sync_file(const char* data, size_t size, uint32_t* game, uint32_t* user, uint32_t* op, char** file_name, char** checksum)
{
    return -1;
}

char* pkt_build_sync_file(uint32_t game, uint32_t user, uint32_t op, const char* file_name, const char* checksum, size_t* size)
{
    char* data;
    *size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    if (op == FO_TOKEN) {
        mpack_start_map(&writer, 4);

        mpack_write_cstr(&writer, GAME);
        mpack_write_u32(&writer, game);
        mpack_write_cstr(&writer, USER);
        mpack_write_u32(&writer, user);
        mpack_write_cstr(&writer, FILE_OP);
        mpack_write_u32(&writer, op);
        mpack_write_cstr(&writer, FILE_NAME);
        mpack_write_str(&writer, file_name, strlen(file_name));

        mpack_finish_map(&writer);
    } else if (op == FO_VERIFY) {
        mpack_start_map(&writer, 5);

        mpack_write_cstr(&writer, GAME);
        mpack_write_u32(&writer, game);
        mpack_write_cstr(&writer, USER);
        mpack_write_u32(&writer, user);
        mpack_write_cstr(&writer, FILE_OP);
        mpack_write_u32(&writer, op);
        mpack_write_cstr(&writer, FILE_NAME);
        mpack_write_str(&writer, file_name, strlen(file_name));
        mpack_write_cstr(&writer, CHECKSUM);
        mpack_write_str(&writer, checksum, strlen(checksum));

        mpack_finish_map(&writer);
    }

    mpack_writer_destroy(&writer);
    return data;
}

int pkt_parse_sync_file_ack(const char* data, size_t size, uint32_t* game, uint32_t* user, uint32_t* op, uint32_t* result, 
    char** file_name, char**file_key, char** token, uint64_t* timestamp, char** x_file_name, char** x_sonkwo_token)
{
    mpack_tree_t tree;
    mpack_node_t root, cur;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);

    cur = mpack_node_map_cstr(root, GAME);
    *game = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, USER);
    *user = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, FILE_OP);
    *op = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, RESULT);
    *result = mpack_node_u32(cur);

    *file_name = pkt_parse_str(root, FILE_NAME);
    if (*op == FO_TOKEN) {
        *token = pkt_parse_str(root, FILE_TOKEN);
        *file_key = pkt_parse_str(root, FILE_KEY);

        cur = mpack_node_map_cstr(root, X_TIMESTAMP);
        *timestamp = mpack_node_u64(cur);
        *x_file_name = pkt_parse_str(root, X_FILE_NAME);
        *x_sonkwo_token = pkt_parse_str(root, X_TOKEN);
    }

    mpack_tree_destroy(&tree);
    return 0;
}

char* pkt_build_sync_file_ack(uint32_t game, uint32_t user, uint32_t op, uint32_t result, const char* file_name, const char* file_url, const char* checksum, size_t* size)
{
    return NULL;
}

int pkt_parse_fetch_save_files(const char* data, size_t size, uint32_t* game, uint32_t* user)
{
    return -1;
}

char* pkt_build_fetch_save_files(uint32_t game, uint32_t user, size_t* size)
{
    char* data;
    *size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 2);

    mpack_write_cstr(&writer, GAME);
    mpack_write_u32(&writer, game);
    mpack_write_cstr(&writer, USER);
    mpack_write_u32(&writer, user);

    mpack_finish_map(&writer);
    mpack_writer_destroy(&writer);

    return data;
}

int pkt_parse_fetch_save_files_ack(const char* data, size_t size, uint32_t* game, uint32_t* user, struct pkt_file_info** files, size_t* file_count)
{
    mpack_tree_t tree;
    mpack_node_t root, cur, file_root, file_cur;
    struct pkt_file_info* ptr;
    size_t i;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);

    cur = mpack_node_map_cstr(root, GAME);
    *game = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, USER);
    *user = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, FILE_LIST);
    *file_count = mpack_node_array_length(cur);
    ptr = calloc(*file_count * sizeof(struct pkt_file_info), 1);
    for (i = 0; i < *file_count; i++) {
        file_root = mpack_node_array_at(cur, i);
        ptr[i].file_name = pkt_parse_str(file_root, FILE_NAME);
        ptr[i].file_key = pkt_parse_str(file_root, FILE_KEY);
        ptr[i].file_hash = pkt_parse_str(file_root, FILE_HASH);
        ptr[i].file_url = pkt_parse_str(file_root, FILE_URL);
        file_cur = mpack_node_map_cstr(file_root, FILE_ID);
        ptr[i].id = mpack_node_u32(file_cur);
        file_cur = mpack_node_map_cstr(file_root, FILE_SIZE);
        ptr[i].file_size = mpack_node_u64(file_cur);
		file_cur = mpack_node_map_cstr(file_root, FILE_PUTTIME);
		ptr[i].puttime = mpack_node_u64(file_cur);
    }

    mpack_tree_destroy(&tree);

    *files = ptr;
    return 0;
}

char* pkt_build_fetch_save_files_ack(uint32_t game, uint32_t user, struct pkt_file_info* files, size_t file_count, size_t* size)
{
    return NULL;
}

void pkt_free_file_info(struct pkt_file_info* files, size_t count)
{
    for (size_t i = 0 ; i < count; i++) {
        free(files[i].file_name);
        free(files[i].file_hash);
        free(files[i].file_key);
        free(files[i].file_url);
    }
    free(files);
}

int pkt_parse_fetch_save_path(const char* data, size_t size, uint32_t* game, uint32_t* user)
{
    return -1;
}

char* pkt_build_fetch_save_path(uint32_t game, uint32_t user, size_t* size)
{
    char* data;
    *size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 2);

    mpack_write_cstr(&writer, GAME);
    mpack_write_u32(&writer, game);
    mpack_write_cstr(&writer, USER);
    mpack_write_u32(&writer, user);

    mpack_finish_map(&writer);
    mpack_writer_destroy(&writer);

    return data;
}

int pkt_parse_fetch_save_path_ack(const char* data, size_t size, uint32_t* game, uint32_t* result, char** sp_root, char** sp_dir, char** sp_pattern)
{
    mpack_tree_t tree;
    mpack_node_t root, cur;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);

    cur = mpack_node_map_cstr(root, GAME);
    *game = mpack_node_u32(cur);
    cur = mpack_node_map_cstr(root, RESULT);
    *result = mpack_node_u32(cur);
    *sp_root = pkt_parse_str(root, SAVEPATH_ROOT);
    *sp_dir = pkt_parse_str(root, SAVEPATH_DIRECTORY);
    *sp_pattern = pkt_parse_str(root, SAVEPATH_PATTERN);
    return 0;
}

char* pkt_build_fetch_save_path_ack(uint32_t game, uint32_t result, const char* root, const char* directory, const char* pattern, size_t* size)
{
    return NULL;
}

int pkt_parse_set_stat_int(const char* data, size_t size, uint32_t* game, uint32_t* user, char** stat, int32_t* value)
{
    return -1;
}

char* pkt_build_set_stat_int(uint32_t game, uint32_t user, const char* stat, int32_t value, size_t* size)
{
    char* data;
    *size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 4);

    mpack_write_cstr(&writer, GAME);
    mpack_write_u32(&writer, game);
    mpack_write_cstr(&writer, USER);
    mpack_write_u32(&writer, user);
    mpack_write_cstr(&writer, STAT);
    mpack_write_str(&writer, stat, strlen(stat));
    mpack_write_cstr(&writer, STAT_VALUE);
    mpack_write_u32(&writer, value);

    mpack_finish_map(&writer);
    mpack_writer_destroy(&writer);

    return data;
}

int pkt_parse_set_stat_int_ack(const char* data, size_t size, uint32_t* game, uint32_t* result, char** stat, int32_t* value)
{
    mpack_tree_t tree;
    mpack_node_t root, cur;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);

    cur = mpack_node_map_cstr(root, GAME);
    *game = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, RESULT);
    *result = mpack_node_u32(cur);

    *stat= pkt_parse_str(root, STAT);

    cur = mpack_node_map_cstr(root, STAT_VALUE);
    *value = mpack_node_i32(cur);

    return 0;
}

char* pkt_build_set_stat_int_ack(uint32_t game, uint32_t result, const char* stat, int32_t value, size_t* size)
{
    return NULL;
}

int pkt_parse_item_op(const char* data, size_t size, uint32_t* game, uint32_t* user, uint32_t* op, struct pkt_item** items, uint32_t* item_count)
{
    return -1;
}

char* pkt_build_item_op(uint32_t game, uint32_t user, uint32_t op, struct pkt_item* items, uint32_t item_count, size_t* size)
{
    char* data;
    uint32_t i;
    *size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 4);

    mpack_write_cstr(&writer, GAME);
    mpack_write_u32(&writer, game);
    mpack_write_cstr(&writer, USER);
    mpack_write_u32(&writer, user);
    mpack_write_cstr(&writer, ITEM_OPER);
    mpack_write_u32(&writer, op);

    mpack_write_cstr(&writer, ITEM_LIST);
    mpack_start_array(&writer, item_count);
    for (i = 0; i < item_count; i++) {
        mpack_start_map(&writer, 4);
        mpack_write_cstr(&writer, ITEM_NAME);
        mpack_write_str(&writer, items[i].name, strlen(items[i].name));
        mpack_write_cstr(&writer, ITEM_PRICE);
        mpack_write_u32(&writer, items[i].price);
        mpack_write_cstr(&writer, ITEM_COUNT);
        mpack_write_u32(&writer, items[i].count);
        mpack_write_cstr(&writer, ORDER_ID);
        mpack_write_u32(&writer, items[i].order);

        mpack_finish_map(&writer);
    }
    mpack_finish_array(&writer);

    mpack_finish_map(&writer);
    mpack_writer_destroy(&writer);

    return data;
}

int pkt_parse_item_op_ack(const char* data, size_t size, uint32_t* result, uint32_t* game, uint32_t* user, uint32_t* op, struct pkt_item** items, uint32_t* item_count)
{
    mpack_tree_t tree;
    mpack_node_t root, cur, item_root, item_cur;
    struct pkt_item* ptr;
    size_t i;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);

    cur = mpack_node_map_cstr(root, GAME);
    *game = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, USER);
    *user = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, RESULT);
    *result = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, ITEM_OPER);
    *op = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, ITEM_LIST);
    *item_count = mpack_node_array_length(cur);
    ptr = calloc(1, *item_count * sizeof(struct pkt_item));
    for (i = 0; i < *item_count; i++) {
        char* name = NULL;
        item_root = mpack_node_array_at(cur, i);
        name = pkt_parse_str(item_root, ITEM_NAME);
        strncpy(ptr[i].name, name, SK_ITEM_MAX);
        free(name);
        item_cur = mpack_node_map_cstr(item_root, ITEM_PRICE);
        ptr[i].price = mpack_node_u32(item_cur);
        item_cur = mpack_node_map_cstr(item_root, ITEM_COUNT);
        ptr[i].count = mpack_node_u32(item_cur);
        item_cur = mpack_node_map_cstr(item_root, ORDER_ID);
        ptr[i].order = mpack_node_u32(item_cur);
    }

    mpack_tree_destroy(&tree);
    *items = ptr;

    return 0;
}

char* pkt_build_item_op_ack(uint32_t result, uint32_t game, uint32_t user, uint32_t op, struct pkt_item* items, uint32_t item_count, size_t* size)
{
    return NULL;
}

int pkt_parse_order_op(const char* data, size_t size, uint32_t* game, uint32_t* user, uint32_t* order, uint32_t* op, char** token)
{
    return -1;
}

char* pkt_build_order_op(uint32_t game, uint32_t user, uint32_t order, uint32_t op, const char* token, size_t* size)
{
    char* data;
    *size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, size);

    mpack_start_map(&writer, 5);

    mpack_write_cstr(&writer, GAME);
    mpack_write_u32(&writer, game);
    mpack_write_cstr(&writer, USER);
    mpack_write_u32(&writer, user);
    mpack_write_cstr(&writer, ORDER_OPER);
    mpack_write_u32(&writer, op);
    mpack_write_cstr(&writer, ORDER_ID);
    mpack_write_u32(&writer, order);
    mpack_write_cstr(&writer, ORDER_TOKEN);
    mpack_write_str(&writer, token, strlen(token));

    mpack_finish_map(&writer);
    mpack_writer_destroy(&writer);

    return data;
}

int pkt_parse_order_op_ack(const char* data, size_t size, uint32_t* game, uint32_t* user, uint32_t* result, uint32_t* order, uint32_t* op)
{
    mpack_tree_t tree;
    mpack_node_t root, cur;

    mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
    root = mpack_tree_root(&tree);

    cur = mpack_node_map_cstr(root, GAME);
    *game = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, USER);
    *user = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, RESULT);
    *result = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, ORDER_ID);
    *order = mpack_node_u32(cur);

    cur = mpack_node_map_cstr(root, ORDER_OPER);
    *op = mpack_node_u32(cur);

    mpack_tree_destroy(&tree);
    return 0;
}

char* pkt_build_order_op_ack(uint32_t game, uint32_t user, uint32_t result, uint32_t order, uint32_t op, size_t* size)
{
    return NULL;
}
void pkt_parse_get_pubkey_ack(const char* data, size_t size, char** pubkey,uint32_t* version)
{
	mpack_tree_t tree;
	mpack_node_t root , cur;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);
	*pubkey = pkt_parse_str(root, PUBLIC_KEY);
	cur = mpack_node_map_cstr(root, PUBKEY_VER);
	*version  = mpack_node_u32(cur);
	mpack_tree_destroy(&tree);
}
char* pkt_build_send_key( const char* key, size_t* size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);

	mpack_start_map(&writer, 1);
	mpack_write_cstr(&writer, PRIVATE_KEY);
	mpack_write_str(&writer, key, strlen(key));

	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);

	return data;
}
char* pkt_build_heartbeat(uint32_t user, uint32_t* games, uint32_t*gamesarea, char**appid, int infosize, const char* region,const char* token, size_t* size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);

	mpack_start_map(&writer, 4);
	mpack_write_cstr(&writer, USER);
	mpack_write_u32(&writer, user);
	mpack_write_cstr(&writer, TOKEN);
	mpack_write_str(&writer, token, strlen(token));
	mpack_write_cstr(&writer, ACCOUNT_REGION);
	mpack_write_str(&writer, region, strlen(region));
	mpack_write_cstr(&writer, GAME_LIST);
	mpack_start_array(&writer, infosize);
	for (int i = 0; i < infosize; i++) {
		mpack_start_map(&writer, 3);
		mpack_write_cstr(&writer, GAME);
		mpack_write_u32(&writer, games[i]);
		mpack_write_cstr(&writer, LOCATION);
		mpack_write_u32(&writer, gamesarea[i]);
		mpack_write_cstr(&writer, APPID);
		mpack_write_str(&writer, appid[i], strlen(appid[i]));
		mpack_finish_map(&writer);
	}
	mpack_finish_array(&writer);



	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);

	return data;
}
char* pkt_build_checkgameid(uint32_t user, uint32_t* game, int gamesize, const char* token, uint32_t location, size_t* size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);

	mpack_start_map(&writer, 4);
	mpack_write_cstr(&writer, USER);
	mpack_write_u32(&writer, user);
	mpack_write_cstr(&writer, LOCATION);
	mpack_write_u32(&writer, location);
	mpack_write_cstr(&writer, TOKEN);
	mpack_write_str(&writer, token, strlen(token));
	mpack_write_cstr(&writer, GAME_LIST);
	mpack_start_array(&writer, gamesize);
	for (int i = 0; i < gamesize; i++) {
		mpack_start_map(&writer, 1);
		mpack_write_cstr(&writer, GAME);
		mpack_write_u32(&writer, game[i]);

		mpack_finish_map(&writer);
	}
	mpack_finish_array(&writer);



	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);

	return data;
}

void pkt_parse_checkgameidack(const char* data, size_t size, uint32_t* result, uint32_t* games, int* item_count, struct pkt_data_item** dlcid)
{
	mpack_tree_t tree;
	mpack_node_t root, cur, item_root, item_cur;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);
	cur = mpack_node_map_cstr(root, RESULT);
	*result = mpack_node_u32(cur);


	cur = mpack_node_map_cstr(root, GAME_DLC);
	*item_count = mpack_node_array_length(cur);
	//	uint32_t * pint = calloc(1, *item_count * sizeof(uint32_t));
	if (*item_count > 0)
	{
		struct pkt_data_item* ptr = calloc(*item_count * sizeof(struct pkt_data_item), 1);
		const char* str;
		for (int i = 0; i < *item_count; i++) {
			item_root = mpack_node_array_at(cur, i);

			item_cur = mpack_node_map_cstr(item_root, DLC);
			uint32_t strlen = mpack_node_strlen(item_cur);
			str = mpack_node_str(item_cur);

			ptr[i].data = calloc(strlen, 1);
			memcpy(ptr[i].data, str, strlen);
			ptr[i].len = strlen;
		}
		*dlcid = ptr;
	}
	cur = mpack_node_map_cstr(root, INVALID_GAMES);
	*games = mpack_node_u32(cur);


	mpack_tree_destroy(&tree);
}

void pkt_parse_heartbeat(const char* data, size_t size, uint32_t* result, uint32_t**games, int*item_count)
{
	mpack_tree_t tree;
	mpack_node_t root, cur, item_root, item_cur;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);
	cur = mpack_node_map_cstr(root, RESULT);
	*result = mpack_node_u32(cur);
	

	cur = mpack_node_map_cstr(root, INVALID_GAMES);
	*item_count = mpack_node_array_length(cur);
	uint32_t * pint = calloc(1,*item_count * sizeof(uint32_t));

	for (int i = 0; i < *item_count; i++) {
		item_root = mpack_node_array_at(cur, i);

		item_cur = mpack_node_map_cstr(item_root, GAME);
		pint[i] = mpack_node_u32(item_cur);
	}
	*games = pint;
	mpack_tree_destroy(&tree);
}
char* pkt_build_pubkey_version(size_t* size,uint32_t version)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);

	mpack_start_map(&writer, 1);
	mpack_write_cstr(&writer, PUBKEY_VER);
	mpack_write_u32(&writer, version);

	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);

	return data;
}

char* pkt_build_get_gamelist(uint32_t user, const char* token, uint32_t location, size_t* size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);

	mpack_start_map(&writer, 3);
	mpack_write_cstr(&writer, USER);
	mpack_write_u32(&writer, user);
	mpack_write_cstr(&writer, LOCATION);
	mpack_write_u32(&writer, location);
	mpack_write_cstr(&writer, TOKEN);
	mpack_write_str(&writer, token, strlen(token));
	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);

	return data;
}
void pkt_parse_get_gamelist_ack(const char* data, size_t size, uint32_t* result, size_t* game_count, char*** const status, uint32_t** const gameid, uint32_t** const install_type, char*** const key_type, char*** const sku_ename, char*** const appid)
{
	mpack_tree_t tree;
	mpack_node_t root, cur, game_root, game_cur ,gameinfo_root,productinfo_root;
	const char* tempstr;
	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);
	cur = mpack_node_map_cstr(root, RESULT);
	*result = mpack_node_u32(cur);
	cur = mpack_node_map_cstr(root, GAME_LIST);
	*game_count = mpack_node_array_length(cur);

	*status = malloc(*game_count * sizeof(char*));
	*gameid= malloc(*game_count * sizeof(uint32_t));
	*install_type = malloc(*game_count * sizeof(uint32_t));
	*key_type = malloc(*game_count * sizeof(char*)); 
	*sku_ename = malloc(*game_count * sizeof(char*));
	*appid = malloc(*game_count * sizeof(char*));
	for (int i = 0; i < *game_count; i++) {
		game_root = mpack_node_array_at(cur, i);
		game_cur = mpack_node_map_cstr(game_root, STATUS);
		size_t strlen = mpack_node_strlen(game_cur);
		(* status)[i] = calloc(strlen + 1, sizeof(char));
		tempstr = mpack_node_str(game_cur);
		memcpy(( * status)[i], tempstr, strlen);

		gameinfo_root = mpack_node_map_cstr(game_root, GAME);
		game_cur = mpack_node_map_cstr(gameinfo_root, KEY_TYPE);
		strlen = mpack_node_strlen(game_cur);
		(* key_type)[i] = calloc(strlen + 1, sizeof(char));
		tempstr = mpack_node_str(game_cur);
		memcpy((* key_type)[i], tempstr, strlen);

		game_cur = mpack_node_map_cstr(gameinfo_root, SKU_ENAME);
		strlen = mpack_node_strlen(game_cur);
		(* sku_ename)[i] = calloc(strlen + 1, sizeof(char));
		tempstr = mpack_node_str(game_cur);
		memcpy((* sku_ename)[i], tempstr, strlen);

		game_cur = mpack_node_map_cstr(gameinfo_root, IDENTIFY_ID);
		(* gameid)[i] = mpack_node_u32(game_cur);

		productinfo_root = mpack_node_map_cstr(gameinfo_root, PRODUCT);
		game_cur = mpack_node_map_cstr(productinfo_root, APPID);
		strlen = mpack_node_strlen(game_cur);
		(* appid)[i] = calloc(strlen + 1, sizeof(char));
		tempstr = mpack_node_str(game_cur);
		memcpy((* appid)[i], tempstr, strlen);

		game_cur = mpack_node_map_cstr(productinfo_root, INSTALL_TYPE);
		(* install_type)[i] = mpack_node_u32(game_cur);
	}

	mpack_tree_destroy(&tree);
}
char* pkt_build_send_hash(const char* hash, const char* token, uint32_t user, size_t* size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);

	mpack_start_map(&writer, 3);
	mpack_write_cstr(&writer, FILE_HASH);
	mpack_write_str(&writer, hash, strlen(hash));
	mpack_write_cstr(&writer, TOKEN);
	mpack_write_str(&writer, token, strlen(token));
	mpack_write_cstr(&writer, USER);
	mpack_write_u32(&writer, user);

	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);

	return data;
}
void pkt_parse_get_sendhash_ack(const char* data, size_t size, uint32_t* gameid , uint32_t *product, uint32_t* result,uint32_t *location ,char **appid,char ** hash,char**dlcs)
{
	mpack_tree_t tree;
	mpack_node_t root, cur;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);
	cur = mpack_node_map_cstr(root, GAME);
	*gameid = mpack_node_u32(cur);
	cur = mpack_node_map_cstr(root, RESULT);
	*result = mpack_node_u32(cur);
	cur = mpack_node_map_cstr(root, PRODUCT);
	*product = mpack_node_u32(cur);
	cur = mpack_node_map_cstr(root, LOCATION);
	*location = mpack_node_u32(cur);
	*appid = pkt_parse_str(root, APPID);
	*hash = pkt_parse_str(root, FILE_HASH);
	*dlcs = pkt_parse_str(root, GAME_DLC);


	mpack_tree_destroy(&tree);
}
char* pkt_build_adult_set(uint32_t adult,uint32_t time, size_t* size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);

	mpack_start_map(&writer, 2);

	mpack_write_cstr(&writer, ADULT);
	mpack_write_u32(&writer, adult);

	mpack_write_cstr(&writer, WORKINGTIME);
	mpack_write_u32(&writer, time);

	mpack_finish_map(&writer);

	mpack_writer_destroy(&writer);
	return data;
}
int pkt_parse_adult_set(const char* data, size_t size, uint32_t* adult,uint32_t *time)
{
	mpack_tree_t tree;
	mpack_node_t root, cur;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);
	cur = mpack_node_map_cstr(root, ADULT);
	*adult = mpack_node_u32(cur);

	cur = mpack_node_map_cstr(root, WORKINGTIME);
	*time = mpack_node_u32(cur);

	mpack_tree_destroy(&tree);

	return 0;
}
void pkt_parse_start_game_ack(const char* data, size_t size, uint32_t* gameid, uint32_t* time)
{
	mpack_tree_t tree;
	mpack_node_t root, cur;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);
	cur = mpack_node_map_cstr(root, GAME);
	*gameid = mpack_node_u32(cur);
	cur = mpack_node_map_cstr(root, LASTTIME);
	*time = mpack_node_u32(cur);
	mpack_tree_destroy(&tree);
}

int pkt_parse_login_access(const char* data, size_t size, char** user, uint32_t* game, char** token)
{
	mpack_tree_t tree;
	mpack_node_t root, cur;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);

	*user = pkt_parse_str(root, USER);

	cur = mpack_node_map_cstr(root, GAME);
	*game = mpack_node_u32(cur);

	*token = pkt_parse_str(root, REFRESH_TOKEN);

	mpack_tree_destroy(&tree);
	return 0;
}

char* pkt_build_login_access(const char* user, uint32_t game, const char* token, size_t* size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);

	mpack_start_map(&writer, 3);

	mpack_write_cstr(&writer, USER);
	mpack_write_str(&writer, user, strlen(user));

	mpack_write_cstr(&writer, GAME);
	mpack_write_u32(&writer, game);

	mpack_write_cstr(&writer, REFRESH_TOKEN);
	mpack_write_str(&writer, token, strlen(token));

	mpack_finish_map(&writer);

	mpack_writer_destroy(&writer);
	return data;
}

int pkt_parse_login_access_ack(const char* data, size_t size, char** user, uint32_t* game, uint32_t* result, char** token)
{
	mpack_tree_t tree;
	mpack_node_t root, cur;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);

	*user = pkt_parse_str(root, USER);

	cur = mpack_node_map_cstr(root, GAME);
	*game = mpack_node_u32(cur);

	cur = mpack_node_map_cstr(root, RESULT);
	*result = mpack_node_u32(cur);

	*token = pkt_parse_str(root, PERMERNENT_TOKEN);

	mpack_tree_destroy(&tree);
	return 0;
}

char* pkt_build_login_access_ack(const char* user, uint32_t game, uint32_t result, const char* token, size_t* size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);

	mpack_start_map(&writer, 4);

	mpack_write_cstr(&writer, USER);
	mpack_write_str(&writer, user, strlen(user));

	mpack_write_cstr(&writer, GAME);
	mpack_write_u32(&writer, game);

	mpack_write_cstr(&writer, RESULT);
	mpack_write_u32(&writer, result);

	mpack_write_cstr(&writer, PERMERNENT_TOKEN);
	mpack_write_str(&writer, token, strlen(token));

	mpack_finish_map(&writer);

	mpack_writer_destroy(&writer);
	return data;
}


//int pkt_parse_check_game(const char* data, size_t size, char** user, char** user_name, char** user_avatar, uint32_t* game, uint32_t* product, char** token, uint32_t* reserved1, char** data1)
//{
//	mpack_tree_t tree;
//	mpack_node_t root, cur;
//
//	mpack_tree_init(&tree, data, size);
//	mpack_tree_parse(&tree);
//	root = mpack_tree_root(&tree);
//
//	*user = pkt_parse_str(root, USER);
//	*user_name = pkt_parse_str(root, K_USER_NAME);
//	*user_avatar = pkt_parse_str(root, K_USER_AVATAR);
//
//	cur = mpack_node_map_cstr(root, GAME);
//	*game = mpack_node_u32(cur);
//
//	cur = mpack_node_map_cstr(root, K_PRODUCT_ID);
//	*product = mpack_node_u32(cur);
//
//	cur = mpack_node_map_cstr(root, K_RESERVED1);
//	*reserved1 = mpack_node_u32(cur);
//
//	*token = pkt_parse_str(root, REFRESH_TOKEN);
//	*data1 = pkt_parse_str(root, "data1");
//
//	mpack_tree_destroy(&tree);
//	return 0;
//}
//
//char* pkt_build_check_game(const char* user, const char* user_name, const char* user_avatar, uint32_t game, uint32_t product, const char* token, uint32_t reserved1, const char* data1, size_t* size)
//{
//	char* data;
//	*size = 0;
//	mpack_writer_t writer;
//	mpack_writer_init_growable(&writer, &data, size);
//
//	mpack_start_map(&writer, 8);
//
//	mpack_write_cstr(&writer, USER);
//	mpack_write_str(&writer, user, strlen(user));
//
//	mpack_write_cstr(&writer, K_USER_NAME);
//	mpack_write_str(&writer, user_name, strlen(user_name));
//
//	mpack_write_cstr(&writer, K_USER_AVATAR);
//	mpack_write_str(&writer, user_avatar, strlen(user_avatar));
//
//	mpack_write_cstr(&writer, GAME);
//	mpack_write_u32(&writer, game);
//
//	mpack_write_cstr(&writer, K_PRODUCT_ID);
//	mpack_write_u32(&writer, product);
//
//	mpack_write_cstr(&writer, K_RESERVED1);
//	mpack_write_u32(&writer, reserved1);
//
//	mpack_write_cstr(&writer, REFRESH_TOKEN);
//	mpack_write_str(&writer, token, strlen(token));
//
//	mpack_write_cstr(&writer, "data1");
//	mpack_write_str(&writer, data1, strlen(data1));
//
//	mpack_finish_map(&writer);
//
//	mpack_writer_destroy(&writer);
//	return data;
//}
//
//int pkt_parse_check_game_ack(const char* data, size_t size, char** user, uint32_t* game, uint32_t* product, uint32_t* result, char** token)
//{
//	mpack_tree_t tree;
//	mpack_node_t root, cur;
//
//	mpack_tree_init(&tree, data, size);
//	mpack_tree_parse(&tree);
//	root = mpack_tree_root(&tree);
//
//	*user = pkt_parse_str(root, USER);
//
//	cur = mpack_node_map_cstr(root, GAME);
//	*game = mpack_node_u32(cur);
//
//	cur = mpack_node_map_cstr(root, K_PRODUCT_ID);
//	*product = mpack_node_u32(cur);
//
//	cur = mpack_node_map_cstr(root, RESULT);
//	*result = mpack_node_u32(cur);
//
//	*token = pkt_parse_str(root, PERMERNENT_TOKEN);
//
//	mpack_tree_destroy(&tree);
//	return 0;
//}
//
//char* pkt_build_check_game_ack(const char* user, uint32_t game, uint32_t product, uint32_t result, const char* token, size_t* size)
//{
//	char* data;
//	*size = 0;
//	mpack_writer_t writer;
//	mpack_writer_init_growable(&writer, &data, size);
//
//	mpack_start_map(&writer, 5);
//
//	mpack_write_cstr(&writer, USER);
//	mpack_write_str(&writer, user, strlen(user));
//
//	mpack_write_cstr(&writer, GAME);
//	mpack_write_u32(&writer, game);
//
//	mpack_write_cstr(&writer, K_PRODUCT_ID);
//	mpack_write_u32(&writer, product);
//
//	mpack_write_cstr(&writer, RESULT);
//	mpack_write_u32(&writer, result);
//
//	mpack_write_cstr(&writer, PERMERNENT_TOKEN);
//	mpack_write_str(&writer, token, strlen(token));
//
//	mpack_finish_map(&writer);
//
//	mpack_writer_destroy(&writer);
//	return data;
//}

int pkt_parse_kv(const char* data, size_t size, char** key, char** value)
{
	mpack_tree_t tree;
	mpack_node_t root;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);

	*key = pkt_parse_str(root, K_KEY);
	*value = pkt_parse_str(root, K_VALUE);

	mpack_tree_destroy(&tree);

	return 0;
}
char* pkt_build_kv(const char* key, const char* value, size_t* size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);

	mpack_start_map(&writer, 2);

	mpack_write_cstr(&writer, K_KEY);
	mpack_write_str(&writer, key, strlen(key));
	mpack_write_cstr(&writer, K_VALUE);
	mpack_write_str(&writer, value, strlen(value));
	
	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);

	return data;
}

int pkt_parse_kv_x(const char* data, size_t size, char** key, uint32_t* type, uint32_t* ivalue, char** value)
{
	mpack_tree_t tree;
	mpack_node_t root;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);

	*key = pkt_parse_str(root, K_KEY);

	mpack_node_t cur = mpack_node_map_cstr(root, K_TYPE);
	*type = mpack_node_u32(cur);
	if (*type == PVT_INT)
	{
		cur = mpack_node_map_cstr(root, K_IVALUE);
		*ivalue = mpack_node_u32(cur);
	}
	else
		*value = pkt_parse_str(root, K_VALUE);

	mpack_tree_destroy(&tree);

	return 0;
}
char* pkt_build_kv_x(const char* key, const char* value, uint32_t type, uint32_t ivalue, size_t* size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);

	mpack_start_map(&writer, 3);

	mpack_write_cstr(&writer, K_KEY);
	mpack_write_str(&writer, key, strlen(key));

	mpack_write_cstr(&writer, K_TYPE);
	mpack_write_u32(&writer, type);

	if (type == PVT_INT)
	{
		mpack_write_cstr(&writer, K_IVALUE);
		mpack_write_u32(&writer, ivalue);
	}
	else
	{
		mpack_write_cstr(&writer, K_VALUE);
		mpack_write_str(&writer, value, strlen(value));
	}

	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);

	return data;
}

int pkt_parse_mm_req_s(const char* data, size_t size, uint32_t* op, uint32_t* sub_op, char** user, uint32_t* product, uint32_t* game, struct kv_list_x* kv_para_list, struct kv_list_x* kv_data_list)
{
	mpack_tree_t tree;
	mpack_node_t root, cur, item_root;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);

	cur = mpack_node_map_cstr(root, K_PARA_OP);
	*op = mpack_node_u32(cur);
	cur = mpack_node_map_cstr(root, K_PARA_SUB_OP);
	*sub_op = mpack_node_u32(cur);

	*user = pkt_parse_str(root, USER);

	cur = mpack_node_map_cstr(root, K_PRODUCT_ID);
	*product = mpack_node_u32(cur);
	cur = mpack_node_map_cstr(root, K_GAME_ID);
	*game = mpack_node_u32(cur);

	cur = mpack_node_map_cstr(root, K_PARA_LIST);
	size_t item_size = mpack_node_array_length(cur);
	if (item_size > 0)
	{
		kv_para_list->kv_count = item_size;
		kv_para_list->kv_data = calloc(item_size * sizeof(struct pkt_kv_data_x), 1);
		size_t i;
		for (i = 0; i < item_size; i++)
		{
			item_root = mpack_node_array_at(cur, i);
			kv_para_list->kv_data[i].key = pkt_parse_str(item_root, K_KEY);

			mpack_node_t sub_cur = mpack_node_map_cstr(item_root, K_TYPE);
			kv_para_list->kv_data[i].type = mpack_node_u32(sub_cur);
			if (kv_para_list->kv_data[i].type == PVT_INT)
			{
				sub_cur = mpack_node_map_cstr(item_root, K_IVALUE);
				kv_para_list->kv_data[i].ivalue = mpack_node_u32(sub_cur);
			}
			else
				kv_para_list->kv_data[i].value = pkt_parse_str(item_root, K_VALUE);
		}
	}
	else
	{
		kv_para_list->kv_data = NULL;
		kv_para_list->kv_count = 0;
	}

	cur = mpack_node_map_cstr(root, K_DATA_LIST);
	item_size = mpack_node_array_length(cur);
	if (item_size > 0)
	{
		kv_data_list->kv_count = item_size;
		kv_data_list->kv_data = calloc(item_size * sizeof(struct pkt_kv_data_x), 1);
		size_t i;
		for (i = 0; i < item_size; i++)
		{
			item_root = mpack_node_array_at(cur, i);
			kv_data_list->kv_data[i].key = pkt_parse_str(item_root, K_KEY);

			mpack_node_t sub_cur = mpack_node_map_cstr(item_root, K_TYPE);
			kv_data_list->kv_data[i].type = mpack_node_u32(sub_cur);
			if (kv_data_list->kv_data[i].type == PVT_INT)
			{
				sub_cur = mpack_node_map_cstr(item_root, K_IVALUE);
				kv_data_list->kv_data[i].ivalue = mpack_node_u32(sub_cur);
			}
			else
				kv_data_list->kv_data[i].value = pkt_parse_str(item_root, K_VALUE);
		}
	}
	else
	{
		kv_data_list->kv_data = NULL;
		kv_data_list->kv_count = 0;
	}

	mpack_tree_destroy(&tree);
	return 0;
}
char* pkt_build_mm_req_s(uint32_t op, uint32_t sub_op, const char* user, uint32_t product, uint32_t game, struct kv_list_x* kv_para_list, struct kv_list_x* kv_data_list, size_t* size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);

	mpack_start_map(&writer, 7);

	mpack_write_cstr(&writer, K_PARA_OP);
	mpack_write_u32(&writer, op);
	mpack_write_cstr(&writer, K_PARA_SUB_OP);
	mpack_write_u32(&writer, sub_op);

	mpack_write_cstr(&writer, USER);
	mpack_write_str(&writer, user, strlen(user));

	mpack_write_cstr(&writer, K_PRODUCT_ID);
	mpack_write_u32(&writer, product);
	mpack_write_cstr(&writer, K_GAME_ID);
	mpack_write_u32(&writer, game);

	if (kv_para_list && kv_para_list->kv_data)
	{
		size_t i;
		mpack_write_cstr(&writer, K_PARA_LIST);
		mpack_start_array(&writer, kv_para_list->kv_count);
		for (i = 0; i < kv_para_list->kv_count; i++) {
			mpack_start_map(&writer, 3);
			mpack_write_cstr(&writer, K_KEY);
			mpack_write_str(&writer, kv_para_list->kv_data[i].key, strlen(kv_para_list->kv_data[i].key));

			mpack_write_cstr(&writer, K_TYPE);
			mpack_write_u32(&writer, kv_para_list->kv_data[i].type);

			if (kv_para_list->kv_data[i].type == PVT_INT)
			{
				mpack_write_cstr(&writer, K_IVALUE);
				mpack_write_u32(&writer, kv_para_list->kv_data[i].ivalue);
			}
			else
			{
				mpack_write_cstr(&writer, K_VALUE);
				mpack_write_str(&writer, kv_para_list->kv_data[i].value, strlen(kv_para_list->kv_data[i].value));
			}
			mpack_finish_map(&writer);
		}
		mpack_finish_array(&writer);
	}
	else
	{
		mpack_write_cstr(&writer, K_PARA_LIST);
		mpack_start_array(&writer, 0);
		mpack_finish_array(&writer);
	}

	if (kv_data_list && kv_data_list->kv_data)
	{
		size_t i;
		mpack_write_cstr(&writer, K_DATA_LIST);
		mpack_start_array(&writer, kv_data_list->kv_count);
		for (i = 0; i < kv_data_list->kv_count; i++) {
			mpack_start_map(&writer, 3);
			mpack_write_cstr(&writer, K_KEY);
			mpack_write_str(&writer, kv_data_list->kv_data[i].key, strlen(kv_data_list->kv_data[i].key));

			mpack_write_cstr(&writer, K_TYPE);
			mpack_write_u32(&writer, kv_data_list->kv_data[i].type);

			if (kv_data_list->kv_data[i].type == PVT_INT)
			{
				mpack_write_cstr(&writer, K_IVALUE);
				mpack_write_u32(&writer, kv_data_list->kv_data[i].ivalue);
			}
			else
			{
				mpack_write_cstr(&writer, K_VALUE);
				mpack_write_str(&writer, kv_data_list->kv_data[i].value, strlen(kv_data_list->kv_data[i].value));
			}
			mpack_finish_map(&writer);
		}
		mpack_finish_array(&writer);
	}
	else
	{
		mpack_write_cstr(&writer, K_DATA_LIST);
		mpack_start_array(&writer, 0);
		mpack_finish_array(&writer);
	}

	mpack_finish_map(&writer);

	mpack_writer_destroy(&writer);
	return data;
}

int pkt_parse_mm_resp_s(const char* data, size_t size, uint32_t* op, uint32_t* sub_op, char** user, uint32_t* product, uint32_t* game, struct kv_list_x* kv_para_list, struct kv_list_x* kv_data_list, struct region_data_list* region_list, struct pkt_data_item** users_data, uint32_t* user_count)
{
	mpack_tree_t tree;
	mpack_node_t root, cur, item_root, item_cur;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);

	cur = mpack_node_map_cstr(root, K_PARA_OP);
	*op = mpack_node_u32(cur);
	cur = mpack_node_map_cstr(root, K_PARA_SUB_OP);
	*sub_op = mpack_node_u32(cur);

	*user = pkt_parse_str(root, USER);

	cur = mpack_node_map_cstr(root, K_PRODUCT_ID);
	*product = mpack_node_u32(cur);
	cur = mpack_node_map_cstr(root, K_GAME_ID);
	*game = mpack_node_u32(cur);

	size_t str_size, i;
	const char* str;
	cur = mpack_node_map_cstr(root, K_USER_LIST);
	size_t item_size = mpack_node_array_length(cur);
	if (item_size > 0)
	{
		struct pkt_data_item* ptr = calloc(item_size * sizeof(struct pkt_data_item), 1);
		for (i = 0; i < item_size; i++) {
			item_root = mpack_node_array_at(cur, i);
			item_cur = mpack_node_map_cstr(item_root, K_DATA);
			str_size = mpack_node_strlen(item_cur);
			str = mpack_node_str(item_cur);

			ptr[i].data = calloc(1, str_size);
			memcpy(ptr[i].data, str, str_size);
			ptr[i].len = str_size;
		}
		*users_data = ptr;
	}
	else
		*users_data = NULL;
	*user_count = item_size;

	cur = mpack_node_map_cstr(root, K_PARA_LIST);
	item_size = mpack_node_array_length(cur);
	if (item_size > 0)
	{
		kv_para_list->kv_count = item_size;
		kv_para_list->kv_data = calloc(item_size * sizeof(struct pkt_kv_data_x), 1);
		size_t i;
		for (i = 0; i < item_size; i++)
		{
			item_root = mpack_node_array_at(cur, i);
			kv_para_list->kv_data[i].key = pkt_parse_str(item_root, K_KEY);

			mpack_node_t sub_cur = mpack_node_map_cstr(item_root, K_TYPE);
			kv_para_list->kv_data[i].type = mpack_node_u32(sub_cur);
			if (kv_para_list->kv_data[i].type == PVT_INT)
			{
				sub_cur = mpack_node_map_cstr(item_root, K_IVALUE);
				kv_para_list->kv_data[i].ivalue = mpack_node_u32(sub_cur);
			}
			else
				kv_para_list->kv_data[i].value = pkt_parse_str(item_root, K_VALUE);
		}
	}
	else
	{
		kv_para_list->kv_data = NULL;
		kv_para_list->kv_count = 0;
	}

	cur = mpack_node_map_cstr(root, K_DATA_LIST);
	item_size = mpack_node_array_length(cur);
	if (item_size > 0)
	{
		kv_data_list->kv_count = item_size;
		kv_data_list->kv_data = calloc(item_size * sizeof(struct pkt_kv_data_x), 1);
		size_t i;
		for (i = 0; i < item_size; i++)
		{
			item_root = mpack_node_array_at(cur, i);
			kv_data_list->kv_data[i].key = pkt_parse_str(item_root, K_KEY);

			mpack_node_t sub_cur = mpack_node_map_cstr(item_root, K_TYPE);
			kv_data_list->kv_data[i].type = mpack_node_u32(sub_cur);
			if (kv_data_list->kv_data[i].type == PVT_INT)
			{
				sub_cur = mpack_node_map_cstr(item_root, K_IVALUE);
				kv_data_list->kv_data[i].ivalue = mpack_node_u32(sub_cur);
			}
			else
				kv_data_list->kv_data[i].value = pkt_parse_str(item_root, K_VALUE);
		}
	}
	else
	{
		kv_data_list->kv_data = NULL;
		kv_data_list->kv_count = 0;
	}

	cur = mpack_node_map_cstr(root, K_REGION_LIST);
	item_size = mpack_node_array_length(cur);
	if (item_size > 0)
	{
		region_list->count = item_size;
		region_list->r_data = calloc(item_size * sizeof(struct region_data), 1);
		size_t i;
		for (i = 0; i < item_size; i++)
		{
			item_root = mpack_node_array_at(cur, i);
			region_list->r_data[i].name = pkt_parse_str(item_root, K_REGION_NAME);
		}
	}
	else
	{
		region_list->r_data = NULL;
		region_list->count = 0;
	}

	mpack_tree_destroy(&tree);
	return 0;
}
char* pkt_build_mm_resp_s(uint32_t op, uint32_t sub_op, const char* user, uint32_t product, uint32_t game, struct kv_list_x* kv_para_list, struct kv_list_x* kv_data_list, struct region_data_list* region_list, struct pkt_data_item* users_data, uint32_t user_count, size_t* size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);

	mpack_start_map(&writer, 9);

	mpack_write_cstr(&writer, K_PARA_OP);
	mpack_write_u32(&writer, op);
	mpack_write_cstr(&writer, K_PARA_SUB_OP);
	mpack_write_u32(&writer, sub_op);

	mpack_write_cstr(&writer, USER);
	mpack_write_str(&writer, user, strlen(user));

	mpack_write_cstr(&writer, K_PRODUCT_ID);
	mpack_write_u32(&writer, product);
	mpack_write_cstr(&writer, K_GAME_ID);
	mpack_write_u32(&writer, game);

	mpack_write_cstr(&writer, K_USER_LIST);
	mpack_start_array(&writer, user_count);
	size_t i;
	for (i = 0; i < user_count; i++) {
		mpack_start_map(&writer, 1);
		mpack_write_cstr(&writer, K_DATA);
		mpack_write_str(&writer, users_data[i].data, users_data[i].len);
		mpack_finish_map(&writer);
	}
	mpack_finish_array(&writer);

	if (kv_para_list && kv_para_list->kv_data)
	{
		size_t i;
		mpack_write_cstr(&writer, K_PARA_LIST);
		mpack_start_array(&writer, kv_para_list->kv_count);
		for (i = 0; i < kv_para_list->kv_count; i++) {
			mpack_start_map(&writer, 3);
			mpack_write_cstr(&writer, K_KEY);
			mpack_write_str(&writer, kv_para_list->kv_data[i].key, strlen(kv_para_list->kv_data[i].key));

			mpack_write_cstr(&writer, K_TYPE);
			mpack_write_u32(&writer, kv_para_list->kv_data[i].type);

			if (kv_para_list->kv_data[i].type == PVT_INT)
			{
				mpack_write_cstr(&writer, K_IVALUE);
				mpack_write_u32(&writer, kv_para_list->kv_data[i].ivalue);
			}
			else
			{
				mpack_write_cstr(&writer, K_VALUE);
				mpack_write_str(&writer, kv_para_list->kv_data[i].value, strlen(kv_para_list->kv_data[i].value));
			}
			mpack_finish_map(&writer);
		}
		mpack_finish_array(&writer);
	}
	else
	{
		mpack_write_cstr(&writer, K_PARA_LIST);
		mpack_start_array(&writer, 0);
		mpack_finish_array(&writer);
	}

	if (kv_data_list && kv_data_list->kv_data)
	{
		size_t i;
		mpack_write_cstr(&writer, K_DATA_LIST);
		mpack_start_array(&writer, kv_data_list->kv_count);
		for (i = 0; i < kv_data_list->kv_count; i++) {
			mpack_start_map(&writer, 3);
			mpack_write_cstr(&writer, K_KEY);
			mpack_write_str(&writer, kv_data_list->kv_data[i].key, strlen(kv_data_list->kv_data[i].key));

			mpack_write_cstr(&writer, K_TYPE);
			mpack_write_u32(&writer, kv_data_list->kv_data[i].type);

			if (kv_data_list->kv_data[i].type == PVT_INT)
			{
				mpack_write_cstr(&writer, K_IVALUE);
				mpack_write_u32(&writer, kv_data_list->kv_data[i].ivalue);
			}
			else
			{
				mpack_write_cstr(&writer, K_VALUE);
				mpack_write_str(&writer, kv_data_list->kv_data[i].value, strlen(kv_data_list->kv_data[i].value));
			}
			mpack_finish_map(&writer);
		}
		mpack_finish_array(&writer);
	}
	else
	{
		mpack_write_cstr(&writer, K_DATA_LIST);
		mpack_start_array(&writer, 0);
		mpack_finish_array(&writer);
	}

	if (region_list && region_list->r_data)
	{
		size_t i;
		mpack_write_cstr(&writer, K_REGION_LIST);
		mpack_start_array(&writer, region_list->count);
		for (i = 0; i < region_list->count; i++) {
			mpack_start_map(&writer, 1);

			mpack_write_cstr(&writer, K_REGION_NAME);
			mpack_write_str(&writer, region_list->r_data[i].name, strlen(region_list->r_data[i].name));
			mpack_finish_map(&writer);
		}
		mpack_finish_array(&writer);
	}
	else
	{
		mpack_write_cstr(&writer, K_REGION_LIST);
		mpack_start_array(&writer, 0);
		mpack_finish_array(&writer);
	}

	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);
	return data;
}


int pkt_parse_user_data(const char* data, size_t size, uint32_t* bAI, char** user, char** user_name, char** user_avatar, uint32_t* team_type, int32_t* chair, uint32_t* state, struct pkt_data_item** data_list, uint32_t* count)
{
	mpack_tree_t tree;
	mpack_node_t root, cur, item_root, item_cur;
	size_t item_size, i;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);

	cur = mpack_node_map_cstr(root, K_BOOL_AI);
	*bAI = mpack_node_u32(cur);

	cur = mpack_node_map_cstr(root, K_TYPE);
	*team_type = mpack_node_u32(cur);

	cur = mpack_node_map_cstr(root, K_CHAIR);
	*chair = mpack_node_i32(cur);

	cur = mpack_node_map_cstr(root, K_STATE);
	*state = mpack_node_u32(cur);

	*user = pkt_parse_str(root, USER);
	*user_name = pkt_parse_str(root, K_USER_NAME);
	*user_avatar = pkt_parse_str(root, K_USER_AVATAR);

	size_t str_size;
	const char* str;
	cur = mpack_node_map_cstr(root, K_DATA_LIST);
	item_size = mpack_node_array_length(cur);

	if (item_size > 0)
	{
		struct pkt_data_item* ptr = calloc(item_size * sizeof(struct pkt_data_item), 1);
		for (i = 0; i < item_size; i++) {
			item_root = mpack_node_array_at(cur, i);
			item_cur = mpack_node_map_cstr(item_root, K_DATA);
			str_size = mpack_node_strlen(item_cur);
			str = mpack_node_str(item_cur);

			ptr[i].data = calloc(1, str_size);
			memcpy(ptr[i].data, str, str_size);
			ptr[i].len = str_size;
		}
		*data_list = ptr;
	}
	else
		*data_list = NULL;
	*count = item_size;

	mpack_tree_destroy(&tree);

	return 0;
}
char* pkt_build_user_data(uint32_t bAI, const char* user, const char* user_name, const char* user_avatar, uint32_t team_type, int32_t chair, uint32_t state, struct pkt_data_item* data_list, uint32_t count, size_t* size)
{
	char* data;
	*size = 0;
	uint32_t i;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);

	mpack_start_map(&writer, 8);

	mpack_write_cstr(&writer, USER);
	mpack_write_str(&writer, user, strlen(user));
	
	mpack_write_cstr(&writer, K_USER_NAME);
	mpack_write_str(&writer, user_name, strlen(user_name));
	
	mpack_write_cstr(&writer, K_USER_AVATAR);
	mpack_write_str(&writer, user_avatar, strlen(user_avatar));

	mpack_write_cstr(&writer, K_BOOL_AI);
	mpack_write_u32(&writer, bAI);

	mpack_write_cstr(&writer, K_TYPE);
	mpack_write_u32(&writer, team_type);

	mpack_write_cstr(&writer, K_CHAIR);
	mpack_write_i32(&writer, chair);

	mpack_write_cstr(&writer, K_STATE);
	mpack_write_u32(&writer, state);

	mpack_write_cstr(&writer, K_DATA_LIST);
	mpack_start_array(&writer, count);
	for (i = 0; i < count; i++) {
		mpack_start_map(&writer, 1);
		mpack_write_cstr(&writer, K_DATA);
		mpack_write_str(&writer, data_list[i].data, data_list[i].len);
		mpack_finish_map(&writer);
	}
	mpack_finish_array(&writer);

	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);
	return data;
}


char* pkt_build_deletecloudfile(uint32_t userid, uint32_t productid, struct pkt_str_item*filename, uint32_t filecout, size_t* size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);

	mpack_start_map(&writer, 3);
	mpack_write_cstr(&writer, USER);
	mpack_write_u32(&writer,userid);
	mpack_write_cstr(&writer, PRODUCT);
	mpack_write_u32(&writer, productid);
	mpack_write_cstr(&writer, FILELIST);
	mpack_start_array(&writer,filecout);
	for (uint32_t i = 0; i < filecout; i++)
	{
		mpack_start_map(&writer, 1);
		mpack_write_cstr(&writer, FILENAME);
		mpack_write_str(&writer, filename[i].data, filename[i].len);
		mpack_finish_map(&writer);
	}
	mpack_finish_array(&writer);
	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);
	return data;
}
char* pkt_build_getticket(uint32_t userid, uint32_t gameid,const char* appid,size_t *size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);
	mpack_start_map(&writer, 3);
	mpack_write_cstr(&writer, USER);
	mpack_write_u32(&writer, userid);
	mpack_write_cstr(&writer, GAME);
	mpack_write_u32(&writer, gameid);
	mpack_write_cstr(&writer, APPID);
	mpack_write_str(&writer, appid, strlen(appid));

	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);
	return data;
}
void pkt_parse_ticket_ack(const char* data, size_t size, uint32_t *game, char** ticket)
{
	mpack_tree_t tree;
	mpack_node_t root, cur;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);

	cur = mpack_node_map_cstr(root, GAME);
	*game = mpack_node_u32(cur);
	//cur = mpack_node_map_cstr(root, TICKET);
	*ticket = pkt_parse_str(root, TICKET);
	mpack_tree_destroy(&tree);
}
char* pkt_build_ticket_for_game(const char* ticket, size_t* size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);
	mpack_start_map(&writer, 1);
	mpack_write_cstr(&writer, TICKET);
	mpack_write_str(&writer, ticket, strlen(ticket));
	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);
	return data;
}
void pkt_pares_ticket_for_game(const char* data, size_t size, char** ticket)
{
	mpack_tree_t tree;
	mpack_node_t root;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);
	*ticket = pkt_parse_str(root, TICKET);
	mpack_tree_destroy(&tree);
}
char* pkt_build_get_friend_status(uint32_t userid, const char* token, size_t *size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);
	mpack_start_map(&writer, 2);
	mpack_write_cstr(&writer, USER);
	mpack_write_u32(&writer, userid);
	mpack_write_cstr(&writer, TOKEN);
	mpack_write_str(&writer, token, strlen(token));
	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);
	return data;

}
void pkt_parse_update_friend_status(const char* data, size_t size, uint32_t* type, char**onlines, char**offlines)
{
	mpack_tree_t tree;
	mpack_node_t root, cur;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);
	cur = mpack_node_map_cstr(root, ONLINE_OFFLINE_TYPE);
	*type = mpack_node_u32(cur);
	*onlines = pkt_parse_str(root, ONLINES);
	*offlines = pkt_parse_str(root,OFFLINES);
	mpack_tree_destroy(&tree);
}
//void pkt_parse_update_friend_status(const char* data, size_t size, uint32_t* type, uint32_t**onlines, int*online_count ,uint32_t**offlines,int*offline_cout)
//{
//	mpack_tree_t tree;
//	mpack_node_t root, cur, item_root, item_cur;
//
//	mpack_tree_init(&tree, data, size);
// mpack_tree_parse(&tree);
//	root = mpack_tree_root(&tree);
//	cur = mpack_node_map_cstr(root, ONLINE_OFFLINE_TYPE);
//	*type = mpack_node_u32(cur);
//
//
//	cur = mpack_node_map_cstr(root, ONLINES);
//	*online_count = mpack_node_array_length(cur);
//	uint32_t * ponlines = calloc(1, *online_count * sizeof(uint32_t));
//
//	for (int i = 0; i < *online_count; i++) {
//		item_root = mpack_node_array_at(cur, i);
//
//		item_cur = mpack_node_map_cstr(item_root, USER);
//		ponlines[i] = mpack_node_u32(item_cur);
//	}
//	*onlines = ponlines;
//
//	cur = mpack_node_map_cstr(root, OFFLINES);
//	*offline_cout = mpack_node_array_length(cur);
//	uint32_t * pofflines = calloc(1, *offline_cout * sizeof(uint32_t));
//
//	for (int i = 0; i < *offline_cout; i++) {
//		item_root = mpack_node_array_at(cur, i);
//
//		item_cur = mpack_node_map_cstr(item_root, USER);
//		pofflines[i] = mpack_node_u32(item_cur);
//	}
//	*offlines = pofflines;
//
//	mpack_tree_destroy(&tree);
//}
char* pkt_build_game_started_notice(uint32_t game, uint32_t user, const char* name, const char* appid, const char* userarea, uint32_t gamearea, size_t* size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);

	mpack_start_map(&writer, 6);

	mpack_write_cstr(&writer, GAME);
	mpack_write_u32(&writer, game);
	mpack_write_cstr(&writer, USER);
	mpack_write_u32(&writer, user);
	mpack_write_cstr(&writer, LOCATION);
	mpack_write_u32(&writer, gamearea);
	mpack_write_cstr(&writer, GAME_NAME);
	mpack_write_str(&writer, name,strlen(name));
	mpack_write_cstr(&writer, APPID);
	mpack_write_str(&writer, appid, strlen(appid));
	mpack_write_cstr(&writer, ACCOUNT_REGION);
	mpack_write_str(&writer, userarea, strlen(userarea));
	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);

	return data;
}
char* pkt_build_activate_to_sonkwo(const char* sonkwoappid, uint32_t user, const char* location, const char* steamid, const char* steamappid,uint32_t gameid, size_t* size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);
	mpack_start_map(&writer, 6);

	mpack_write_cstr(&writer, STEAM_APPID);
	mpack_write_str(&writer, steamappid, strlen(steamappid));
	mpack_write_cstr(&writer, STEAM_ID);
	mpack_write_str(&writer, steamid, strlen(steamid));
	mpack_write_cstr(&writer, APPID);
	mpack_write_str(&writer, sonkwoappid,strlen(sonkwoappid));
	mpack_write_cstr(&writer, LOCATION);
	mpack_write_str(&writer, location,strlen(location));
	mpack_write_cstr(&writer, USER);
	mpack_write_u32(&writer, user);
	mpack_write_cstr(&writer, GAME);
	mpack_write_u32(&writer, gameid);
	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);
	return data;
}
void pkt_parse_activate_ack(const char* data, size_t size, uint32_t *game, uint32_t* result, char**region)
{
	mpack_tree_t tree;
	mpack_node_t root, cur;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);

	cur = mpack_node_map_cstr(root, GAME);
	*game = mpack_node_u32(cur);
	cur = mpack_node_map_cstr(root, RESULT);
	*result = mpack_node_u32(cur);
	*region = pkt_parse_str(root, ACCOUNT_REGION);
	mpack_tree_destroy(&tree);
}
char* pkt_build_steamappid_to_gameid(const char * steamappid, const char * region,size_t *size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);
	mpack_start_map(&writer, 2);
	mpack_write_cstr(&writer, STEAM_APPID);
	mpack_write_str(&writer, steamappid, strlen(steamappid));
	mpack_write_cstr(&writer, ACCOUNT_REGION);
	mpack_write_str(&writer, region, strlen(region));
	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);
	return data;
}
void pkt_pares_steamappid_to_gameid_ack(const char * data, size_t size, uint32_t* gameid, uint32_t* result,char**region)
{
	mpack_tree_t tree;
	mpack_node_t root, cur;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);
	cur = mpack_node_map_cstr(root, GAME);
	*gameid = mpack_node_u32(cur);
	cur = mpack_node_map_cstr(root, RESULT);
	*result = mpack_node_u32(cur);
	*region = pkt_parse_str(root, ACCOUNT_REGION);
	mpack_tree_destroy(&tree);
}
char* pkt_build_gameid_to_steamappid(uint32_t game, const char * region, size_t *size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);
	mpack_start_map(&writer, 2);
	mpack_write_cstr(&writer, GAME);
	mpack_write_u32(&writer, game);
	mpack_write_cstr(&writer, ACCOUNT_REGION);
	mpack_write_str(&writer, region, strlen(region));
	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);
	return data;
}
void pkt_pares_gameid_tosteamappid_ack(const char * data, size_t size, char**steamappid, uint32_t* gameid,uint32_t* result, char**region)
{
	mpack_tree_t tree;
	mpack_node_t root, cur;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);
	*steamappid = pkt_parse_str(root, STEAM_APPID);
	cur = mpack_node_map_cstr(root, GAME);
	*gameid = mpack_node_u32(cur);
	cur = mpack_node_map_cstr(root, RESULT);
	*result = mpack_node_u32(cur);
	*region = pkt_parse_str(root, ACCOUNT_REGION);
	mpack_tree_destroy(&tree);
}
char* pkt_build_generate_order(uint32_t sku, uint32_t location, const char* token, const char* appid,size_t*size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);
	mpack_start_map(&writer, 4);
	mpack_write_cstr(&writer, SKU);
	mpack_write_u32(&writer, sku);
	mpack_write_cstr(&writer, LOCATION);
	mpack_write_u32(&writer, location);
	mpack_write_cstr(&writer, TOKEN);
	mpack_write_str(&writer, token, strlen(token));
	mpack_write_cstr(&writer, APPID);
	mpack_write_str(&writer, appid, strlen(appid));
	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);
	return data;
}
void pkt_pares_generate_order_ack(const char * data, size_t size,uint32_t * result,  uint32_t*sku,uint32_t *price, \
	char** name, char** appid)
{
	mpack_tree_t tree;
	mpack_node_t root, cur;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);
	cur = mpack_node_map_cstr(root, RESULT);
	*result = mpack_node_u32(cur);
	cur = mpack_node_map_cstr(root, SKU);
	*sku = mpack_node_u32(cur);
	cur = mpack_node_map_cstr(root, PRICE);
	*price = mpack_node_u32(cur);
	*name = pkt_parse_str(root, NAME);
	*appid = pkt_parse_str(root, APPID);
	mpack_tree_destroy(&tree);
}
char* pkt_build_verify_code_request(const char* token, size_t*size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);
	mpack_start_map(&writer, 1);
	mpack_write_cstr(&writer, TOKEN);
	mpack_write_str(&writer, token, strlen(token));
	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);
	return data;
}
void pkt_pares_verify_code_request_ack(const char* data, size_t size, uint32_t* result)
{
	mpack_tree_t tree;
	mpack_node_t root, cur;
	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);
	cur = mpack_node_map_cstr(root, RESULT);
	*result = mpack_node_u32(cur);
	mpack_tree_destroy(&tree);
}
char* pkt_build_confirm_pay_request(const char* token, uint32_t price, const char*messagecode, uint32_t location, uint32_t sku,\
	const char* appid, size_t *size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);
	mpack_start_map(&writer, 6);
	mpack_write_cstr(&writer, PRICE);
	mpack_write_u32(&writer, price);
	mpack_write_cstr(&writer, MESSAGE_CODE);
	mpack_write_str(&writer, messagecode, strlen(messagecode));
	mpack_write_cstr(&writer, LOCATION);
	mpack_write_u32(&writer, location);
	mpack_write_cstr(&writer, TOKEN);
	mpack_write_str(&writer, token, strlen(token));
	mpack_write_cstr(&writer, SKU);
	mpack_write_u32(&writer, sku);
	mpack_write_cstr(&writer, APPID);
	mpack_write_str(&writer, appid, strlen(appid));
	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);
	return data;
}
void pkt_pares_confirm_pay_request_ack(const char* data, size_t size, uint32_t* result,uint32_t* sku,char** appid)
{
	mpack_tree_t tree;
	mpack_node_t root, cur;
	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);
	cur = mpack_node_map_cstr(root, RESULT);
	*result = mpack_node_u32(cur);
	cur = mpack_node_map_cstr(root, SKU);
	*sku = mpack_node_u32(cur);
	*appid = pkt_parse_str(root, APPID);
	mpack_tree_destroy(&tree);
}
char* pkt_build_get_item_response(uint32_t sku,uint32_t result, size_t* size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);
	mpack_start_map(&writer, 2);
	mpack_write_cstr(&writer, SKU);
	mpack_write_u32(&writer, sku);
	mpack_write_cstr(&writer, RESULT);
	mpack_write_u32(&writer, result);
	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);
	return data;
}
void pkt_pares_get_item_response(const char* data, size_t size, uint32_t *sku, uint32_t *result)
{
	mpack_tree_t tree;
	mpack_node_t root, cur;

	mpack_tree_init(&tree, data, size);
	mpack_tree_parse(&tree);
	root = mpack_tree_root(&tree);

	cur = mpack_node_map_cstr(root, SKU);
	*sku = mpack_node_u32(cur);
	cur = mpack_node_map_cstr(root, RESULT);
	*result = mpack_node_u32(cur);
	mpack_tree_destroy(&tree);
}
char* pkt_build_demo_status(uint32_t userid,uint32_t gameid, const char* appid, const char* area, const char* status, size_t* size)
{
	char* data;
	*size = 0;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, size);
	mpack_start_map(&writer, 5);
	mpack_write_cstr(&writer, GAME);
	mpack_write_u32(&writer, gameid);
	mpack_write_cstr(&writer, USER);
	mpack_write_u32(&writer, userid);
	mpack_write_cstr(&writer, APPID);
	mpack_write_str(&writer, appid, strlen(appid));
	mpack_write_cstr(&writer, AREA);
	mpack_write_str(&writer, area, strlen(area));
	mpack_write_cstr(&writer, STATUS);
	mpack_write_str(&writer, status, strlen(status));
	mpack_finish_map(&writer);
	mpack_writer_destroy(&writer);
	return data;
}