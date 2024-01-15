#ifndef WIFI_REASON_LOOKUP_H_
#define WIFI_REASON_LOOKUP_H_

#include <esp_wifi_types.h>

// Range 1:
//     WIFI_REASON_UNSPECIFIED                        = 1,
//     WIFI_REASON_AUTH_EXPIRE                        = 2,
//     WIFI_REASON_AUTH_LEAVE                         = 3,
//     WIFI_REASON_ASSOC_EXPIRE                       = 4,
//     WIFI_REASON_ASSOC_TOOMANY                      = 5,
//     WIFI_REASON_NOT_AUTHED                         = 6,
//     WIFI_REASON_NOT_ASSOCED                        = 7,
//     WIFI_REASON_ASSOC_LEAVE                        = 8,
//     WIFI_REASON_ASSOC_NOT_AUTHED                   = 9,
//     WIFI_REASON_DISASSOC_PWRCAP_BAD                = 10,
//     WIFI_REASON_DISASSOC_SUPCHAN_BAD               = 11,
//     WIFI_REASON_BSS_TRANSITION_DISASSOC            = 12,
//     WIFI_REASON_IE_INVALID                         = 13,
//     WIFI_REASON_MIC_FAILURE                        = 14,
//     WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT             = 15,
//     WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT           = 16,
//     WIFI_REASON_IE_IN_4WAY_DIFFERS                 = 17,
//     WIFI_REASON_GROUP_CIPHER_INVALID               = 18,
//     WIFI_REASON_PAIRWISE_CIPHER_INVALID            = 19,
//     WIFI_REASON_AKMP_INVALID                       = 20,
//     WIFI_REASON_UNSUPP_RSN_IE_VERSION              = 21,
//     WIFI_REASON_INVALID_RSN_IE_CAP                 = 22,
//     WIFI_REASON_802_1X_AUTH_FAILED                 = 23,
//     WIFI_REASON_CIPHER_SUITE_REJECTED              = 24,
//     WIFI_REASON_TDLS_PEER_UNREACHABLE              = 25,
//     WIFI_REASON_TDLS_UNSPECIFIED                   = 26,
//     WIFI_REASON_SSP_REQUESTED_DISASSOC             = 27,
//     WIFI_REASON_NO_SSP_ROAMING_AGREEMENT           = 28,
//     WIFI_REASON_BAD_CIPHER_OR_AKM                  = 29,
//     WIFI_REASON_NOT_AUTHORIZED_THIS_LOCATION       = 30,
//     WIFI_REASON_SERVICE_CHANGE_PERCLUDES_TS        = 31,
//     WIFI_REASON_UNSPECIFIED_QOS                    = 32,
//     WIFI_REASON_NOT_ENOUGH_BANDWIDTH               = 33,
//     WIFI_REASON_MISSING_ACKS                       = 34,
//     WIFI_REASON_EXCEEDED_TXOP                      = 35,
//     WIFI_REASON_STA_LEAVING                        = 36,
//     WIFI_REASON_END_BA                             = 37,
//     WIFI_REASON_UNKNOWN_BA                         = 38,
//     WIFI_REASON_TIMEOUT                            = 39,

// Range 2:
//     WIFI_REASON_PEER_INITIATED                     = 46,
//     WIFI_REASON_AP_INITIATED                       = 47,
//     WIFI_REASON_INVALID_FT_ACTION_FRAME_COUNT      = 48,
//     WIFI_REASON_INVALID_PMKID                      = 49,
//     WIFI_REASON_INVALID_MDE                        = 50,
//     WIFI_REASON_INVALID_FTE                        = 51,

// Range 3:
//     WIFI_REASON_TRANSMISSION_LINK_ESTABLISH_FAILED = 67,
//     WIFI_REASON_ALTERATIVE_CHANNEL_OCCUPIED        = 68,


// Range 4:
//     WIFI_REASON_BEACON_TIMEOUT                     = 200,
//     WIFI_REASON_NO_AP_FOUND                        = 201,
//     WIFI_REASON_AUTH_FAIL                          = 202,
//     WIFI_REASON_ASSOC_FAIL                         = 203,
//     WIFI_REASON_HANDSHAKE_TIMEOUT                  = 204,
//     WIFI_REASON_CONNECTION_FAIL                    = 205,
//     WIFI_REASON_AP_TSF_RESET                       = 206,
//     WIFI_REASON_ROAMING                            = 207,
//     WIFI_REASON_ASSOC_COMEBACK_TIME_TOO_LONG       = 208,
//     WIFI_REASON_SA_QUERY_TIMEOUT                   = 209,

