#pragma once
#include <memory>
#include <string_view>
#include <set>
#include <unordered_map>
#include <optional>
#include <cassert>

#include "message_processer.h"
#include "delegate_macros.h"
#pragma warning(push)
#pragma warning(disable:4251)


class IRPCProcesser;
class RPCRequest;
class RPCResponse;
class IGroupRPC;
typedef struct RPCHandle_t :CommonHandle_t {
    constexpr  RPCHandle_t(CommonHandleID_t id) : CommonHandle_t(id) {}
    constexpr RPCHandle_t():CommonHandle_t() {}
    constexpr RPCHandle_t(const NullCommonHandle_t nullhandle):CommonHandle_t(nullhandle) {}
    RPCHandle_t(std::atomic<CommonHandleID_t>& counter) :CommonHandle_t(counter) {}
    bool operator<(const RPCHandle_t& handle) const {
        return ID < handle.ID;
    }
    IGroupRPC* PIGroupRPC{ 0 };
}RPCHandle_t;




class IPC_EXPORT IGroupRPC {
public:
    IGroupRPC(IRPCProcesser* inprocesser) :processer(inprocesser) {}
    virtual const char* GetName() = 0;
    virtual bool OnRequestRecv(std::shared_ptr<RPCRequest>) = 0;
    virtual bool OnResponseRecv(std::shared_ptr<RPCResponse>, std::shared_ptr<RPCRequest>) = 0;
    virtual RPCHandle_t SendRPCRequest(std::shared_ptr< RPCRequest> req);
    virtual bool SendRPCEvent(std::shared_ptr< RPCRequest> req);
    virtual std::shared_ptr<RPCRequest> CancelRPCRequest(RPCHandle_t handle);
    virtual bool SendRPCResponse(RPCHandle_t handle, std::shared_ptr<RPCResponse> response);
protected:
    IRPCProcesser* processer;
};
struct RPCInterfaceInfo
{
    typedef void*(*fnnew) (size_t);
    using TCreateMethod = std::unique_ptr<IGroupRPC>(*)(IRPCProcesser*, fnnew);
    using TCheckMethod = bool(*)(const char*);
    TCreateMethod CreateFunc;
    TCheckMethod CheckFunc;

};
class IPC_EXPORT RPCInterfaceFactory
{

public:
    RPCInterfaceFactory() = delete;

    static bool Register(const char* name, RPCInterfaceInfo info);

    static std::unique_ptr<IGroupRPC> Create(const char* name, IRPCProcesser* inprocesser, RPCInterfaceInfo::fnnew=nullptr);
    static bool CheckMethod(const char* name, const char* methodname);

    static std::unordered_map<std::string, RPCInterfaceInfo>* GetRPCInfos();

    static std::unordered_map<std::string, RPCInterfaceInfo>* RPCInfos;
};


template <typename T>
struct RPCMethodInfo
{
    typedef std::function<void(T*, std::shared_ptr<RPCRequest>)> TOnRequestMethod;
    typedef std::function<void(T*, std::shared_ptr<RPCResponse>, std::shared_ptr<RPCRequest>)> TOnResponseMethod;
    typedef std::function<void(T*, uint32_t)> TRemoveSendDelagateMethod;
    std::string Name;
    TOnRequestMethod OnRequestMethod;
    TOnResponseMethod OnResponseMethod;
    TRemoveSendDelagateMethod RemoveSendDelagateMethod;
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

/// @details static value in rpc api 
template <typename T>
class RPCInfoRegister
{
    friend T;
public:
    RPCInfoRegister(RPCMethodInfo<T> info) {
        RPCInfoData<T>::AddMethod(info);
    }
};


/// @details inherit RegisteredInRPCFactory and  achieve
/// static std::unique_ptr<IGroupRPC> Create()
/// static bool CheckMethod(const std::string& )
/// static std::string GetGroupName()

template <typename T>
class RegisteredInRPCFactory
{
public :
    static bool s_bRegistered;
protected:
    RegisteredInRPCFactory() {
        s_bRegistered;
    }
};
template <typename T>
bool RegisteredInRPCFactory<T>::s_bRegistered =
RPCInterfaceFactory::Register(T::GetGroupName(), { .CreateFunc = &T::Create ,.CheckFunc = &RPCInfoData<T>::CheckMethod });



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

typedef std::function<void(RPCHandle_t,int64_t, std::string_view, std::string_view)> TRPCErrorDelegate;



#define REGISTER_RPC_API(ClassName,APIName,RequestRecvFunc,ResponseRecvFunc)                                                    \
    const char ClassName::APIName##Name[] = #APIName;                                                                           \
    static RPCInfoRegister<ClassName> APIName##Register(RPCMethodInfo<ClassName>{.Name = ClassName::APIName##Name,            \
        .OnRequestMethod = &ClassName::RequestRecvFunc,                                                                         \
        .OnResponseMethod = &ClassName::ResponseRecvFunc,                                                                       \
        .RemoveSendDelagateMethod= &ClassName::Remove##APIName##SendDelagate                                                    \
    });

