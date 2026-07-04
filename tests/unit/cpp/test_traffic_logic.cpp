#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest.h"
#include "traffic_logic.h"

using namespace traffic_logic;

// ---------------------------------------------------------------------------
// Plate format validation
// ---------------------------------------------------------------------------
TEST_CASE("is_valid_plate accepts well-formed plates") {
    CHECK(is_valid_plate("L-AB-1234"));
    CHECK(is_valid_plate("A-B-1"));
    CHECK(is_valid_plate("ABC-AB-9999"));
}

TEST_CASE("is_valid_plate rejects malformed plates") {
    CHECK_FALSE(is_valid_plate(""));
    CHECK_FALSE(is_valid_plate("lowercase-ab-1234"));
    CHECK_FALSE(is_valid_plate("ABCD-AB-1234"));   // 4 letters in first group (max 3)
    CHECK_FALSE(is_valid_plate("AB-ABC-1234"));    // 3 letters in second group (max 2)
    CHECK_FALSE(is_valid_plate("AB-CD-12345"));    // 5 digits (max 4)
    CHECK_FALSE(is_valid_plate("AB CD 1234"));     // wrong separator
    CHECK_FALSE(is_valid_plate("-AB-1234"));       // missing first group
}

// ---------------------------------------------------------------------------
// Email format validation
// ---------------------------------------------------------------------------
TEST_CASE("is_valid_email accepts well-formed addresses") {
    CHECK(is_valid_email("kaif@example.com"));
    CHECK(is_valid_email("a.b+c@sub.example.co.uk"));
}

TEST_CASE("is_valid_email rejects malformed addresses") {
    CHECK_FALSE(is_valid_email(""));
    CHECK_FALSE(is_valid_email("no-at-sign.com"));
    CHECK_FALSE(is_valid_email("missing-domain@"));
    CHECK_FALSE(is_valid_email("spaces in@email.com"));
}

// ---------------------------------------------------------------------------
// Violation type / severity validation
// ---------------------------------------------------------------------------
TEST_CASE("is_valid_violation_type only accepts known types") {
    CHECK(is_valid_violation_type("speeding"));
    CHECK(is_valid_violation_type("illegal_parking"));
    CHECK(is_valid_violation_type("red_light"));
    CHECK_FALSE(is_valid_violation_type("jaywalking"));
    CHECK_FALSE(is_valid_violation_type(""));
}

TEST_CASE("is_valid_severity only accepts known severities for a known type") {
    CHECK(is_valid_severity("speeding", "low"));
    CHECK(is_valid_severity("speeding", "medium"));
    CHECK(is_valid_severity("speeding", "high"));
    CHECK_FALSE(is_valid_severity("speeding", "extreme"));
    CHECK_FALSE(is_valid_severity("unknown_type", "low"));
}

// ---------------------------------------------------------------------------
// Fine amount lookup
// ---------------------------------------------------------------------------
TEST_CASE("get_fine_amount matches the documented severity table") {
    CHECK(get_fine_amount("speeding", "low") == doctest::Approx(50.0));
    CHECK(get_fine_amount("speeding", "medium") == doctest::Approx(120.0));
    CHECK(get_fine_amount("speeding", "high") == doctest::Approx(250.0));

    CHECK(get_fine_amount("illegal_parking", "low") == doctest::Approx(20.0));
    CHECK(get_fine_amount("illegal_parking", "medium") == doctest::Approx(50.0));
    CHECK(get_fine_amount("illegal_parking", "high") == doctest::Approx(100.0));

    CHECK(get_fine_amount("red_light", "low") == doctest::Approx(80.0));
    CHECK(get_fine_amount("red_light", "medium") == doctest::Approx(150.0));
    CHECK(get_fine_amount("red_light", "high") == doctest::Approx(300.0));
}

TEST_CASE("fine amounts strictly increase with severity for every violation type") {
    for (const auto& [type, severities] : FINE_TABLE) {
        double low = severities.at("low");
        double medium = severities.at("medium");
        double high = severities.at("high");
        CHECK(low < medium);
        CHECK(medium < high);
    }
}

// ---------------------------------------------------------------------------
// Congestion threshold logic
// ---------------------------------------------------------------------------
TEST_CASE("is_congested flags counts strictly above the threshold") {
    CHECK_FALSE(is_congested(0));
    CHECK_FALSE(is_congested(CONGESTION_THRESHOLD));       // exactly at threshold: not congested
    CHECK(is_congested(CONGESTION_THRESHOLD + 1));
}

TEST_CASE("is_predicted_congested flags historical averages strictly above the threshold") {
    CHECK_FALSE(is_predicted_congested(0.0));
    CHECK_FALSE(is_predicted_congested(static_cast<double>(CONGESTION_THRESHOLD)));
    CHECK(is_predicted_congested(CONGESTION_THRESHOLD + 0.1));
}

// ---------------------------------------------------------------------------
// Emergency vehicle alert logic
// ---------------------------------------------------------------------------
TEST_CASE("is_emergency_alert_allowed only allows vehicles flagged as emergency") {
    CHECK(is_emergency_alert_allowed(true));
    CHECK_FALSE(is_emergency_alert_allowed(false));
}
