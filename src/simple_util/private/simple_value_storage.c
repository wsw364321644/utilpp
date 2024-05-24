//#include "simple_value_storage.h"
//#include <stdlib.h> 
//#include <stdatomic.h>
//#include <stdalign.h>
//#include <assert.h>
//#define INCREASE_SIZE 64
//typedef struct storage_value_info_t {
//    atomic_intptr_t value;
//    atomic_intptr_t latest_value;
//    uint32_t size;
//    storage_value_changed_callback callback;
//}storage_value_info_t;
//storage_value_info_t** storage_value_info_list = NULL;
//uint32_t storage_value_info_list_len = 0;
//uint32_t storage_value_info_list_contain = 0;
//
//static bool resize_storage(uint32_t need_size) {
//    if (storage_value_info_list_len >= need_size) {
//        return true;
//    }
//
//    uint32_t new_size =((need_size - 1) / INCREASE_SIZE + 1)* INCREASE_SIZE;
//    storage_value_info_list = (storage_value_info_t**)malloc(sizeof(storage_value_info_t*) * new_size);
//    if (!storage_value_info_list) {
//        return false;
//    }
//    storage_value_info_list_len = new_size;
//    return true;
//}
//
//
//storage_value_handle_t register_storage_value(uint32_t size) {
//    if (!resize_storage(storage_value_info_list_contain +1)) {
//        return NULL;
//    }
//    uint32_t index = 0;
//    for (; index < storage_value_info_list_len; index++) {
//        if (!storage_value_info_list[index]) {
//            break;
//        }
//    }
//    assert(index < storage_value_info_list_len);
//    storage_value_info_list[index] = (storage_value_info_t*)malloc(sizeof(storage_value_info_t));
//    if (!storage_value_info_list[index]) {
//        return NULL;
//    }
//    storage_value_info_t* pinfo = storage_value_info_list[index];
//    pinfo->value = NULL;
//    pinfo->latest_value = NULL;
//    pinfo->size = size;
//    return (storage_value_handle_t)pinfo;
//}
//
//bool register_storage_value_changed(storage_value_handle_t handle, storage_value_changed_callback callback)
//{
//    storage_value_info_t* pinfo = (storage_value_info_t*)handle;
//    pinfo->callback = callback;
//    return true;
//}
//
//void remove_storage_value(storage_value_handle_t handle){
//    storage_value_info_t* pinfo = (storage_value_info_t*)handle;
//    uint32_t index = 0;
//    for (; index < storage_value_info_list_len; index++) {
//        if (storage_value_info_list[index]== pinfo) {
//            break;
//        }
//    }
//    if (index >= storage_value_info_list_len) {
//        return;
//    }
//    storage_value_info_list[index] = NULL;
//    free(pinfo);
//}
//
//void tick_value_storage(float delta)
//{
//    return ;
//}
//
//bool set_storage_value(storage_value_handle_t handle,void* value) {
//    storage_value_info_t* pinfo = (storage_value_info_t*)handle;
//    void* new_buf = malloc(pinfo->size);
//    
//    //atomic_store(, new_buf, pinfo->size)
//    //    memcpy(new_buf)
//}
//
//bool get_storage_value(storage_value_handle_t handle, void** value) {
//    storage_value_info_t* pinfo = (storage_value_info_t*)handle;
//
//}