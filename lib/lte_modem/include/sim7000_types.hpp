#pragma once

#include <span>
#include <memory>
#include <string>
#include <array>

namespace axomotor::lte_modem {

namespace internal {

enum class at_cmd_t
{
    NONE        = -1, //

    AT          = 0, // Check command
    A_SLASH     = 1, // Re-issues the Last Command Given
    D           = 2, // Mobile Originated Call to Dial A Number
    E           = 3, // Set Command Echo Mode
    H           = 4, // Disconnect Existing Connection
    I           = 5, // Display Product Identification Information
    L           = 6, // Set Monitor speaker loudness
    M           = 7, // Set Monitor Speaker Mode
    PLUS_TO_3RD = 8, // Switch from Data Mode or PPP Online Mode to Command Mode
    O           = 9, // Switch from Command Mode to Data Mode
    Q           = 10, // Set Result Code Presentation Mode
    S0          = 11, // Set Number of Rings before Automatically Answering the Call
    S3          = 12, // Set Command Line Termination Character
    S4          = 13, // Set Response Formatting Character
    S5          = 14, // Set Command Line Editing Character
    S6          = 15, // Pause Before Blind Dialing
    S7          = 16, // Set Number of Seconds to Wait for Connection Completion
    S8          = 18, // Set Number of Seconds to Wait for Comma Dial Modifier Encountered in Dial D Command
    S10         = 19, // Set Disconnect Delay after Indicating the Absence of Data Carrier
    V           = 20, // TA Response Format
    X           = 21, // Set CONNECT Result Code Format and Monitor Call Progress
    AND_C       = 22, // Set DCD Function Mode
    AND_D       = 23, // Set DTR Function Mode
    AND_E       = 24, // Set CONNECT Result Code Format About Speed
    GCAP        = 25, // Request Complete TA Capabilities List
    GMI         = 26, // Request Manufacturer Identification
    GMM         = 27, // Request TA Model Identification
    GMR         = 28, // Request TA Revision Identification of Software Release
    GOI         = 29, // Request Global Object Identification
    GSN         = 30, // Request TA Serial Number Identification (IMEI)
    ICF         = 31, // Set TE-TA Control Character Framing
    IFC         = 32, // Set TE-TA Local Data Flow Control
    IPR         = 33, // Set TE-TA Fixed Local Rate

    CGMI        = 101, // Request Manufacturer Identification
    CGMM        = 102, // Request Model Identification
    CGMR        = 103, // Request TA Revision Identification of Software Release
    CGSN        = 104, // Request Product Serial Number Identification
    CSCS        = 105, // Select TE Character Set
    CIMI        = 106, // Request International Mobile Subscriber Identity
    CLCK        = 107, // Facility Lock
    CMEE        = 108, // Report Mobile Equipment Error
    COPS        = 109, // Operator Selection
    CPAS        = 110, // Phone Activity Status
    CPIN        = 111, // Enter PIN
    CPWD        = 112, // Change Password
    CRC         = 113, // Set Cellular Result Codes for Incoming Call Indication
    CREG        = 114, // Network Registration
    CRSM        = 115, // Restricted SIM Access
    CSQ         = 116, // Signal Quality Report
    CPOL        = 117, // Preferred Operator List
    COPN        = 118, // Read Operator Names
    CFUN        = 119, // Set Phone Functionality
    CCLK        = 120, // Clock
    CSIM        = 121, // Generic SIM Access
    CBC         = 122, // Battery Charge
    CUSD        = 123, // Unstructured Supplementary Service Data
    CNUM        = 124, // Subscriber Number

    CMGD        = 201, // Delete SMS Message
    CMGF        = 202, // Select SMS Message Fo at
    CMGL        = 203, // List SMS Messages from Preferred Store
    CMGR        = 204, // Read SMS Message
    CMGS        = 205, // Send SMS Message
    CMGW        = 206, // Write SMS Message to Memory
    CMSS        = 207, // Send SMS Message from Storage
    CNMI        = 208, // New SMS Message Indications
    CPMS        = 209, // Preferred SMS Message Storage
    CRES        = 210, // Restore SMS Settings
    CSAS        = 211, // Save SMS Settings
    CSCA        = 212, // SMS Service Center Address
    CSDH        = 213, // Show SMS Text Mode Parameters
    CSMP        = 214, // Set SMS Text Mode Parameters
    CSMS        = 215, // Select Message Service

