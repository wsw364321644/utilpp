#pragma once
#include <memory>
#include <set>
#include <unordered_map>
#include <optional>

#include "message_processer.h"
#include "delegate_macros.h"
#pragma warning(push)
#pragma warning(disable:4251)


class RPCProcesser;
class RPCRequest;
class RPCResponse;

typedef struct RPCHandle_t :CommonHandle_t {
    constexpr  RPCHandle_t(uint32_t id) : CommonHandle_t(id) {}
    constexpr  RPCHandle_t():CommonHandle_t() {}
    RPCHandle_t(std::atomic_uint32_t& counter) :CommonHandle_t(counter) {}
    bool operator<(const RPCHandle_t& handle) const {
        return ID < handle.ID;
    }
}RPCHandle_t;




class  IGroupRPC {
public:
    virtual const char* GetName() = 0;
    virtual bool OnRequestRecv(std::shared_ptr<RPCRequest>) = 0;
    virtual bool OnResponseRecv(std::shared_ptr<RPCResponse>, std::shared_ptr<RPCRequest>) = 0;
    //virtual ~IGroupRPC() = 0;
    //DEFINE_EVENT_ONE_PARAM(OnRequestRecv, std::shared_ptr<RPCRequest>);
    //DEFINE_EVENT_TWO_PARAM(OnResponseRecv, std::shared_ptr<RPCResponse>, std::shared_ptr<RPCRequest>);
};
struct RPCInterfaceInfo
{
    typedef void*(*fnnew) (size_t);
    using TCreateMethod = std::unique_ptr<IGroupRPC>(*)(RPCProcesser*, fnnew);
    using TCheckMethod = bool(*)(const char*);
    TCreateMethod CreateFunc;
    TCheckMethod CheckFunc;

};
class IPC_EXPORT RPCInterfaceFactory
{

public:
    RPCInterfaceFactory() = delete;

    static bool Register(const char* name, RPCInterfaceInfo info);

    static std::unique_ptr<IGroupRPC> Create(const char* name, RPCProcesser* inprocesser, RPCInterfaceInfo::fnnew=nullptr);
    static bool CheckMethod(const char* name, const char* methodname);

    static std::unordered_map<std::string, RPCInterfaceInfo>* GetRPCInfos();

    static std::unordered_map<std::string, RPCInterfaceInfo>* RPCInfos;
};

/// <summary>
/// inherit RegisteredInRPCFactory and  achieve
/// static std::unique_ptr<IGroupRPC> Create()
/// static bool CheckMethod(const std::string& )
/// static std::string GetGroupName()
/// </summary>
/// <typeparam name="T"></typeparam>
template <typename T>
class RegisteredInRPCFactory
{
protected:
    RegisteredInRPCFactory() {
        s_bRegistered;
    }
    static bool s_bRegistered;
};
template <typename T>
bool RegisteredInRPCFactory<T>::s_bRegistered =
    RPCInterfaceFactory::Register(T::GetGroupName(), { .CreateFunc = &T::Create ,.CheckFunc = &T::CheckMethod });






template <typename T>
struct RPCMethodInfo
{
    typedef std::function<void(T*, std::shared_ptr<RPCRequest>)> TOnRequestMethod;
    typedef std::function<void(T*, std::shared_ptr<RPCResponse>, std::shared_ptr<RPCRequest>)> TOnResponseMethod;
    std::string Name;
    TOnRequestMethod OnRequestMethod;
    TOnResponseMethod OnResponseMethod;
};

template <typename T>
class RPCInfoData
{
public:
    static bool CheckMethod(const char* methodname) {
        auto itr = MethodInfos.find(methodname);
        if (itr == MethodInfos.end()) {
            return false;
        }
        return true;
    }
    static bool AddMethod(RPCMethodInfo<T> info) {
        MethodInfos.emplace(info.Name, info);
        return true;
    }

    static std::optional<RPCMethodInfo<T>> GetMethodInfo(const char* methodname) {
        if (CheckMethod(methodname)) {
            return MethodInfos[methodname];
        }
        return  std::nullopt;
    }
protected:
    static std::unordered_map<std::string, RPCMethodInfo<T>> MethodInfos;
};
/// <summary>
/// static value in rpc api 
/// 
/// </summary>
/// <typeparam name="T"></typeparam>
///
template <typename T>
class RPCInfoRegister
{
    friend T;
public:
    RPCInfoRegister(RPCMethodInfo<T> info) {
        RPCInfoData<T>::AddMethod(info);
    }
};

