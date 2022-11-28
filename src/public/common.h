/**
 *   common.h
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif
    enum PACKET_TYPE
    {
        PT_None,
        PT_Ack,
        PT_MAX,
    };

	enum PACKET_SERVER_TYPE
	{
		PST_LOGIN = PT_MAX,
		PST_LOGIN_ACK,//3
		PST_FETCH_ACHIEVEMENTS,
		PST_FETCH_ACHIEVEMENTS_ACK,
		PST_UNLOCK_ACHIEVEMENT,
		PST_UNLOCK_ACHIEVEMENT_ACK,
		PST_SYNC_FILE,
		PST_SYNC_FILE_ACK,
		PST_SYNC_STAT,
		PST_SYNC_STAT_ACK,
		PST_FETCH_SAVEFILES,
		PST_FETCH_SAVEFILES_ACK,
		PST_FETCH_SAVEPATH,
		PST_FETCH_SAVEPATH_ACK,
		PST_SET_STAT_INT,
		PST_SET_STAT_INT_ACK,
		PST_ITEM_OP,
		PST_ITEM_OP_ACK,
		PST_ORDER_OP,
		PST_ORDER_OP_ACK,
		PST_GET_PUBKEY,
		PST_GET_PUBKEY_ACK,
		PST_SEND_KEY,
		PST_SEND_KEY_ACK,
		PST_HEARTBEAT,
		PST_HEARTBEAT_ACK,
		PST_BEKICKOUT,
		PST_CHACKGAME,
		PST_CHACKGAME_ACK,
		PST_GETGAMELIST,
		PST_GETGAMELIST_ACK,
		PST_SENDHASH,
		PST_SENDHASH_ACK,
		PST_ADDLOCALGAME,
		PST_ADDLOCALGAME_ACK,
		PST_GAMESTART,
		PST_GAMEOVER,
		PST_GAMESTART_ACK,	    
		PST_ERRLOGINFO,
		PST_ON_KEY_ACK,
		PST_DELETE_CLOUDFILE,
		PST_GET_TICKET,
		PST_GET_TICKET_ACK,
		PST_GET_FRIEND_STATUS,
		PST_UPDATE_FRIEND_STATUS,
		PST_ACTIVATE_TO_SONKWO,
		PST_ACTIVATE_ACK,
		PST_STEAMAPPIDTOGAMEID,
		PST_STEAMAPPIDTOGAMEID_ACK,
		PST_GAMEIDTOSTEAMAPPID,
		PST_GAMEIDTOSTEAMAPPID_ACK,
		PST_HEARTBEATWITHOUTGAME,
		PST_GENERATE_ORDER,
		PST_GENERATE_ORDER_ACK,
		PST_VERIFY_CODE,
		PST_VERIFY_CODE_ACK,
		PST_CONFIRM_REQUEST,
		PST_CONFIRM_REQUEST_ACK,
		PST_DEMO_STATUS,
    };

	enum PACKET_SERVER_MM_TYPE
	{
		PST_MM_CHECK_GAME = PST_LOGIN_ACK + 1,
		PST_MM_CHECK_GAME_ACK,
		PST_MM_START_GAME,
		PST_MM_START_GAME_ACK,
		PST_MM_START_SERVER_NOTIFY,
		PST_MM_INVITE_TO_NOTIFY,
		PST_MM_GAME_OFFLINE_NOTIFY,
		PST_MM_CHECK_USER_STAT,
		PST_MM_CHECK_USER_STAT_ACK,
		PST_MM_CANCEL_GAME,
		PST_MM_CANCEL_GAME_ACK,
		PST_MM_USER_OFFLINE_NOTIFY,
		PST_MM_USER_JOIN_NOTIFY,
		PST_MM_USER_LEAVE_NOTIFY,

		PST_MM_PARTY_ACTION,
		PST_MM_PARTY_ACTION_ACK,
		PST_MM_PARTY_JOIN_ACK,

		PST_MM_ROOM_ACTION,
		PST_MM_ROOM_ACTION_ACK,
		PST_MM_ROOM_JOIN_ACK,

		PST_MM_INVITE_TO,
		PST_MM_USER_ONLINE_NOTIFY,

		PST_MM_ACTION,
		PST_MM_ACTION_ACK,
		PST_MM_GET_REGION_LIST,
		PST_MM_GET_REGION_LIST_ACK,
		PST_MM_HEART_BEAT,
		PST_MM_END,
	};

	enum PACKET_SERVER_MM_SUB_TYPE
	{
		PST_SM_CREATE_ROOM,
		PST_SM_LEAVE_ROOM,
		PST_SM_DISMISS_ROOM,
		PST_SM_JOIN_ROOM,
		PST_SM_CHNAGE_ROOM_PASSWD,
		PST_SM_ADD_AI,
		PST_SM_REMOVE_AI,
		PST_SM_JOIN_TEAM,
		PST_SM_LEAVE_TEAM,
		PST_SM_START,
		PST_SM_READY,
		PST_SM_SET_DATA,

		PST_PM_CREATE_PARTY,
		PST_PM_LEAVE_PARTY,
		PST_PM_JOIN_PARTY,
		PST_PM_DISMISS_PARTY,
	};

	enum PACKET_GAME_TYPE
	{
		PGT_GAME_INFO = PT_MAX,
		PGT_GAME_INFO_ACK,
		PGT_ACHIEVEMENT,
		PGT_ACHIEVEMENT_ACK,
		PGT_SYNC_SAVEFILE,
		PGT_SYNC_SAVEFILE_ACK,
		PGT_QUIT,
		PGT_QUIT_ACK,
		PGT_ITEM_QUERY,
		PGT_ITEM_QUERY_ACK,
		PGT_ITEM_PURCHASE,
		PGT_ITEM_PURCHASE_ACK,
		PGT_ORDER_PAY,
		PGT_PRDER_PAY_ACK,
		PGT_ORDER_CHECKOUT,
		PGT_ORDER_CHECKOUT_ACK,
		PGT_ADULT_SET,
		PGT_MM_START,
		PGT_MM_START_ACK,
		PGT_MM_CANCEL,
		PGT_MM_CANCEL_ACK,
		PGT_MM_CHECK_USER_STAT,
		PGT_MM_CHECK_USER_STAT_ACK,
		PGT_MM_USER_JOIN_NOTIFY,
		PGT_MM_USER_LEAVE_NOTIFY,

		PGT_MM_PARTY_ACTION,
		PGT_MM_PARTY_ACTION_ACK,
		PGT_MM_PARTY_JOIN_ACK,

		PGT_MM_ROOM_ACTION,
		PGT_MM_ROOM_ACTION_ACK,
		PGT_MM_ROOM_JOIN_ACK,

		PGT_MM_NET_NOTIFY,
		PGT_MM_START_SERVER_NOTIFY,
		PGT_MM_ACTION,
		PGT_MM_ACTION_ACK,

		PGT_TICKET_REQUEST,
		PGT_TICKET_REQUEST_ACK,
		PGT_MM_REGION_LIST_NOTIFY,
		PGT_MM_GET_PING_TTL,
		PGT_MM_GET_PING_TTL_ACK,
		PGT_MM_SELECT_REGION,
		PGT_GET_ITEM_ACK,
	};
	
	enum PACKET_MM_SUB_TYPE
	{
		PGT_RM_CREATE_ROOM,
		PGT_RM_LEAVE_ROOM,
		PGT_RM_DISMISS_ROOM,
		PGT_RM_JOIN_ROOM,
		PGT_RM_CHNAGE_ROOM_PASSWD,
		PGT_RM_ADD_AI,
		PGT_RM_REMOVE_AI,
		PGT_RM_JOIN_TEAM,
		PGT_RM_LEAVE_TEAM,
		PGT_RM_READY,
		PGT_RM_START,
		PGT_RM_SET_DATA,

		PGT_PM_CREATE_PARTY,
		PGT_PM_LEAVE_PARTY,
		PGT_PM_DISMISS_PARTY,
		PGT_PM_JOIN_PARTY,
    };

    enum PACKET_CODE
    {
        PC_SUCCESS,
        PC_FAILED
    };

    enum ACTION_TYPE
    {
        AT_ACHIEVEMENT,
        AT_FILE,
        AT_ITEM,
        AT_ORDER,
		AT_MM,
		AT_TICKET
    };

    enum STAT_OP
    {
        SO_SYNC     = 1 << 0,
        SO_UNLOCK   = 1 << 1,
        SO_FETCH    = 1 << 2,
        SO_SET      = 1 << 3,
    };

    enum FILE_OP
    {
        FO_TOKEN    = 1 << 0,
        FO_VERIFY   = 1 << 1,
    };

    enum ITEM_OP
    {
        IO_QUERY    = 1 << 0,
        IO_PURCHASE = 1 << 1,
    };

    enum ORDER_OP
    {
        OO_PAY      = 1 << 0,
        OO_CHECKOUT = 1 << 1,
    };

	enum MM_OP
	{
		MO_START				= 0,
		MO_CANCEL,				

		MO_SET_DATA,

		MO_CREATE_PARTY,	
		MO_JOIN_PARTY,
		MO_LEAVE_PARYT,
		MO_DISMISS_PARTY,

		MO_CREATE_ROOM,		
		MO_JOIN_ROOM,			
		MO_LEAVE_ROOM,		
		MO_DISMISS_ROOM,
		MO_CHANGE_ROOM_PASSWD,	

		MO_ROOM_ADD_AI ,
		MO_ROOM_REMOVE_AI,
		MO_ROOM_READY,
		MO_ROOM_START,
		MO_ROOM_LEAVE_TEAM,
		MO_ROOM_JOIN_TEAM,

		MO_CHECK,
		MO_GET_REGION_LIST_PING_TTL,
		MO_SELECT_REGION,
	};
    
	enum NTF_TYPE
	{
		NT_ROOM          = 0,
		NT_PARTY,
		NT_TEAM,
		NT_GAME,
		NT_ROOM_READY,
		NT_ROOM_START,
		NT_ROOM_SET_ROOM_DATA,
		NT_ROOM_SET_USER_DATA,
		NT_ROOM_DISMISS_NORMAL,
		NT_ROOM_DISMISS_TIMEOUT,
		NT_ROOM_RESTART,
		NT_PARTY_DISMISS,
		NT_MATCH,
		NT_MATCH_END,
		NT_PARTY_SET_DATA,
		NT_PARTY_SET_USER_DATA,
		NT_KICK,
	};

	enum NET_STATE
	{
		NS_ONLINE = 0,
		NS_OFFLINE
	};
	
    enum OP_RESULT
    {
        OR_OK       = 1 << 0,
        OR_FAIL     = 1 << 1,
    };
	enum NET_LOCATION
	{
		NET_CN = 1,
		NET_HK
	};
	enum FRIEND_STATUS
	{
		FRIEND_INIT =0,
		FRIEND_ONLINE,
		FRIEND_OFFLINE,
		FRIEND_GAME_START,
		FRIEND_GAME_OVER,
	};
// invalid game id
#define INVALID_GAME_ID     0
// invalid user id
#define INVALID_USER_ID     0

// header size of the packet
#define PACKET_HEADER_SIZE 4

// message key definitions
#define USER            "user"
#define GAME            "game"
#define RESULT          "result"
#define TIME            "time"
#define NAME            "name"
#define CHECKSUM        "checksum"
#define CLIENT_VERSION  "version"

// game info 
#define API_DLL_PATH    "api_path"
#define EXE_NAME        "exe_name"
#define USER_INFO		"user_info"
#define KEY_TYPE		"key_type"
#define SKU_ENAME		"sku_ename"
#define INSTALL_TYPE		"install_type"

// ack key
#define ACK_RESULT      "ack_result"
#define ACK_ERROR       "ack_error"
#define ACK_REQUEST     "ack_request"

// login
#define REFRESH_TOKEN       "refresh_token"
#define PERMERNENT_TOKEN    "permernent_token"

// achievement&stat
#define ACH_OPER            "ach_oper"
#define ACH_VALUE           "ach_value"
#define ACH_TIME            "unlock_time"
#define ACHIEVEMENT         "achievement"
#define ACHIEVEMENT_LIST    "achievement_list"
#define STAT                "stat"
#define STAT_VALUE          "stat_value"

// iap
#define ITEM_LIST           "item_list"
#define ITEM_TOTAL          "item_total"
#define ITEM_NAME           "item_name"
#define ITEM_PRICE          "item_price"
#define ITEM_COUNT          "item_count"
#define ITEM_OPER           "item_oper"

#define ORDER_ID            "order_id"
#define ORDER_OPER          "order_oper"
#define ORDER_TOKEN         "order_token"

// matchmaking
enum PKT_VALUE_TYPE
{
	PVT_STRING = 0,
	PVT_INT
};

#define K_SERVER_VERSION	"s_ver"
#define K_CLIENT_VERSION	"c_ver"
#define K_PASSWD			"passwd"
#define K_GAME_MODE			"gamemd"
#define K_GAME_MODE_COUNT	"gamemd_cnt"
#define K_TTL				"ttl"
#define K_ROOM_TOKEN		"rtoken"
#define K_ROOM_ROLE			"rrole"
#define K_READY_COUNT		"krcnt"
#define K_OLD_ROOM_TOKEN	"roldtoken"	
#define K_ROOM_TOTAL_COUNT	"rtotcnt"
#define K_TEAMA_COUNT		"ta_cnt"
#define K_TEAMB_COUNT		"tb_cnt"
#define K_TEAM_VISIT_COUNT	"tv_cnt"	
#define K_OP				"op"
#define K_OP_KICK			"op_kick"
#define K_OP_USER			"op_usr"
#define K_OP_ROOM			"op_room"
#define K_OP_PARTY			"op_party"
#define K_OP_PARTY_USER		"op_pusr"
#define K_OP_CANCEL			"op_cancel"
#define K_OP_GAME			"op_game"
#define K_OP_READY			"op_ready"
#define K_OP_QUIT			"op_quit"
#define K_CHAIR				"chair"
#define K_TEAM				"team"
#define K_USER_ID			"uid"
#define K_KICK_USER			"kuid"
#define K_AI_ID				"aid"	
#define K_USER_COUNT		"ucnt"
#define K_ORG_ID			"orgid"
#define K_PARTY_ID			"pid"
#define K_PARTY_TOTAL_COUNT	"ptotcnt"	
#define K_CLIENT_TOKEN		"ctoken"
#define K_RESULT			"ret"
#define K_USER_NAME			"uname"
#define K_USER_AVATAR		"uavat"
#define K_BOOL_AI			"b_ai"
#define K_ROOM_OWNER		"rowner"
#define K_PARTY_OWNER		"powner"
#define K_BOOL_INVITE		"b_inv"
#define K_BOOL_TIMEOUT		"btimeout"
#define K_BOOL_SELF			"bself"
#define K_STATE				"state"
#define K_REASON			"reason"
#define K_REQ_UUID			"ruuid"
#define K_META_LINE			"metal"
#define K_AI_USERS			"aiuser"
#define K_TEAMA_USERS		"ausers"
#define K_TEAMB_USERS		"busers"
#define K_TEAMC_USERS		"cusers"
#define K_TEAM_VISIT_USERS	"vusers"	
#define K_RANDOM			"random"
#define K_ROOM_KEY			"room_key"
#define K_AI_COUNT			"aicnt"
#define K_BOOL_NOTIFY		"bntf"
#define K_INVITE_USER		"inviteuser"	
#define K_GAMESVR_IP		"ip"
#define K_GAMESVR_PORT		"port"
#define K_TIME				"time"
#define K_SIGNATURE			"signature"

#define K_RESERVED1			"reserved1"
#define K_SUB_DATA_LIST     "sub_data_list"
#define K_DATA_LIST         "data_list"
#define K_PARA_LIST         "para_list"
#define K_USER_LIST         "user_list"
#define K_REGION_LIST		"r_list"
#define K_PRODUCT_ID		"pro_id"
#define K_GAME_ID			"g_id"
#define K_PARA_OP			"op"
#define K_PARA_SUB_OP		"sop"
#define K_DATA				"data"
#define K_SVR_DATA_LIST     "s_data_list"
#define K_SVR_DATA			"s_data"
#define K_SCORE				"score"
#define K_TYPE				"type"
#define K_KEY				"key"
#define K_VALUE				"cval"
#define K_IVALUE			"ival"
#define K_KV_COUNT			"kvcnt"
#define K_REGION_NAME		"krname"

// file sync
#define FILE_OP             "file_oper"
#define FILE_SIZE           "file_size"
#define FILE_NAME           "file_name"
#define FILE_URL            "file_url"
#define FILE_TOKEN          "file_token"
#define FILE_HASH           "file_hash"
#define FILE_KEY            "file_key"
#define FILE_ID             "file_id"
#define FILE_LIST           "file_list"
#define X_FILE_NAME         "x_file_name"
#define X_TIMESTAMP         "x_timestamp"
#define X_TOKEN             "x_token"
#define FILE_PUTTIME "file_puttime"

// save path
#define SAVEPATH_ROOT       "sp_root"
#define SAVEPATH_DIRECTORY  "sp_directory"
#define SAVEPATH_PATTERN    "sp_pattern"

#define PUBLIC_KEY			"public_key"
#define PRIVATE_KEY			"aes_key"
#define TOKEN				"user_token"
#define RESULT				"result"
#define PUBKEY_VER			"pubkey_ver"
#define GAME_LIST			"game_list"
#define INVALID_GAMES		"invalid_games"
#define ADULT				"adult"
#define WORKINGTIME			"workingtime"
#define LASTTIME			"time"

#define SONKWOWINDOWS		"windows"
#define CPU_FREQUENCY		"cpu_frequency"
#define CPU_MAN				"cpu_manufactureid"
#define CPU_TYPE			"cpuType"
#define MEM_LOAD			"memory_load"
#define MEM_TOTAL			"mem_total_phys"
#define DISK_INFO			"disk_info"
#define VIDEO_INFO			"video_info"
#define ERR_TYPE			"err_type"
#define ERR_DETAILS			"err_details"
#define SYS_INFO			"sysinfo"
#define GAME_DLC			"game_dlc"
#define DLC					"dlc"
#define PRODUCT				"product"
#define LOCATION			"location"
#define FILELIST			"filelist"
#define FILENAME			"filename"
#define TICKET				"ticket"
#define APPID				"app_id"
#define IDENTIFY_ID			"id"
#define ONLINE_OFFLINE_TYPE	"online_offline_type"
#define ONLINES				"onlines"
#define OFFLINES			"offlines"
#define GAME_NAME			"game_name"
#define STEAM_ID			"steam_id"
#define STEAM_APPID			"steam_appid"
#define ACCOUNT_REGION		"account_region"
#define SKU					"sku"
#define PAYMENTID			"paymentid"
#define SUBTOTAL			"sub_total"
#define ENAME				"ename"
#define PHONENUM			"phone_number"
#define WALLETBALANCE		"wallet_balance"
#define ORDERID				"orderid"
#define MESSAGE_CODE		"message_code"
#define AREA				"area"
#define STATUS				"status"
#define PRICE				"price"
#define SK_ACHIEVEMENT_MAX 256
#define SK_ITEM_MAX 256
#define SK_PATH_MAX 1024


#ifdef __cplusplus
}
#endif