    CPOWD       = 301, // Power off
    CADC        = 302, // Read ADC
    CFGRI       = 303, // Indicate RI When Using URC
    CLTS        = 304, // Get Local Timestamp
    CBAND       = 305, // Get and Set Mobile Operation Band
    CNSMOD      = 306, // Show Network System Mode
    CSCLK       = 307, // Configure Slow Clock
    CCID        = 308, // Show ICCID
    CDEVICE     = 309, // View Current Flash Device Type
    GSV         = 310, // Display Product Identification Information
    SGPIO       = 311, // Control the GPIO
    SLEDS       = 312, // Set the Timer Period of Net Light
    CNETLIGHT   = 313, // Close the Net Light or Open It to Shining
    CSGS        = 314, // Netlight Indication of GPRS Status
    CGPIO       = 315, // Control the GPIO by PIN Index
    CBATCHK     = 316, // Set VBAT Checking Feature ON/OFF
    CNMP        = 317, // Preferred Mode Selection
    CMNB        = 318, // Preferred Selection between CAT-M and NB-IoT
    CPSMS       = 319, // Power Saving Mode Setting
    CEDRXS      = 320, // Extended-DRX Setting
    CPSI        = 321, // Inquiring UE System Information
    CGNAPN      = 322, // Get Network APN in CAT-M Or NB-IOT
    CSDP        = 323, // Service Domain Preference
    MCELLLOCK   = 324, // Lock the special CAT-M cell
    NCELLLOCK   = 325, // Lock the special NB-IOT cell
    NBSC        = 326, // Configure NB-IOT Scrambling Feature
    CAPNMODE    = 327, // Select the Mode of Application Configure APN
    CRRCSTATE   = 328, // Query RRC State
    CBANDCFG    = 329, // Configure CAT-M Or NB-IOT Band
    CNACT       = 330, // APP Network Active
    CNCFG       = 331, // PDP Configure
    CEDUMP      = 332, // Set Whether the Module Reset When The Module is Crashed
    CNBS        = 333, // Configure Band Scan Optimization For NB-IOT
    CNDS        = 334, // Configure Service Domain Preference For NB-IOT
    CENG        = 335, // Switch On or Off Engineering Mode
    CNACTCFG    = 336, // Configure IP Protocol Type
    CTLIIC      = 337, // Control the Switch of IIC
    CWIIC       = 338, // Write Values to Register of IIC Device
    CRIIC       = 339, // Read Values from Register of IIC Device
    CMCFG       = 340, // Manage Mobile Operator Configuration
    CSIMLOCK    = 341, // SIM Lock
    CRATSRCH    = 342, // Configure Parameter for Better RAT Search
    SPWM        = 343, // Generate the Pulse-Width-Modulation
    CASRIP      = 344, // Show Remote IP address and Port When Received Data
    CEDRX       = 345, // Configure EDRX parameters
    CPSMRDP     = 346, // Read PSM Dynamic Parameters
    CPSMCFG     = 347, // Configure PSM version and Minimum Threshold Value
    CPSMCFGEXT  = 348, // Configure Modem Optimization of PSM
    CPSMSTATUS  = 349, // Enable Deep Sleep Wakeup Indication
    CEDRXRDP    = 350, // eDRX Read Dynamic Parameters
    CRAI        = 351, // Configure Release Assistance Indication in NB-IOT network

    CGATT       = 401, // Attach or Detach from GPRS Service
    CGDCONT     = 402, // Define PDP Context
    CGACT       = 403, // PDP Context Activate or Deactivate
    CGPADDR     = 404, // Show PDP Address
    CGREG       = 405, // Network Registration Status
    CGSMS       = 406, // Select Service for MO SMS Messages
    CEREG       = 407, // EPS Network Registration Status