namespace std {
    template<>
    struct equal_to<RPCHandle_t> {
        using argument_type = RPCHandle_t;
        using result_type = bool;
        constexpr bool operator()(const RPCHandle_t& lhs, const RPCHandle_t& rhs) const {
            return lhs.ID == rhs.ID;
        }
    };


    template <>
    class hash<RPCHandle_t>
    {
    public:
        size_t operator()(const RPCHandle_t& handle) const
        {
            return handle.ID;
        }
    };

    template <>
    struct less<RPCHandle_t> {
    public:
        size_t operator()(const RPCHandle_t& _Left, const RPCHandle_t& _Right) const
        {
            return _Left.operator<(_Right);
        }
    };
}
#pragma warning(pop)

typedef std::function<void(RPCHandle_t,double,const char*, const char*)> TRPCErrorDelegate;
#define REGISTER_RPC_API_AUTO(APIName,ClassName) REGISTER_RPC_API(APIName,ClassName,On##APIName##RequestRecv, On##APIName##ResponseRecv)


#define DECLARE_RESPONSE_RPC_BASIC(Name)  \
private: \
            std::unordered_map<uint32_t, T##Name##Delegate> Name##Delegates; \
            std::unordered_map<uint32_t, TRPCErrorDelegate> Name##ErrorDelegates; \
            void Add##Name##SendDelagate(uint32_t id, T##Name##Delegate Delegate, TRPCErrorDelegate ErrDelegate) { \
                Name##Delegates.emplace(id, Delegate); \
                Name##ErrorDelegates.emplace(id, ErrDelegate); \
            } \
            void Remove##Name##SendDelagate(uint32_t id) { \
                Name##Delegates.erase(id); \
                Name##ErrorDelegates.erase(id); \
            } \
            bool Has##Name##SendDelagate(uint32_t id) { \
                if (Name##Delegates.find(id) == Name##Delegates.end()) { \
                    return false; \
                } \
                return true; \
            } \
public: \
            void On##Name##ResponseRecv(std::shared_ptr<RPCResponse>, std::shared_ptr<RPCRequest>);


#define DECLARE_REQUEST_RPC_BASIC(APIName)  \
public:  \
            static const char* Get##APIName##Name(){  \
                return APIName##Name;  \
            }  \
            void Cancel##APIName(RPCHandle_t); \
            void Register##APIName##(TRecv##APIName##Delegate); \
            void On##APIName##RequestRecv(std::shared_ptr<RPCRequest>); \
            static const char APIName##Name[]; \
private: \
            TRecv##APIName##Delegate recv##APIName##Delegate; \
public:


#define DEFINE_REQUEST_RPC_BASIC(ClassName,Name) \
void ClassName::Register##Name##(TRecv##Name##Delegate indelegate) \
{ \
    recv##Name##Delegate = indelegate; \
}



#define DEFINE_REQUEST_RPC(ClassName,Name) \
DEFINE_REQUEST_RPC_BASIC(ClassName,Name) \
void ClassName##::Cancel##Name##(RPCHandle_t handle) \
{ \
    auto req = processer->CancelRequest(handle); \
    if (!req.get()) { \
        return; \
    } \
    Name##Delegates.erase(req->ID.value()); \
} 


#define DEFINE_REQUEST_RPC_EVENT(ClassName,Name) \
DEFINE_REQUEST_RPC_BASIC(ClassName,Name) \
void ClassName##::Cancel##Name##(RPCHandle_t handle) \
{ \
    auto req = processer->CancelRequest(handle); \
    if (!req.get()) { \
        return; \
    } \
} 

#define DEFINE_REQUEST_RPC_DELEGATE(Name)  T##Name##Delegate,TRPCErrorDelegate

#define DECLARE_RESPONSE_RPC(Name)  \
            typedef std::function<void(RPCHandle_t)> T##Name##Delegate; \
            bool Respond##Name (RPCHandle_t id ); \
            DECLARE_RESPONSE_RPC_BASIC(Name)
#define DECLARE_REQUEST_RPC(Name)  \
            RPCHandle_t Name (DEFINE_REQUEST_RPC_DELEGATE(Name)) ; \
            typedef std::function<void(RPCHandle_t)> TRecv##Name##Delegate; \
            DECLARE_REQUEST_RPC_BASIC(Name)
