#pragma once

// Pure, DB-free business logic shared by main.cpp and the unit test binary
// (tests/unit/cpp/test_traffic_logic.cpp). Nothing in this header touches
// pqxx, crow::request/response, or any I/O, so it can be unit tested in
// isolation without a live Postgres instance or HTTP server.

#include <string>
#include <map>
#include <regex>

namespace traffic_logic {

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------
inline constexpr int CONGESTION_WINDOW_MINUTES = 15;
inline constexpr int CONGESTION_THRESHOLD = 20;      // vehicles within the window
inline constexpr int EMERGENCY_LOOKBACK_MINUTES = 10; // "recently logged" drivers to notify

// Fine amount lookup table: violation_type -> severity -> fine amount.
inline const std::map<std::string, std::map<std::string, double>> FINE_TABLE = {
    {"speeding",        {{"low", 50.0},  {"medium", 120.0}, {"high", 250.0}}},
    {"illegal_parking", {{"low", 20.0},  {"medium", 50.0},  {"high", 100.0}}},
    {"red_light",       {{"low", 80.0},  {"medium", 150.0}, {"high", 300.0}}}
};

inline const std::regex PLATE_FORMAT("^[A-Z]{1,3}-[A-Z]{1,2}-[0-9]{1,4}$");
inline const std::regex EMAIL_FORMAT("^[^\\s@]+@[^\\s@]+\\.[^\\s@]+$");

// ---------------------------------------------------------------------------
// Validation / lookup helpers
// ---------------------------------------------------------------------------

inline bool is_valid_email(const std::string& email) {
    return std::regex_match(email, EMAIL_FORMAT);
}

inline bool is_valid_plate(const std::string& plate) {
    return std::regex_match(plate, PLATE_FORMAT);
}

inline bool is_valid_violation_type(const std::string& type) {
    return FINE_TABLE.find(type) != FINE_TABLE.end();
}

inline bool is_valid_severity(const std::string& type, const std::string& severity) {
    auto it = FINE_TABLE.find(type);
    if (it == FINE_TABLE.end()) return false;
    return it->second.find(severity) != it->second.end();
}

inline double get_fine_amount(const std::string& type, const std::string& severity) {
    return FINE_TABLE.at(type).at(severity);
}

// Congestion logic: a junction is congested if its recent vehicle count
// exceeds CONGESTION_THRESHOLD. Predicted congestion uses the same
// threshold against a historical hourly average.
inline bool is_congested(long recent_count) {
    return recent_count > CONGESTION_THRESHOLD;
}

inline bool is_predicted_congested(double historical_avg_for_hour) {
    return historical_avg_for_hour > CONGESTION_THRESHOLD;
}

// Emergency alert logic: only vehicles flagged is_emergency=true at
// registration may raise an alert. Caller is expected to have already
// confirmed the vehicle is registered before calling this.
inline bool is_emergency_alert_allowed(bool vehicle_is_emergency) {
    return vehicle_is_emergency;
}

} // namespace traffic_logic