    SAPBR       = 501, // Bearer Settings for Applications Based on IP

    CIPMUX      = 601, // Start Up Multi-IP Connection
    CIPSTART    = 602, // Start Up TCP or UDP Connection
    CIPSEND     = 603, // Send Data Through TCP or UDP Connection
    CIPQSEND    = 604, // Select Data Transmitting Mode
    CIPACK      = 605, // Query Previous Connection Data Transmitting State
    CIPCLOSE    = 606, // Close TCP or UDP Connection
    CIPSHUT     = 607, // Deactivate GPRS PDP Context
    CLPORT      = 608, // Set Local Port
    CSTT        = 609, // Start Task and Set APN, USER NAME, PASSWORD
    CIICR       = 610, // Bring Up Wireless Connection with GPRS
    CIFSR       = 611, // Get Local IP Address
    CIFSREX     = 612, // Get Local IP Address extend
    CIPSTATUS   = 613, // Query Current Connection Status
    CDNSCFG     = 614, // Configure Domain Name Server
    CDNSGIP     = 615, // Query the IP Address of Given Domain Name
    CIPHEAD     = 616, // Add an IP Head at the Beginning of a Package Received
    CIPATS      = 617, // Set Auto Sending Timer
    CIPSPRT     = 618, // Set Prompt of ‘>’ When Module Sends Data
    CIPSERVER   = 619, // Configure Module as Server
    CIPCSGP     = 620, // Set GPRS for Connection Mode
    CIPSRIP     = 621, // Show Remote IP Address and Port When Received Data
    CIPDPDP     = 622, // Set Whether to Check State of GPRS Network Timing
    CIPMODE     = 623, // Select TCPIP Application Mode
    CIPCCFG     = 624, // Configure Transparent Transfer Mode
    CIPSHOWTP   = 625, // Display Transfer Protocol in IP Head When Received Data
    CIPUDPMODE  = 626, // UDP Extended Mode
    CIPRXGET    = 627, // Get Data from Network Manually
    CIPRDTIMER  = 628, // Set Remote Delay Timer
    CIPSGTXT    = 629, // Select GPRS PDP context
    CIPSENDHEX  = 630, // Set CIPSEND Data Format to Hex
    CIPHEXS     = 631, // Set Output-data Format with suffix
    CIPTKA      = 632, // Set TCP Keepalive Parameters
    CIPOPTION   = 633, // Enable or Disable TCP nagle algorithm

    SHSSL       = 701, // Select SSL Configure
    SHCONF      = 702, // Set HTTP(S) Parameter
    SHCONN      = 703, // HTTP(S) Connection
    SHBOD       = 704, // Set Body
    SHBODEXT    = 705, // Set Extension Body
    SHAHEAD     = 706, // Add Head
    SHCHEAD     = 707, // Clear Head
    SHPARA      = 708, // Set HTTP(S) Para
    SHCPARA     = 709, // Clear HTTP(S) Para
    SHSTATE     = 710, // Query HTTP(S) Connection Status
    SHREQ       = 711, // Set Request Type
    SHREAD      = 712, // Read Response Value
    SHDISC      = 713, // Disconnect HTTP(S)
    HTTPTOFS    = 714, // Download File to AP File System
    HTTPTOFSRL  = 715, // State of Download File to AP File System

    CNTPCID     = 901, // Set GPRS Bearer Profile’s ID
    CNTP        = 902, // Synchronize Network Time

    SMCONF      = 1001, // Set MQTT Parameter
    CSSLCFG     = 1002, // SSL Configure
    SMSSL       = 1003, // Select SSL Configure
    SMCONN      = 1004, // MQTT Connection
    SMPUB       = 1005, // Send Packet
    SMSUB       = 1006, // Subscribe Packet
    SMUNSUB     = 1007, // Unsubscribe Packet
    SMSTATE     = 1008, // Inquire MQTT Connection Status
    SMPUBHEX    = 1009, // Set SMPUB Data Format to Hex
    SMDISC      = 1010, // Disconnection MQTT