#define DECLARE_REQUEST_RPC_EVENT(Name)  \
            bool Name(); \
            typedef std::function<void()> TRecv##Name##Delegate; \
            DECLARE_REQUEST_RPC_BASIC(Name)



#define DECLARE_RESPONSE_RPC_ONE_PARAM(Name, Param1Type)  \
            typedef std::function<void(RPCHandle_t, Param1Type)> T##Name##Delegate; \
            bool Respond##Name (RPCHandle_t id, Param1Type Param1); \
            DECLARE_RESPONSE_RPC_BASIC(Name)
#define DECLARE_REQUEST_RPC_ONE_PARAM(Name, Param1Type)  \
            RPCHandle_t Name (Param1Type Param1,DEFINE_REQUEST_RPC_DELEGATE(Name)) ; \
            typedef std::function<void(RPCHandle_t, Param1Type)> TRecv##Name##Delegate; \
            DECLARE_REQUEST_RPC_BASIC(Name)
#define DECLARE_REQUEST_RPC_EVENT_ONE_PARAM(Name, Param1Type)  \
            bool Name(Param1Type Param1); \
            typedef std::function<void( Param1Type)> TRecv##Name##Delegate; \
            DECLARE_REQUEST_RPC_BASIC(Name)



#define DECLARE_RESPONSE_RPC_TWO_PARAM(Name, Param1Type ,Param2Type)  \
            typedef std::function<void(RPCHandle_t, Param1Type,Param2Type)> T##Name##Delegate; \
            bool Respond##Name (RPCHandle_t id, Param1Type Param1,Param2Type Param2); \
            DECLARE_RESPONSE_RPC_BASIC(Name)
#define DECLARE_REQUEST_RPC_TWO_PARAM(Name, Param1Type,Param2Type)  \
            RPCHandle_t Name (Param1Type Param1,Param2Type Param2,DEFINE_REQUEST_RPC_DELEGATE(Name)) ; \
            typedef std::function<void(RPCHandle_t, Param1Type,Param2Type)> TRecv##Name##Delegate; \
            DECLARE_REQUEST_RPC_BASIC(Name)
#define DECLARE_REQUEST_RPC_EVENT_TWO_PARAM(Name, Param1Type,Param2Type)  \
            bool Name(Param1Type Param1,Param2Type Param2); \
            typedef std::function<void(Param1Type,Param2Type)> TRecv##Name##Delegate; \
            DECLARE_REQUEST_RPC_BASIC(Name)



#define DECLARE_RESPONSE_RPC_THREE_PARAM(Name, Param1Type ,Param2Type,Param3Type)  \
            typedef std::function<void(RPCHandle_t, Param1Type,Param2Type,Param3Type)> T##Name##Delegate; \
            bool Respond##Name (RPCHandle_t id, Param1Type Param1,Param2Type Param2,Param3Type Param3); \
            DECLARE_RESPONSE_RPC_BASIC(Name)
#define DECLARE_REQUEST_RPC_THREE_PARAM(Name, Param1Type,Param2Type,Param3Type)  \
            RPCHandle_t Name (Param1Type Param1,Param2Type Param2,Param3Type Param3,DEFINE_REQUEST_RPC_DELEGATE(Name)) ; \
            typedef std::function<void(RPCHandle_t, Param1Type,Param2Type,Param3Type)> TRecv##Name##Delegate; \
            DECLARE_REQUEST_RPC_BASIC(Name)
#define DECLARE_REQUEST_RPC_EVENT_THREE_PARAM(Name, Param1Type,Param2Type,Param3Type)  \
            bool Name(Param1Type Param1,Param2Type Param2,Param3Type Param3); \
            typedef std::function<void(Param1Type,Param2Type,Param3Type)> TRecv##Name##Delegate; \
            DECLARE_REQUEST_RPC_BASIC(Name)


#define DECLARE_RESPONSE_RPC_FOUR_PARAM(Name, Param1Type ,Param2Type,Param3Type,Param4Type)  \
            typedef std::function<void(RPCHandle_t, Param1Type,Param2Type,Param3Type,Param4Type)> T##Name##Delegate; \
            bool Respond##Name (RPCHandle_t id, Param1Type Param1,Param2Type Param2,Param3Type Param3,Param4Type Param4); \
            DECLARE_RESPONSE_RPC_BASIC(Name)
