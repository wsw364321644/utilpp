#include "HOOK/hotkey_list.h"
#include <stdlib.h>

/// <summary>
/// hot_key_list_header_t
/// hot_key_buf  |__ name_len
///              |__ name_buf
///              |__ key_with_modifier_t
/// ...
/// </summary>
/// <returns></returns>

static const uint32_t BUF_INCREASE_SIZE = 128;

typedef struct hot_key_list_header_t {
    uint64_t buf_size;
    uint64_t used_size;
    uint64_t hot_key_num;
}hot_key_list_header_t;

bool resize_hot_key_list(hot_key_list_header_t** pplist, uint64_t need_size) {
    hot_key_list_header_t*  list = *pplist;
    uint64_t new_size= list->buf_size;
    while (need_size > new_size) {
        new_size += BUF_INCREASE_SIZE;
    }
    if (new_size== list->buf_size) {
        return true;
    }
    *pplist = (hot_key_list_header_t*)malloc(new_size);
    if (*pplist == NULL) {
        *pplist = list;
        return false;
    }
    memcpy(*pplist, list, list->used_size);
    free(list);
    list = *pplist;
    list->buf_size = new_size;
    return true;
}

hot_key_list_handle_t new_hot_key_list()
{
    hot_key_list_header_t* list = (hot_key_list_header_t*)malloc(BUF_INCREASE_SIZE);
    if (!list) {
        return (hot_key_list_handle_t)NULL;
    }
    list->buf_size = BUF_INCREASE_SIZE;
    list->used_size = sizeof(hot_key_list_header_t);
    list->hot_key_num = 0;

    hot_key_list_handle_t handle = (uintptr_t)malloc(sizeof(hot_key_list_header_t*));
    if (!handle) {
        free(list);
        return (hot_key_list_handle_t)NULL;
    }
    *(hot_key_list_header_t ** )handle = list;
    return handle;
}

bool add_hot_key(hot_key_list_handle_t handle, const char* name, key_with_modifier_t key)
{
    if (!handle) {
        return false;
    }
    hot_key_list_header_t* list = *(hot_key_list_header_t ** )handle;
    uint64_t str_len = strlen(name)+1;
    uint64_t increase_size = sizeof(uint64_t) + str_len + sizeof(key_with_modifier_t);
    if (!resize_hot_key_list((hot_key_list_header_t**)handle, list->used_size + increase_size)) {
        return false;
    }
    uint8_t* hot_key_buf = (uint8_t*)list + list->used_size;
    memcpy(hot_key_buf, &str_len, sizeof(uint64_t));
    hot_key_buf += sizeof(uint64_t);
    memcpy(hot_key_buf, name, str_len);
    hot_key_buf += str_len;
    memcpy(hot_key_buf, &key, sizeof(key_with_modifier_t));
    return true;
}

bool remove_hot_key(hot_key_list_handle_t handle, const char* name)
{
    hot_key_list_header_t* list = *(hot_key_list_header_t**)handle;
    uint8_t* cursor= (uint8_t*)list + sizeof(hot_key_list_header_t);
    uint64_t need_copy_len = list->used_size-sizeof(hot_key_list_header_t);
    int i = 0;
    for (; i < list->hot_key_num; i ++ ) {
        uint64_t str_len;
        memcpy(&str_len, cursor, sizeof(uint64_t));
        uint64_t hot_key_buf_size = sizeof(uint64_t) + str_len + sizeof(key_with_modifier_t);
        str_len -= 1;
        need_copy_len -= hot_key_buf_size;
        if (strlen(name)!= str_len ||!!strncmp(name, (const char*)(cursor+ sizeof(uint64_t)), str_len) ){
            cursor += hot_key_buf_size;
            continue;
        }
        memmove(cursor, cursor + hot_key_buf_size, need_copy_len);
        list->used_size -= hot_key_buf_size;
        break;
    }
    if (i == list->hot_key_num) {
        return false;
    }
    return true;
}