    CGNSPWR     = 1201, // GNSS Power Control
    CGNSINF     = 1202, // GNSS Navigation Information Parsed From NMEA Sentences
    CGNSURC     = 1203, // GNSS Navigation URC Report
    CGNSPORT    = 1204, // GNSS NMEA Out Port Set
    CGNSCOLD    = 1205, // GNSS Cold Start
    CGNSWARM    = 1206, // GNSS Warm Start
    CGNSHOT     = 1207, // GNSS Hot Start
    CGNSMOD     = 1208, // GNSS Work Mode Set
    CGNSCFG     = 1209, // GNSS NMEA Out Configure
    CGNSTST     = 1210, // GNSS NMEA Data Output to AT Port
    CGNSXTRA    = 1211, // GNSS XTRA Function Open
    CGNSCPY     = 1212, // GNSS XTRA File Copy
    CGNSRTMS    = 1213, // GNSS NMEA Out Frequency Configure
    CGNSHOR     = 1214, // Configure Positioning Desired Accuracy
    CGNSUTIPR   = 1215, // Configure Baud Rate When NMEA Output from UART3
    CGNSNMEA    = 1216, // Configure NMEA Output Sentences
    CGTP        = 1217, // IZAT GNSS Configure
    CGNSSUPLCFG = 1218, // GNSS SUPL Configure
    CGNSSUPL    = 1219, // GNSS SUPL Control
};

enum class at_cmd_type_t
{
    BASIC,      // AT<x><n>
    S_PARAM,    // AT<n>=<m>
    EXTENDED    // AT+<x>
};

enum class at_cmd_result_t
{
    UNKNOWN,
    OK,
    ERROR,
    CME_ERROR,
    CMS_ERROR,
    NO_ANSWER,
    BUFFER_OVF
};

enum class cme_error_code_t
{
    NO_CODE = -1,
    PHONE_FAILURE = 0,
    NO_CONNECTION_TO_PHONE = 1,
    PHONE_ADAPTOR_LINK_RESERVED = 2,
    OPERATION_NOT_ALLOWED = 3,
    OPERATION_NOT_SUPPORTED = 4,
    PH_SIM_PIN_REQUIRED = 5,
    PH_FSIM_PIN_REQUIRED = 6,
    PH_FSIM_PUK_REQUIRED = 7,
    SIM_NOT_INSERTED = 10,
    SIM_PIN_REQUIRED = 11,
    SIM_PUK_REQUIRED = 12,
    SIM_FAILURE = 13,
    SIM_BUSY = 14,
    SIM_WRONG = 15,
    INCORRECT_PASSWORD = 16,
    SIM_PIN2_REQUIRED = 17,
    SIM_PUK2_REQUIRED = 18,
    MEMORY_FULL = 20,
    INVALID_INDEX = 21,
    NOT_FOUND = 22,
    MEMORY_FAILURE = 23,
    TEXT_STRING_TOO_LONG = 24,
    INVALID_CHARACTERS_IN_TEXT_STRING = 25,
    DIAL_STRING_TOO_LONG = 26,
    INVALID_CHARACTERS_IN_DIAL_STRING = 27,
    NO_NETWORK_SERVICE = 30,
    NETWORK_TIMEOUT = 31,
    NETWORK_NOT_ALLOWED___EMERGENCY_CALL_ONLY = 32,
    NETWORK_PERSONALIZATION_PIN_REQUIRED = 40,
    NETWORK_PERSONALIZATION_PUK_REQUIRED = 41,
    NETWORK_SUBSET_PERSONALIZATION_PIN_REQUIRED = 42,
    NETWORK_SUBSET_PERSONALIZATION_PUK_REQUIRED = 43,
    SERVICE_PROVIDER_PERSONALIZATION_PIN_REQUIRED = 44,
    SERVICE_PROVIDER_PERSONALIZATION_PUK_REQUIRED = 45,
    CORPORATE_PERSONALIZATION_PIN_REQUIRED = 46,
    CORPORATE_PERSONALIZATION_PUK_REQUIRED = 47,
    RESOURCE_LIMITATION = 99,
    UNKNOWN = 100,
    ILLEGAL_MS = 103,
    ILLEGAL_ME = 106,
    GPRS_SERVICES_NOT_ALLOWED = 107,
    PLMN_NOT_ALLOWED = 111,
    LOCATION_AREA_NOT_ALLOWED = 112,
    ROAMING_NOT_ALLOWED_IN_THIS_LOCATION_AREA = 113,
    SERVICE_OPTION_NOT_SUPPORTED = 132,
    REQUESTED_SERVICE_OPTION_NOT_SUBSCRIBED = 133,
    SERVICE_OPTION_TEMPORARILY_OUT_OF_ORDER = 134,
    UNSPECIFIED_GPRS_ERROR = 148,
    PDP_AUTHENTICATION_FAILURE = 149,
    INVALID_MOBILE_CLASS = 150,
    DNS_RESOLVE_FAILED = 160,
    SOCKET_OPEN_FAILED = 161,
    MMS_TASK_IS_BUSY_NOW = 171,
    THE_MMS_DATA_IS_OVERSIZE = 172,
    THE_OPERATION_IS_OVERTIME = 173,
    THERE_IS_NO_MMS_RECEIVER = 174,
    THE_STORAGE_FOR_ADDRESS_IS_FULL = 175,
    NOT_FIND_THE_ADDRESS = 176,
    THE_CONNECTION_TO_NETWORK_IS_FAILED = 177,
    FAILED_TO_READ_PUSH_MESSAGE = 178,
    THIS_IS_NOT_A_PUSH_MESSAGE = 179,
    GPRS_IS_NOT_ATTACHED = 180,
    TCPIP_STACK_IS_BUSY = 181,
    THE_MMS_STORAGE_IS_FULL = 182,
    THE_BOX_IS_EMPTY = 183,
    FAILED_TO_SAVE_MMS = 184,
    IT_IS_IN_EDIT_MODE = 185,
    IT_IS_NOT_IN_EDIT_MODE = 186,
    NO_CONTENT_IN_THE_BUFFER = 187,
    NOT_FIND_THE_FILE = 188,
    FAILED_TO_RECEIVE_MMS = 189,
    FAILED_TO_READ_MMS = 190,
    NOT_M_NOTIFICATION_IND = 191,
    THE_MMS_ENCLOSURE_IS_FULL = 192,
    UNKNOWN_1 = 193,
    NO_ERROR = 600,
    UNRECOGNIZED_COMMAND = 601,
    RETURN_VALUE_ERROR = 602,
    SYNTAX_ERROR = 603,
    UNSPECIFIED_ERROR = 604,
    DATA_TRANSFER_ALREADY = 605,
    ACTION_ALREADY = 606,
    NOT_AT_CMD = 607,
    MULTI_CMD_TOO_LONG = 608,
    ABORT_COPS = 609,
    NO_CALL_DISC = 610,
    BT_SAP_UNDEFINED = 611,
    BT_SAP_NOT_ACCESSIBLE = 612,
    BT_SAP_CARD_REMOVED = 613,
    AT_NOT_ALLOWED_BY_CUSTOMER = 614,
    MISSING_REQUIRED_CMD_PARAMETER = 753,
    INVALID_SIM_COMMAND = 754,
    INVALID_FILE_ID = 755,
    MISSING_REQUIRED_P1_2_3_PARAMETER = 756,
    INVALID_P1_2_3_PARAMETER = 757,
    MISSING_REQUIRED_COMMAND_DATA = 758,
    INVALID_CHARACTERS_IN_COMMAND_DATA = 759,
    INVALID_INPUT_VALUE = 765,
    UNSUPPORTED_MODE = 766,
    OPERATION_FAILED = 767,
    MUX_ALREADY_RUNNING = 768,
    UNABLE_TO_GET_CONTROL = 769,
    SIM_NETWORK_REJECT = 770,
    CALL_SETUP_IN_PROGRESS = 771,
    SIM_POWERED_DOWN = 772,
    SIM_FILE_NOT_PRESENT = 773,
    PARAM_COUNT_NOT_ENOUGH = 791,
    PARAM_COUNT_BEYOND = 792,
    PARAM_VALUE_RANGE_BEYOND = 793,
    PARAM_TYPE_NOT_MATCH = 794,
    PARAM_FORMAT_INVALID = 795,
    GET_A_NULL_PARAM = 796,
    CFUN_STATE_IS_0_OR_4 = 797
};

enum class cms_error_code_t
{
    NO_CODE = -1,
    UNASSIGNED_NUMBER = 1,
    NO_ROUTE_TO_DESTINATION = 3,
    CHANNEL_UNACCEPTABLE = 6,
    OPERATOR_DETERMINED_BARRING = 8,
    CALL_BARRED = 10,
    RESERVED = 11,
    NORMAL_CALL_CLEARING = 16,
    USER_BUSY = 17,
    NO_USER_RESPONDING = 18,
    USER_ALERTING = 19,
    SHORT_MESSAGE_TRANSFER_REJECTED = 21,
    NUMBER_CHANGED = 22,
    PRE_EMPTION = 25,
    NON_SELECTED_USER_CLEARING = 26,
    DESTINATION_OUT_OF_SERVICE = 27,
    INVALID_NUMBER_FORMAT = 28,
    FACILITY_REJECTED = 29,
    RESPONSE_TO_STATUS_ENQUIRY = 30,
    NORMAL = 32,
    NO_CIRCUIT_CHANNEL_AVAILABLE = 34,
    NETWORK_OUT_OF_ORDER = 38,
    TEMPORARY_FAILURE = 41,
    SWITCHING_EQUIPMENT_CONGESTION = 42,
    ACCESS_INFORMATION_DISCARDED = 43,
    REQUESTED_CIRCUIT_CHANNEL_NOT_AVAILABLE = 44,
    RESOURCES_UNAVAILABLE = 47,
    QUALITY_OF_SERVICE_UNAVAILABLE = 49,
    REQUESTED_FACILITY_NOT_SUBSCRIBED = 50,
    REQUESTED_FACILITY_NOT_SUBSCRIBED_1 = 55,
    BEARER_CAPABILITY_NOT_AUTHORIZED = 57,
    BEARER_CAPABILITY_NOT_PRESENTLY_AVAILABLE = 58,
    SERVICE_OR_OPTION_NOT_AVAILABLE = 63,
    BEARER_SERVICE_NOT_IMPLEMENTED = 65,
    ACM_EQUAL_OR_GREATER_THAN_ACM_MAXIMUM = 68,
    REQUESTED_FACILITY_NOT_IMPLEMENTED = 69,
    ONLY_RESTRICTED_DIGITAL_INFORMATION_BEARER_CAPABILITY_IS_AVAILABLE = 70,
    SERVICE_OR_OPTION_NOT_IMPLEMENTED = 79,
    INVALID_TRANSACTION_IDENTIFIER_VALUE    = 81,
    USER_NOT_MEMBER_OF_CUG  = 87,
    INCOMPATIBLE_DESTINATION    = 88,
    INVALID_TRANSIT_NETWORK_SELECTION = 91,
    SEMANTICALLY_INCORRECT_MESSAGE = 95,
    INVALID_MANDATORY_INFORMATION = 96,
    MESSAGE_TYPE_NON_EXISTENT_OR_NOT_IMPLEMENTED = 97,
    MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE = 98,
    INFORMATION_ELEMENT_NON_EXISTENT_OR_NOT_IMPLEMENTED = 99,
    CONDITIONAL_INFORMATION_ELEMENT_ERROR = 100,
    MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL = 101,
    RECOVERY_ON_TIMER_EXPIRY = 102,
    PROTOCOL_ERROR = 111,
    INTERWORKING = 127,
    TELEMATIC_INTERWORKING_NOT_SUPPORTED = 128,
    SHORT_MESSAGE_TYPE_0_NOT_SUPPORTED = 129,
    CANNOT_REPLACE_SHORT_MESSAGE = 130,
    UNSPECIFIED_TP_PID_ERROR = 143,
    DATA_CODING_SCHEME_NOT_SUPPORTED = 144,
    MESSAGE_CLASS_NOT_SUPPORTED = 145,
    UNSPECIFIED_TP_DCS_ERROR = 159,
    COMMAND_CANNOT_BE_ACTED = 160,
    COMMAND_UNSUPPORTED = 161,
    UNSPECIFIED_TP_COMMAND_ERROR = 175,
    TPDU_NOT_SUPPORTED = 176,
    SC_BUSY = 192,
    NO_SC_SUBSCRIPTION = 193,
    SC_SYSTEM_FAILURE = 194,
    INVALID_SME_ADDRESS = 195,
    DESTINATION_SME_BARRED = 196,
    SM_REJECTED_DUPLICATE_SM = 197,
    TP_VPF_NOT_SUPPORTED = 198,
    TP_VP_NOT_SUPPORTED = 199,
    SIM_SMS_STORAGE_FULL = 208,
    NO_SMS_STORAGE_CAPABILITY_IN_SIM = 209,
    ERROR_IN_MS = 210,
    MEMORY_CAPACITY_EXCEEDED = 211,
    SIM_APPLICATION_TOOLKIT_BUSY = 212,
    SIM_DATA_DOWNLOAD_ERROR = 213,
    CP_RETRY_EXCEED = 224,
    RP_TRIM_TIMEOUT = 225,
    SMS_CONNECTION_BROKEN = 226,
    UNSPECIFIED_ERROR_CAUSE = 255,
    ME_FAILURE = 300,
    SMS_RESERVED = 301,
    OPERATION_NOT_ALLOWED = 302,
    OPERATION_NOT_SUPPORTED = 303,
    INVALID_PDU_MODE = 304,
    INVALID_TEXT_MODE = 305,
    SIM_NOT_INSERTED = 310,
    SIM_PIN_NECESSARY = 311,
    PH_SIM_PIN_NECESSARY = 312,
    SIM_FAILURE = 313,
    SIM_BUSY = 314,
    SIM_WRONG = 315,
    SIM_PUK_REQUIRED = 316,
    SIM_PIN2_REQUIRED = 317,
    SIM_PUK2_REQUIRED = 318,
    MEMORY_FAILURE = 320,
    INVALID_MEMORY_INDEX = 321,
    MEMORY_FULL = 322,
    INVALID_INPUT_PARAMETER = 323,
    INVALID_INPUT_FORMAT = 324,
    INVALID_INPUT_VALUE = 325,
    SMSC_ADDRESS_UNKNOWN = 330,
    NO_NETWORK = 331,
    NETWORK_TIMEOUT = 332,
    NO_CNMA_ACK = 340,
    UNKNOWN = 500,
    SMS_NO_ERROR = 512,
    MESSAGE_LENGTH_EXCEEDS_MAXIMUM_LENGTH = 513,
    INVALID_REQUEST_PARAMETERS = 514,
    ME_STORAGE_FAILURE = 515,
    INVALID_BEARER_SERVICE = 516,
    INVALID_SERVICE_MODE = 517,
    INVALID_STORAGE_TYPE = 518,
    INVALID_MESSAGE_FORMAT = 519,
    TOO_MANY_MO_CONCATENATED_MESSAGES = 520,
    SMSAL_NOT_READY = 521,
    SMSAL_NO_MORE_SERVICE = 522,
    NOT_SUPPORT_TP_STATUS_REPORT_AND_TP_COMMAND_IN_STORAGE = 523,
    RESERVED_MTI = 524,
    NO_FREE_ENTITY_IN_RL_LAYER = 525,
    THE_PORT_NUMBER_IS_ALREADY_REGISTERRED = 526,
    THERE_IS_NO_FREE_ENTITY_FOR_PORT_NUMBER = 527,
    MORE_MESSAGE_TO_SEND_STATE_ERROR = 528,
    MO_SMS_IS_NOT_ALLOW = 529,
    GPRS_IS_SUSPENDED = 530,
    ME_STORAGE_FULL = 531,
    DOING_SIM_REFRESH = 532
};

enum class parse_state_t
{
    WAITING_HEADER,
    IN_BODY,
    WAITING_TERMINATOR,
    COMPLETED,
    ERROR
};

struct at_cmd_def_t
{
    at_cmd_t command;
    at_cmd_type_t type;
    const char *string;
    int max_response_time; // 0 = indefined
};

struct result_code_def_t 
{
    at_cmd_result_t code;
    const char *string;
};

struct sim7000_cmd_result_info_t
{
    at_cmd_result_t result;
    int error_code;
    std::string response;
    
