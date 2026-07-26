#pragma once
#include <simdjson.h>
#include <simple_error.h>
template<class T, class V>
void SimdjsonGetResult(simdjson::simdjson_result<T> res, V& out, std::error_code& ec) {
    if (res.error() != simdjson::SUCCESS) {
        ec = utilpp::make_common_used_error(utilpp::ECommonUsedError::CUE_UNKNOW);
    }
    out = res.value_unsafe();
}

template<class T, class V ,class DefaultV>
void SimdjsonGetResultWithDefault(simdjson::simdjson_result<T> res, V& out, DefaultV&& defaultV) {
    if (res.error() != simdjson::SUCCESS) {
        out = defaultV;
    }
    out = res.value_unsafe();
}