#define REGISTER_RPC_API_AUTO(ClassName,APIName) REGISTER_RPC_API(ClassName,APIName,On##APIName##RequestRecv, On##APIName##ResponseRecv)

#define REGISTER_RPC_EVENT_API(ClassName,APIName,RequestRecvFunc) \
    const char ClassName::APIName##Name[] = #APIName; \
    static RPCInfoRegister<##ClassName> APIName##Register(RPCMethodInfo<##ClassName>{.Name = ClassName::APIName##Name, \
        .OnRequestMethod = &##ClassName::##RequestRecvFunc, \
        .OnResponseMethod = nullptr \
    });

#define REGISTER_RPC_EVENT_API_AUTO(ClassName,APIName) REGISTER_RPC_EVENT_API(ClassName,APIName,On##APIName##RequestRecv)


#define DECLARE_RPC_OVERRIDE_FUNCTION(ClassName)                                                              \
public:                                                                                                       \
    friend struct RPCMethodInfo<ClassName>;                                                                   \
    ClassName(IRPCProcesser*);                                                                                 \
    virtual ~ClassName();                                                                                     \
    static std::unique_ptr<IGroupRPC> Create(IRPCProcesser* inprocesser, RPCInterfaceInfo::fnnew);             \
    static const char* GetGroupName();                                                                        \
    const char* GetName() override;                                                                           \
    bool OnRequestRecv(std::shared_ptr<RPCRequest>)override;                                                  \
    bool OnResponseRecv(std::shared_ptr<RPCResponse>, std::shared_ptr<RPCRequest>)override;                   \
    std::shared_ptr<RPCRequest> CancelRPCRequest(RPCHandle_t handle) override;       

#define DEFINE_RPC_OVERRIDE_FUNCTION_BASE(ClassName,GroupName)                                             \
static bool RegisteredInRPCFactory##ClassName= RegisteredInRPCFactory<ClassName>::s_bRegistered;           \
template <>                                                                                               \
std::unordered_map<std::string, RPCMethodInfo<ClassName>> RPCInfoData<ClassName>::MethodInfos{};          \
ClassName::ClassName(IRPCProcesser* inprocesser) :IGroupJRPC(inprocesser)                                  \
{                                                                                                         \
}                                                                                                         \
ClassName::~ClassName()                                                                                   \
{                                                                                                         \
}                                                                                                         \
std::unique_ptr<IGroupRPC> ClassName::Create(IRPCProcesser* inprocesser, RPCInterfaceInfo::fnnew fn)       \
{                                                                                                         \
    if (fn) {                                                                                             \
        ClassName* ptr = (ClassName*)fn(sizeof(ClassName));                                               \
        new(ptr)ClassName(inprocesser);                                                                   \
        return  std::unique_ptr<ClassName>(ptr);                                                          \
    }                                                                                                     \
    return std::make_unique<ClassName>(inprocesser);                                                      \
}                                                                                                         \
const char* ClassName::GetGroupName()                                                                     \
{                                                                                                         \
    return GroupName;                                                                                     \
}                                                                                                         \
const char* ClassName::GetName() {                                                                        \
    return GetGroupName();                                                                                \
}                                                                                                         