void free_hot_key_list(hot_key_list_handle_t handle)
{
    if (handle) {
        hot_key_list_header_t* list = *(hot_key_list_header_t**)handle;
        if (list) {
            free(list);
        }
        free((hot_key_list_header_t**)handle);
    }
}

void get_hot_key_list_buf(hot_key_list_handle_t handle, uint8_t** buf, uint64_t* buf_size)
{
    hot_key_list_header_t* list = *(hot_key_list_header_t**)handle;
    *buf = (uint8_t*)list + sizeof(hot_key_list_header_t)-sizeof(uint64_t);
}

hot_key_list_handle_t parse_hot_key_list_buf(const uint8_t* buf)
{
    uint64_t list_len;
    uint64_t need_len=sizeof(hot_key_list_header_t);
    const uint8_t* cursor = buf+ sizeof(uint64_t);
    memcpy(&list_len, buf, sizeof(uint64_t));
    for (int i = 0; i < list_len; i++) {
        uint64_t str_len;
        memcpy(&str_len, cursor, sizeof(uint64_t));
        uint64_t hot_key_buf_size = sizeof(uint64_t) + str_len + sizeof(key_with_modifier_t);
        need_len += hot_key_buf_size;
        cursor += hot_key_buf_size;
    }
    hot_key_list_header_t* list = (hot_key_list_header_t*)malloc(need_len);
    if (!list) {
        return (hot_key_list_handle_t)NULL;
    }
    hot_key_list_handle_t handle = (uintptr_t)malloc(sizeof(hot_key_list_header_t*));
    if (!handle) {
        free(list);
        return (hot_key_list_handle_t)NULL;
    }
    *(hot_key_list_header_t**)handle = list;
    list->used_size = list->buf_size = need_len;

    memcpy((uint8_t*)list + sizeof(hot_key_list_header_t) - sizeof(uint64_t), buf, need_len - sizeof(hot_key_list_header_t) + sizeof(uint64_t));
    return handle;
}

uint64_t get_hot_key_list_len(hot_key_list_handle_t handle)
{
    if (!handle) {
        return 0;
    }
    hot_key_list_header_t* list = *(hot_key_list_header_t**)handle;
    return list->hot_key_num;
}

const char* get_hot_key_name(hot_key_list_handle_t handle,uint64_t index)
{
    if (!handle) {
        return NULL;
    }
    hot_key_list_header_t* list = *(hot_key_list_header_t**)handle;
    if (index >= list->hot_key_num) {
        return NULL;
    }
    uint8_t* cursor = (uint8_t*)list + sizeof(hot_key_list_header_t);
    for (int i = 0; i < index; i++) { 
        int64_t str_len;
        memcpy(&str_len, cursor, sizeof(uint64_t));
        uint64_t hot_key_buf_size = sizeof(uint64_t) + str_len + sizeof(key_with_modifier_t);
        cursor += hot_key_buf_size;
    }
    return (const char*)(cursor+ sizeof(uint64_t));
}

key_with_modifier_t get_hot_key(hot_key_list_handle_t handle,uint64_t index)
{
    key_with_modifier_t res={0};
    if (!handle) {
        return res;
    }
    hot_key_list_header_t* list = *(hot_key_list_header_t**)handle;
    if (index >= list->hot_key_num) {
        return res;
    }
    uint8_t* cursor = (uint8_t*)list + sizeof(hot_key_list_header_t);
    for (int i = 0; i < index; i++) {
        int64_t str_len;
        memcpy(&str_len, cursor, sizeof(uint64_t));
        uint64_t hot_key_buf_size = sizeof(uint64_t) + str_len + sizeof(key_with_modifier_t);
        cursor += hot_key_buf_size;
    }
    int64_t str_len;
    memcpy(&str_len, cursor, sizeof(uint64_t));
    memcpy(&res, cursor + sizeof(uint64_t) + str_len, sizeof(key_with_modifier_t));
    return res;
}
