#include <gtest/gtest.h>

#include <wifi_reason_lookup.h>

TEST(TestCWifi, TestWifiReasonLookup) {
    using namespace cwifi;

    EXPECT_EQ("WIFI_REASON_UNSPECIFIED", wifi_reason_lookup(WIFI_REASON_UNSPECIFIED)) 
                    << "Invalid WIFI_REASON_UNSPECIFIED";
    EXPECT_EQ("WIFI_REASON_AUTH_EXPIRE", wifi_reason_lookup(WIFI_REASON_AUTH_EXPIRE)) 
                    << "Invalid WIFI_REASON_AUTH_EXPIRE";
    EXPECT_EQ("WIFI_REASON_TIMEOUT", wifi_reason_lookup(WIFI_REASON_TIMEOUT)) 
                    << "Invalid WIFI_REASON_TIMEOUT";


    EXPECT_EQ("WIFI_REASON_PEER_INITIATED", wifi_reason_lookup(WIFI_REASON_PEER_INITIATED)) 
                    << "Invalid WIFI_REASON_PEER_INITIATED";
    EXPECT_EQ("WIFI_REASON_INVALID_FTE", wifi_reason_lookup(WIFI_REASON_INVALID_FTE)) 
                    << "Invalid WIFI_REASON_INVALID_FTE";

    EXPECT_EQ("WIFI_REASON_TRANSMISSION_LINK_ESTABLISH_FAILED", wifi_reason_lookup(WIFI_REASON_TRANSMISSION_LINK_ESTABLISH_FAILED)) 
                    << "Invalid WIFI_REASON_TRANSMISSION_LINK_ESTABLISH_FAILED";
    EXPECT_EQ("WIFI_REASON_ALTERATIVE_CHANNEL_OCCUPIED", wifi_reason_lookup(WIFI_REASON_ALTERATIVE_CHANNEL_OCCUPIED)) 
                    << "Invalid WIFI_REASON_ALTERATIVE_CHANNEL_OCCUPIED";

    EXPECT_EQ("WIFI_REASON_BEACON_TIMEOUT", wifi_reason_lookup(WIFI_REASON_BEACON_TIMEOUT)) 
                    << "Invalid WIFI_REASON_BEACON_TIMEOUT";
    EXPECT_EQ("WIFI_REASON_SA_QUERY_TIMEOUT", wifi_reason_lookup(WIFI_REASON_SA_QUERY_TIMEOUT)) 
                    << "Invalid WIFI_REASON_SA_QUERY_TIMEOUT";

    EXPECT_EQ("UNKNOWN", wifi_reason_lookup(0  )) << "Invalid UNKNOWN";
    EXPECT_EQ("UNKNOWN", wifi_reason_lookup(40 )) << "Invalid UNKNOWN";
    EXPECT_EQ("UNKNOWN", wifi_reason_lookup(45 )) << "Invalid UNKNOWN";
    EXPECT_EQ("UNKNOWN", wifi_reason_lookup(52 )) << "Invalid UNKNOWN";
    EXPECT_EQ("UNKNOWN", wifi_reason_lookup(66 )) << "Invalid UNKNOWN";
    EXPECT_EQ("UNKNOWN", wifi_reason_lookup(69 )) << "Invalid UNKNOWN";
    EXPECT_EQ("UNKNOWN", wifi_reason_lookup(199)) << "Invalid UNKNOWN";
    EXPECT_EQ("UNKNOWN", wifi_reason_lookup(210)) << "Invalid UNKNOWN";
    EXPECT_EQ("UNKNOWN", wifi_reason_lookup(255)) << "Invalid UNKNOWN";
}