namespace cwifi {
#if defined(ENABLE_LOGGING)
    inline static constexpr const char * const REASONS_1[WIFI_REASON_ALTERATIVE_CHANNEL_OCCUPIED+1] = {
        "WIFI_REASON_UNSPECIFIED",
        "WIFI_REASON_AUTH_EXPIRE",
        "WIFI_REASON_AUTH_LEAVE",
        "WIFI_REASON_ASSOC_EXPIRE",
        "WIFI_REASON_ASSOC_TOOMANY",
        "WIFI_REASON_NOT_AUTHED",
        "WIFI_REASON_NOT_ASSOCED",
        "WIFI_REASON_ASSOC_LEAVE",
        "WIFI_REASON_ASSOC_NOT_AUTHED",
        "WIFI_REASON_DISASSOC_PWRCAP_BAD",
        "WIFI_REASON_DISASSOC_SUPCHAN_BAD",
        "WIFI_REASON_BSS_TRANSITION_DISASSOC",
        "WIFI_REASON_IE_INVALID",
        "WIFI_REASON_MIC_FAILURE",
        "WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT",
        "WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT",
        "WIFI_REASON_IE_IN_4WAY_DIFFERS",
        "WIFI_REASON_GROUP_CIPHER_INVALID",
        "WIFI_REASON_PAIRWISE_CIPHER_INVALID",
        "WIFI_REASON_AKMP_INVALID",
        "WIFI_REASON_UNSUPP_RSN_IE_VERSION",
        "WIFI_REASON_INVALID_RSN_IE_CAP",
        "WIFI_REASON_802_1X_AUTH_FAILED",
        "WIFI_REASON_CIPHER_SUITE_REJECTED",
        "WIFI_REASON_TDLS_PEER_UNREACHABLE",
        "WIFI_REASON_TDLS_UNSPECIFIED",
        "WIFI_REASON_SSP_REQUESTED_DISASSOC",
        "WIFI_REASON_NO_SSP_ROAMING_AGREEMENT",
        "WIFI_REASON_BAD_CIPHER_OR_AKM",
        "WIFI_REASON_NOT_AUTHORIZED_THIS_LOCATION",
        "WIFI_REASON_SERVICE_CHANGE_PERCLUDES_TS",
        "WIFI_REASON_UNSPECIFIED_QOS",
        "WIFI_REASON_NOT_ENOUGH_BANDWIDTH",
        "WIFI_REASON_MISSING_ACKS",
        "WIFI_REASON_EXCEEDED_TXOP",
        "WIFI_REASON_STA_LEAVING",
        "WIFI_REASON_END_BA",
        "WIFI_REASON_UNKNOWN_BA",
        "WIFI_REASON_TIMEOUT",
    };
    
    inline static constexpr const char * const REASONS_2[WIFI_REASON_INVALID_FTE-WIFI_REASON_PEER_INITIATED+1] = {
        "WIFI_REASON_PEER_INITIATED",
        "WIFI_REASON_AP_INITIATED",
        "WIFI_REASON_INVALID_FT_ACTION_FRAME_COUNT",
        "WIFI_REASON_INVALID_PMKID",
        "WIFI_REASON_INVALID_MDE",
        "WIFI_REASON_INVALID_FTE",
    };
    
    inline static constexpr const char * const REASONS_3[WIFI_REASON_ALTERATIVE_CHANNEL_OCCUPIED-WIFI_REASON_TRANSMISSION_LINK_ESTABLISH_FAILED+1] = {
        "WIFI_REASON_TRANSMISSION_LINK_ESTABLISH_FAILED",
        "WIFI_REASON_ALTERATIVE_CHANNEL_OCCUPIED",
    };
    
    inline static constexpr const char * const REASONS_4[WIFI_REASON_SA_QUERY_TIMEOUT-WIFI_REASON_BEACON_TIMEOUT+1] = {
        "WIFI_REASON_BEACON_TIMEOUT",
        "WIFI_REASON_NO_AP_FOUND",
        "WIFI_REASON_AUTH_FAIL",
        "WIFI_REASON_ASSOC_FAIL",
        "WIFI_REASON_HANDSHAKE_TIMEOUT",
        "WIFI_REASON_CONNECTION_FAIL",
        "WIFI_REASON_AP_TSF_RESET",
        "WIFI_REASON_ROAMING",
        "WIFI_REASON_ASSOC_COMEBACK_TIME_TOO_LONG",
        "WIFI_REASON_SA_QUERY_TIMEOUT",
    };
#endif

    const char * wifi_reason_lookup(uint8_t reason) {
#if defined(ENABLE_LOGGING)
        if((reason >= WIFI_REASON_UNSPECIFIED && reason <= WIFI_REASON_TIMEOUT)) {
            return REASONS_1[reason - WIFI_REASON_UNSPECIFIED];
        } else if (reason >= WIFI_REASON_PEER_INITIATED && reason <= WIFI_REASON_INVALID_FTE) {
            return REASONS_2[reason - WIFI_REASON_PEER_INITIATED];
        } else if (reason >= WIFI_REASON_TRANSMISSION_LINK_ESTABLISH_FAILED && reason <= WIFI_REASON_ALTERATIVE_CHANNEL_OCCUPIED) {
            return REASONS_3[reason - WIFI_REASON_TRANSMISSION_LINK_ESTABLISH_FAILED];
        } else if (reason >= WIFI_REASON_BEACON_TIMEOUT && reason <= WIFI_REASON_SA_QUERY_TIMEOUT) {
            return REASONS_4[reason - WIFI_REASON_BEACON_TIMEOUT];
        }
#endif        
        return "UNKNOWN";
    }
}

#endif // WIFI_REASON_LOOKUP_H_