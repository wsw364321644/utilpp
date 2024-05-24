#pragma once
#include <stdint.h>
enum class  EJsonRPCError :uint32_t {
    //reverse
	ReservedBegin = 0xFFFF8000,
    ParseError= 0xFFFF8044,
    InvalidRequest= 0xFFFF80A8,
    MethodNotFound= 0xFFFF80A7,
    InvalidParams= 0xFFFF80A6,
    InternalError= 0xFFFF80A5,
	ReservedServerBegin = 0xFFFF829D,
	ReservedEnd = 0xFFFF8300,
};

static char const* ToString(EJsonRPCError errorcode) {
	switch (errorcode)
	{
	case EJsonRPCError::ParseError:
		return "RPC Parse  Failed";
	case EJsonRPCError::InvalidRequest:
		return "Not a valid request";
	case EJsonRPCError::MethodNotFound:
		return "RPC method not found";
	case EJsonRPCError::InvalidParams:
		return "RPC params is invalid";
	case EJsonRPCError::InternalError:
		return "Server has error";
	default: {
		if (errorcode >= EJsonRPCError::ReservedBegin && errorcode <= EJsonRPCError::ReservedEnd) {
			if (errorcode >= EJsonRPCError::ReservedServerBegin) {
				return "Server error";
			}
			else {
				return "JRPC reserved";
			}
		}
		return "Error Not Exist";
	}
	}
}