#define DECLARE_REQUEST_RPC_FOUR_PARAM(Name, Param1Type,Param2Type,Param3Type,Param4Type)  \
            RPCHandle_t Name (Param1Type Param1,Param2Type Param2,Param3Type Param3,Param4Type Param4,DEFINE_REQUEST_RPC_DELEGATE(Name)) ; \
            typedef std::function<void(RPCHandle_t, Param1Type,Param2Type,Param3Type,Param4Type)> TRecv##Name##Delegate; \
            DECLARE_REQUEST_RPC_BASIC(Name)
#define DECLARE_REQUEST_RPC_EVENT_FOUR_PARAM(Name, Param1Type,Param2Type,Param3Type,Param4Type)  \
            bool Name(Param1Type Param1,Param2Type Param2,Param3Type Param3,Param4Type Param4); \
            typedef std::function<void(Param1Type,Param2Type,Param3Type,Param4Type)> TRecv##Name##Delegate; \
            DECLARE_REQUEST_RPC_BASIC(Name)


#define DECLARE_RESPONSE_RPC_FIVE_PARAM(Name, Param1Type ,Param2Type,Param3Type,Param4Type,Param5Type)  \
            typedef std::function<void(RPCHandle_t, Param1Type,Param2Type,Param3Type,Param4Type,Param5Type)> T##Name##Delegate; \
            bool Respond##Name (RPCHandle_t id, Param1Type Param1,Param2Type Param2,Param3Type Param3,Param4Type Param4,Param5Type Param5); \
            DECLARE_RESPONSE_RPC_BASIC(Name)
#define DECLARE_REQUEST_RPC_FIVE_PARAM(Name, Param1Type,Param2Type,Param3Type,Param4Type,Param5Type)  \
            RPCHandle_t Name (Param1Type Param1,Param2Type Param2,Param3Type Param3,Param4Type Param4,Param5Type Param5,DEFINE_REQUEST_RPC_DELEGATE(Name)) ; \
            typedef std::function<void(RPCHandle_t, Param1Type,Param2Type,Param3Type,Param4Type,Param5Type)> TRecv##Name##Delegate; \
            DECLARE_REQUEST_RPC_BASIC(Name)
#define DECLARE_REQUEST_RPC_EVENT_FIVE_PARAM(Name, Param1Type,Param2Type,Param3Type,Param4Type,Param5Type)  \
            bool Name(Param1Type Param1,Param2Type Param2,Param3Type Param3,Param4Type Param4,Param5Type Param5); \
            typedef std::function<void(Param1Type,Param2Type,Param3Type,Param4Type,Param5Type)> TRecv##Name##Delegate; \
            DECLARE_REQUEST_RPC_BASIC(Name)




#define DECLARE_RESPONSE_RPC_SIX_PARAM(Name, Param1Type ,Param2Type,Param3Type,Param4Type,Param5Type,Param6Type)  \
            typedef std::function<void(RPCHandle_t, Param1Type,Param2Type,Param3Type,Param4Type,Param5Type,Param6Type)> T##Name##Delegate; \
            bool Respond##Name (RPCHandle_t id, Param1Type Param1,Param2Type Param2,Param3Type Param3,Param4Type Param4,Param5Type Param5,Param6Type Param6); \
            DECLARE_RESPONSE_RPC_BASIC(Name)
#define DECLARE_REQUEST_RPC_SIX_PARAM(Name, Param1Type,Param2Type,Param3Type,Param4Type,Param5Type,Param6Type)  \
            RPCHandle_t Name (Param1Type Param1,Param2Type Param2,Param3Type Param3,Param4Type Param4,Param5Type Param5,Param6Type Param6,DEFINE_REQUEST_RPC_DELEGATE(Name)) ; \
            typedef std::function<void(RPCHandle_t, Param1Type,Param2Type,Param3Type,Param4Type,Param5Type,Param6Type)> TRecv##Name##Delegate; \
            DECLARE_REQUEST_RPC_BASIC(Name)
#define DECLARE_REQUEST_RPC_EVENT_SIX_PARAM(Name, Param1Type,Param2Type,Param3Type,Param4Type,Param5Type,Param6Type)  \
            bool Name(Param1Type Param1,Param2Type Param2,Param3Type Param3,Param4Type Param4,Param5Type Param5,Param6Type Param6); \
            typedef std::function<void(Param1Type,Param2Type,Param3Type,Param4Type,Param5Type,Param6Type)> TRecv##Name##Delegate; \
            DECLARE_REQUEST_RPC_BASIC(Name)