#define DEFINE_RPC_OVERRIDE_FUNCTION(ClassName,GroupName)                                              \
DEFINE_RPC_OVERRIDE_FUNCTION_BASE(ClassName,GroupName)                                                 \
std::shared_ptr<RPCRequest> ClassName::CancelRPCRequest(RPCHandle_t handle) {                          \
    std::shared_ptr<RPCRequest> req = IGroupRPC::CancelRPCRequest(handle);                             \
    if (!req) {                                                                                        \
        return req;                                                                                    \
    }                                                                                                  \
    auto RPCMethodInfoOpt = RPCInfoData<ClassName>::GetMethodInfo(req->GetMethod().data());            \
    assert(RPCMethodInfoOpt.has_value());                                                              \
    RPCMethodInfoOpt.value().RemoveSendDelagateMethod(this, req->GetID());                             \
    return req;                                                                                        \
}


#define DEFINE_RPC_OVERRIDE_FUNCTION_EVENT(ClassName,GroupName)                                         \
DEFINE_RPC_OVERRIDE_FUNCTION_BASE(ClassName,GroupName)                                                  \
std::shared_ptr<RPCRequest> ClassName::CancelRPCRequest(RPCHandle_t handle) {                           \
    return IGroupRPC::CancelRPCRequest(handle);                                                         \
}




#define DECLARE_RESPONSE_RPC_BASIC(Name)                                                                                                 \
private:                                                                                                                                 \
            std::unordered_map<uint32_t, T##Name##Delegate> Name##Delegates;                                                             \
            std::unordered_map<uint32_t, TRPCErrorDelegate> Name##ErrorDelegates;                                                        \
            void Add##Name##SendDelagate(uint32_t id, T##Name##Delegate Delegate, TRPCErrorDelegate ErrDelegate) {                       \
                Name##Delegates.emplace(id, Delegate);                                                                                   \
                Name##ErrorDelegates.emplace(id, ErrDelegate);                                                                           \
            }                                                                                                                            \
            template <class ..._Types>                                                                                                   \
            void Trigger##Name##SendDelegate(uint32_t id, _Types&&... args) {                                                            \
                Name##Delegates[id](RPCHandle_t(id), std::forward<_Types>(args)...);                                                     \
            }                                                                                                                            \
                                                                                                                                         \
            void Trigger##Name##SendErrorDelegate(uint32_t id, double code, const char* errMsg, const char* errData) {                   \
                Name##ErrorDelegates[id](RPCHandle_t(id), code, errMsg, errData);                                                        \
            }                                                                                                                            \
public:                                                                                                                                  \
            void Remove##Name##SendDelagate(uint32_t id) {                                                                               \
                Name##Delegates.erase(id);                                                                                               \
                Name##ErrorDelegates.erase(id);                                                                                          \
            }                                                                                                                            \
            bool Has##Name##SendDelagate(uint32_t id) {                                                                                  \
                if (Name##Delegates.find(id) == Name##Delegates.end()) {                                                                 \
                    return false;                                                                                                        \
                }                                                                                                                        \
                return true;                                                                                                             \
            }                                                                                                                            \
            void On##Name##ResponseRecv(std::shared_ptr<RPCResponse>, std::shared_ptr<RPCRequest>);


#define DECLARE_REQUEST_RPC_BASIC(APIName)  \
public:  \
            static const char* Get##APIName##Name(){  \
                return APIName##Name;  \
            }  \
            void Register##APIName(TRecv##APIName##Delegate); \
            void On##APIName##RequestRecv(std::shared_ptr<RPCRequest>); \
            static const char APIName##Name[]; \
private: \
            TRecv##APIName##Delegate Recv##APIName##Delegate; \
public:


#define DEFINE_REQUEST_RPC_BASIC(ClassName,Name) \
void ClassName::Register##Name(TRecv##Name##Delegate indelegate) \
{ \
    Recv##Name##Delegate = indelegate; \
}



#define DEFINE_REQUEST_RPC(ClassName,Name) \
DEFINE_REQUEST_RPC_BASIC(ClassName,Name)

#define DEFINE_REQUEST_RPC_EVENT(ClassName,Name) \
DEFINE_REQUEST_RPC_BASIC(ClassName,Name) 

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
