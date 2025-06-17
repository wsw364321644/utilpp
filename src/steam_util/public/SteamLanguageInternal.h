#pragma once 
#include <stdint.h>
class MsgClientLogon
{
public:
    static constexpr uint32_t ObfuscationMask = 0xBAADF00D;
    static constexpr uint32_t CurrentProtocol = 65580;
    static constexpr uint32_t ProtocolVerMajorMask = 0xFFFF0000;
    static constexpr uint32_t ProtocolVerMinorMask = 0xFFFF;
    static constexpr uint8_t ProtocolVerMinorMinGameServers = 4;
    static constexpr uint8_t ProtocolVerMinorMinForSupportingEMsgMulti = 12;
    static constexpr uint8_t ProtocolVerMinorMinForSupportingEMsgClientEncryptPct = 14;
    static constexpr uint8_t ProtocolVerMinorMinForExtendedMsgHdr = 17;
    static constexpr uint8_t ProtocolVerMinorMinForCellId = 18;
    static constexpr uint8_t ProtocolVerMinorMinForSessionIDLast = 19;
    static constexpr uint8_t ProtocolVerMinorMinForServerAvailablityMsgs = 24;
    static constexpr uint8_t ProtocolVerMinorMinClients = 25;
    static constexpr uint8_t ProtocolVerMinorMinForOSType = 26;
    static constexpr uint8_t ProtocolVerMinorMinForCegApplyPESig = 27;
    static constexpr uint8_t ProtocolVerMinorMinForMarketingMessages2 = 27;
    static constexpr uint8_t ProtocolVerMinorMinForAnyProtoBufMessages = 28;
    static constexpr uint8_t ProtocolVerMinorMinForProtoBufLoggedOffMessage = 28;
    static constexpr uint8_t ProtocolVerMinorMinForProtoBufMultiMessages = 28;
    static constexpr uint8_t ProtocolVerMinorMinForSendingProtocolToUFS = 30;
    static constexpr uint8_t ProtocolVerMinorMinForMachineAuth = 33;
    static constexpr uint8_t ProtocolVerMinorMinForSessionIDLastAnon = 36;
    static constexpr uint8_t ProtocolVerMinorMinForEnhancedAppList = 40;
    static constexpr uint8_t ProtocolVerMinorMinForSteamGuardNotificationUI = 41;
    static constexpr uint8_t ProtocolVerMinorMinForProtoBufServiceModuleCalls = 42;
    static constexpr uint8_t ProtocolVerMinorMinForGzipMultiMessages = 43;
    static constexpr uint8_t ProtocolVerMinorMinForNewVoiceCallAuthorize = 44;
    static constexpr uint8_t ProtocolVerMinorMinForClientInstanceIDs = 44;

};