    void reset() 
    {
        result = at_cmd_result_t::UNKNOWN;
        error_code = -1;
        response.clear();
    }
};

struct sim7000_cmd_context_t
{
    sim7000_cmd_context_t() { reset(); }

    at_cmd_t command;
    bool is_raw;
    bool is_partial;
    bool ignore_response;
    bool response_received;

    void reset()
    {
        command = at_cmd_t::NONE;
        is_raw = false;
        is_partial = false;
        ignore_response = false;
        response_received = false;
    }
};

struct sim7000_status_t
{
    bool is_echo_disabled : 1;
    bool is_grps_active : 1;
    bool is_tcp_active : 1;
    bool is_ip_active : 1;
    bool is_active : 1;

    bool is_gnss_turned_on : 1;
    bool is_gnss_urc_enabled : 1;
    bool is_mqtt_enabled : 1;
};

const at_cmd_def_t *get_command_def(at_cmd_t command);

const result_code_def_t *get_result_code_def(at_cmd_result_t code);

} // namespace internal

/* Estructuras de datos de alto nivel */

enum class sim_status_t
{
    READY,
    PIN_REQUIRED,
    PUK_REQUIRED,
    PHONE_REQUIRED
};

enum class signal_strength_t 
{
    NOT_DETECTABLE,
    MARGINAL,
    GOOD,
    EXCELLENT,
};

enum class network_reg_status_t 
{
    UNKNOWN,
    NOT_REGISTERED,
    REGISTERED,
    TRYING_TO_REGISTER,
    REGISTRATION_DENIED,
    REGISTERED_ROAMING,
};

enum class connection_status_t
{
    UNKNOWN,
    IP_INITIAL,
    IP_START,
    IP_CONFIG,
    IP_GPRSACT,
    IP_STATUS,
    CONNECTING,
    CONNECT_OK,
    CLOSING,
    CLOSED,
    PDP_DEACT,
};

enum class bearer_status_t
{
    UNKNOWN,
    CONNECTING,
    CONNECTED,
    CLOSING,
    CLOSED,
};

enum class network_active_status_t
{
    UNKNOWN,
    DEACTIVED,
    ACTIVED,
    IN_OPERATION,
};

enum class network_active_mode_t
{
    UNKNOWN,
    DEACTIVE,
    ACTIVE,
    AUTO_ACTIVE,
};

enum class operator_netact_t
{
    UNKNOWN,
    USER_SPECIFIED,
    GSM_COMPACT,
    GSM_EGPRS,
    LTE_M1_A_GB,
    LTE_NB_S1
};

struct apn_config_t
{
    char apn[64];
    char user[32];
    char pwd[32];
    uint8_t cid;
};

struct gnss_nav_info_t 
{
    uint64_t date_time; // 20250627222325
    float latitude;
    float longitude;
    float msl_altitude;
    float speed_over_ground;
    float course_over_ground;
    uint8_t gnss_satellites;
    uint8_t gps_satellites;
    uint8_t run_status      : 1;
    uint8_t fix_status      : 1;
    uint8_t fix_mode        : 2;
};

struct mqtt_config_t
{
    char client_id[32];
    char broker[64];
    char username[32];
    char password[32];
    uint32_t port;
    uint16_t keep_time;
    bool session_cleaning;
    uint8_t qos;
};

} // namespace axomotor::lte_mode
