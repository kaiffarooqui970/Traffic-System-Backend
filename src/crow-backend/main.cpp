#include "crow_all.h"
#include "traffic_logic.h"
#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <map>
#include <cstdlib>
#include <pqxx/pqxx>

using traffic_logic::CONGESTION_WINDOW_MINUTES;
using traffic_logic::CONGESTION_THRESHOLD;
using traffic_logic::EMERGENCY_LOOKBACK_MINUTES;
using traffic_logic::is_valid_email;
using traffic_logic::is_valid_plate;
using traffic_logic::is_valid_violation_type;
using traffic_logic::is_valid_severity;
using traffic_logic::get_fine_amount;
using traffic_logic::is_congested;
using traffic_logic::is_predicted_congested;
using traffic_logic::is_emergency_alert_allowed;

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

std::string env_or(const char* name, const std::string& fallback) {
    const char* v = std::getenv(name);
    return v ? std::string(v) : fallback;
}

// Builds the libpqxx connection string from environment variables so the
// same binary works locally (falls back to unix socket defaults) and inside
// docker-compose (where PGHOST/PGUSER/PGPASSWORD are set).
std::string get_connection_string() {
    std::string conn = "dbname=" + env_or("PGDATABASE", "traffic_system_db");
    std::string host = env_or("PGHOST", "");
    std::string port = env_or("PGPORT", "");
    std::string user = env_or("PGUSER", "");
    std::string password = env_or("PGPASSWORD", "");
    if (!host.empty()) conn += " host=" + host;
    if (!port.empty()) conn += " port=" + port;
    if (!user.empty()) conn += " user=" + user;
    if (!password.empty()) conn += " password=" + password;
    return conn;
}

crow::response make_error(int code, const std::string& message) {
    crow::json::wvalue error_res;
    error_res["status"] = "error";
    error_res["message"] = message;
    return crow::response(code, error_res);
}

// Stub email sender. No SMTP is configured for this university project, so
// notifications are logged to console and to notifications.log. A real
// integration would replace the body of this function with an SMTP/API call
// and keep the same signature, so callers never need to change.
void send_email_notification(const std::string& to, const std::string& subject, const std::string& body) {
    std::string smtp_host = env_or("SMTP_HOST", "");

    std::ostringstream entry;
    entry << "[EMAIL] To: " << to << " | Subject: " << subject << " | Body: " << body;

    if (smtp_host.empty()) {
        std::cout << entry.str() << std::endl;
        std::ofstream log_file("notifications.log", std::ios::app);
        if (log_file.is_open()) {
            log_file << entry.str() << std::endl;
        }
        return;
    }

    // Real SMTP send would go here, e.g.:
    //   SmtpClient client(smtp_host, env_or("SMTP_PORT", "587"));
    //   client.send(to, subject, body);
    std::cout << "[EMAIL:SMTP would send here] " << entry.str() << std::endl;
}

// Returns true if a junction with this id exists.
bool junction_exists(pqxx::work& W, int junction_id) {
    pqxx::result r = W.exec_params("SELECT 1 FROM junctions WHERE id = $1", junction_id);
    return !r.empty();
}

int main() {
    crow::SimpleApp app;

    // -----------------------------------------------------------------
    // Week 12: Vehicle Registration
    // -----------------------------------------------------------------
    CROW_ROUTE(app, "/api/register_vehicle").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){

        auto body = crow::json::load(req.body);
        if (!body) return make_error(400, "Invalid JSON received");

        std::string plate = "UNKNOWN";
        if (body.has("plate_number")) plate = body["plate_number"].s();

        std::string owner = "UNKNOWN";
        if (body.has("owner_name")) owner = body["owner_name"].s();

        std::string email = "";
        if (body.has("email")) email = body["email"].s();

        std::string type = "Car";
        if (body.has("vehicle_type")) type = body["vehicle_type"].s();

        bool is_emergency = false;
        if (body.has("is_emergency")) is_emergency = body["is_emergency"].b();

        if (!is_valid_plate(plate)) {
            return make_error(400, "Registration failed: Illogical license plate.");
        }
        if (owner.empty() || owner == "UNKNOWN") {
            return make_error(400, "Registration failed: owner_name is required.");
        }
        if (!email.empty() && !is_valid_email(email)) {
            return make_error(400, "Registration failed: invalid email format.");
        }

        try {
            pqxx::connection C(get_connection_string());
            if (C.is_open()) {
                pqxx::work W(C);
                std::string sql = "INSERT INTO vehicles (plate_number, owner_name, email, vehicle_type, is_emergency) "
                                   "VALUES ($1, $2, $3, $4, $5)";
                W.exec_params(sql, plate, owner, email, type, is_emergency);
                W.commit();
                std::cout << "Successfully saved to Database: " << plate << std::endl;
            }
        } catch (const std::exception &e) {
            std::cerr << "Database error: " << e.what() << std::endl;
            return make_error(500, "Failed to save to database. Is PostgreSQL running?");
        }

        crow::json::wvalue res;
        res["status"] = "success";
        res["message"] = "Vehicle registered and saved to database!";
        res["registered_plate"] = plate;
        return crow::response(200, res);
    });

    // -----------------------------------------------------------------
    // Week 12: Junction plate logging / recognition
    // -----------------------------------------------------------------
    CROW_ROUTE(app, "/api/log_plate").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = crow::json::load(req.body);
        if (!body) return make_error(400, "Invalid JSON received");

        std::string plate = "";
        if (body.has("plate_number")) plate = body["plate_number"].s();

        std::string direction = "";
        if (body.has("direction")) direction = body["direction"].s();

        bool has_junction = body.has("junction_id");
        int junction_id = has_junction ? body["junction_id"].i() : -1;

        std::string timestamp = "";
        if (body.has("timestamp")) timestamp = body["timestamp"].s();

        if (!is_valid_plate(plate)) {
            return make_error(400, "Invalid plate_number format.");
        }
        if (direction != "entry" && direction != "exit") {
            return make_error(400, "direction must be 'entry' or 'exit'.");
        }
        if (!has_junction) {
            return make_error(400, "junction_id is required.");
        }

        try {
            pqxx::connection C(get_connection_string());
            pqxx::work W(C);

            if (!junction_exists(W, junction_id)) {
                return make_error(404, "junction_id does not reference a known junction.");
            }

            if (timestamp.empty()) {
                W.exec_params(
                    "INSERT INTO plate_logs (plate_number, junction_id, direction) VALUES ($1, $2, $3)",
                    plate, junction_id, direction);
            } else {
                W.exec_params(
                    "INSERT INTO plate_logs (plate_number, junction_id, direction, timestamp) VALUES ($1, $2, $3, $4)",
                    plate, junction_id, direction, timestamp);
            }

            pqxx::result vehicle = W.exec_params(
                "SELECT owner_name FROM vehicles WHERE plate_number = $1", plate);

            W.commit();

            crow::json::wvalue res;
            res["status"] = "success";
            res["plate_number"] = plate;
            res["registered"] = !vehicle.empty();
            if (!vehicle.empty()) {
                res["owner_name"] = vehicle[0]["owner_name"].c_str();
            }
            return crow::response(200, res);
        } catch (const std::exception &e) {
            std::cerr << "Database error: " << e.what() << std::endl;
            return make_error(500, "Failed to log plate event.");
        }
    });

    // -----------------------------------------------------------------
    // Week 13: Violation reporting & fine issuance
    // -----------------------------------------------------------------
    CROW_ROUTE(app, "/api/report_violation").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = crow::json::load(req.body);
        if (!body) return make_error(400, "Invalid JSON received");

        std::string plate = "";
        if (body.has("plate_number")) plate = body["plate_number"].s();

        std::string violation_type = "";
        if (body.has("violation_type")) violation_type = body["violation_type"].s();

        std::string severity = "";
        if (body.has("severity")) severity = body["severity"].s();

        bool has_junction = body.has("junction_id");
        int junction_id = has_junction ? body["junction_id"].i() : -1;

        std::string timestamp = "";
        if (body.has("timestamp")) timestamp = body["timestamp"].s();

        if (!is_valid_plate(plate)) {
            return make_error(400, "Invalid plate_number format.");
        }
        if (!is_valid_violation_type(violation_type)) {
            return make_error(400, "violation_type must be one of: speeding, illegal_parking, red_light.");
        }
        if (!is_valid_severity(violation_type, severity)) {
            return make_error(400, "severity must be one of: low, medium, high.");
        }

        try {
            pqxx::connection C(get_connection_string());
            pqxx::work W(C);

            if (has_junction && !junction_exists(W, junction_id)) {
                return make_error(404, "junction_id does not reference a known junction.");
            }

            pqxx::result vehicle = W.exec_params(
                "SELECT owner_name, email FROM vehicles WHERE plate_number = $1", plate);
            if (vehicle.empty()) {
                return make_error(404, "No registered vehicle found for that plate_number.");
            }

            double fine_amount = get_fine_amount(violation_type, severity);
            std::string owner_name = vehicle[0]["owner_name"].c_str();
            std::string owner_email = vehicle[0]["email"].is_null() ? "" : vehicle[0]["email"].c_str();

            pqxx::result inserted;
            if (has_junction) {
                if (timestamp.empty()) {
                    inserted = W.exec_params(
                        "INSERT INTO violations (plate_number, violation_type, severity, fine_amount, junction_id) "
                        "VALUES ($1, $2, $3, $4, $5) RETURNING id",
                        plate, violation_type, severity, fine_amount, junction_id);
                } else {
                    inserted = W.exec_params(
                        "INSERT INTO violations (plate_number, violation_type, severity, fine_amount, junction_id, timestamp) "
                        "VALUES ($1, $2, $3, $4, $5, $6) RETURNING id",
                        plate, violation_type, severity, fine_amount, junction_id, timestamp);
                }
            } else {
                if (timestamp.empty()) {
                    inserted = W.exec_params(
                        "INSERT INTO violations (plate_number, violation_type, severity, fine_amount) "
                        "VALUES ($1, $2, $3, $4) RETURNING id",
                        plate, violation_type, severity, fine_amount);
                } else {
                    inserted = W.exec_params(
                        "INSERT INTO violations (plate_number, violation_type, severity, fine_amount, timestamp) "
                        "VALUES ($1, $2, $3, $4, $5) RETURNING id",
                        plate, violation_type, severity, fine_amount, timestamp);
                }
            }
            W.commit();

            if (!owner_email.empty()) {
                std::ostringstream body_msg;
                body_msg << "Dear " << owner_name << ", your vehicle " << plate
                          << " was recorded for a '" << violation_type << "' violation ("
                          << severity << " severity). A fine of $" << fine_amount
                          << " has been issued to your account.";
                send_email_notification(owner_email, "Traffic Violation Notice", body_msg.str());
            }

            crow::json::wvalue res;
            res["status"] = "success";
            res["violation_id"] = inserted[0]["id"].as<int>();
            res["plate_number"] = plate;
            res["violation_type"] = violation_type;
            res["severity"] = severity;
            res["fine_amount"] = fine_amount;
            res["owner_notified"] = !owner_email.empty();
            return crow::response(200, res);
        } catch (const std::exception &e) {
            std::cerr << "Database error: " << e.what() << std::endl;
            return make_error(500, "Failed to report violation.");
        }
    });

    // -----------------------------------------------------------------
    // Week 14: Traffic flow logging
    // -----------------------------------------------------------------
    CROW_ROUTE(app, "/api/log_traffic_count").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = crow::json::load(req.body);
        if (!body) return make_error(400, "Invalid JSON received");

        bool has_junction = body.has("junction_id");
        int junction_id = has_junction ? body["junction_id"].i() : -1;

        bool has_count = body.has("vehicle_count");
        int vehicle_count = has_count ? body["vehicle_count"].i() : -1;

        std::string timestamp = "";
        if (body.has("timestamp")) timestamp = body["timestamp"].s();

        if (!has_junction) {
            return make_error(400, "junction_id is required.");
        }
        if (!has_count || vehicle_count < 0) {
            return make_error(400, "vehicle_count must be a non-negative integer.");
        }

        try {
            pqxx::connection C(get_connection_string());
            pqxx::work W(C);

            if (!junction_exists(W, junction_id)) {
                return make_error(404, "junction_id does not reference a known junction.");
            }

            if (timestamp.empty()) {
                W.exec_params(
                    "INSERT INTO traffic_counts (junction_id, vehicle_count) VALUES ($1, $2)",
                    junction_id, vehicle_count);
            } else {
                W.exec_params(
                    "INSERT INTO traffic_counts (junction_id, vehicle_count, timestamp) VALUES ($1, $2, $3)",
                    junction_id, vehicle_count, timestamp);
            }
            W.commit();

            crow::json::wvalue res;
            res["status"] = "success";
            res["junction_id"] = junction_id;
            res["vehicle_count"] = vehicle_count;
            return crow::response(200, res);
        } catch (const std::exception &e) {
            std::cerr << "Database error: " << e.what() << std::endl;
            return make_error(500, "Failed to log traffic count.");
        }
    });

    // -----------------------------------------------------------------
    // Week 14: Congestion report + simple hour-of-day prediction
    // -----------------------------------------------------------------
    CROW_ROUTE(app, "/api/congestion_report").methods(crow::HTTPMethod::GET)
    ([](const crow::request& req){
        try {
            pqxx::connection C(get_connection_string());
            pqxx::work W(C);

            // Optional query params: junction_id filter, hour (0-23) for prediction.
            std::string junction_filter = req.url_params.get("junction_id") ? req.url_params.get("junction_id") : "";
            int target_hour = -1;
            if (req.url_params.get("hour")) {
                target_hour = std::atoi(req.url_params.get("hour"));
                if (target_hour < 0 || target_hour > 23) {
                    return make_error(400, "hour must be between 0 and 23.");
                }
            } else {
                target_hour = std::atoi(W.exec("SELECT EXTRACT(HOUR FROM NOW())::int")[0][0].c_str());
            }

            std::string recent_sql =
                "SELECT j.id, j.name, "
                "COALESCE(pl.cnt, 0) + COALESCE(tc.cnt, 0) AS recent_count "
                "FROM junctions j "
                "LEFT JOIN (SELECT junction_id, COUNT(*) AS cnt FROM plate_logs "
                "           WHERE timestamp >= NOW() - INTERVAL '" + std::to_string(CONGESTION_WINDOW_MINUTES) + " minutes' "
                "           GROUP BY junction_id) pl ON pl.junction_id = j.id "
                "LEFT JOIN (SELECT junction_id, SUM(vehicle_count) AS cnt FROM traffic_counts "
                "           WHERE timestamp >= NOW() - INTERVAL '" + std::to_string(CONGESTION_WINDOW_MINUTES) + " minutes' "
                "           GROUP BY junction_id) tc ON tc.junction_id = j.id ";
            if (!junction_filter.empty()) {
                recent_sql += "WHERE j.id = " + W.quote(std::atoi(junction_filter.c_str())) + " ";
            }
            recent_sql += "ORDER BY j.id";

            pqxx::result recent = W.exec(recent_sql);

            pqxx::result historical = W.exec_params(
                "SELECT junction_id, AVG(vehicle_count) AS avg_count FROM traffic_counts "
                "WHERE EXTRACT(HOUR FROM timestamp) = $1 GROUP BY junction_id", target_hour);

            std::map<int, double> historical_avg;
            for (auto row : historical) {
                historical_avg[row["junction_id"].as<int>()] = row["avg_count"].as<double>();
            }

            crow::json::wvalue res;
            res["status"] = "success";
            res["window_minutes"] = CONGESTION_WINDOW_MINUTES;
            res["threshold"] = CONGESTION_THRESHOLD;
            res["hour_checked"] = target_hour;

            std::vector<crow::json::wvalue> junctions_json;
            std::vector<std::pair<int, long>> congested_junctions; // id, count
            std::vector<std::pair<int, long>> all_junction_counts;

            for (auto row : recent) {
                int jid = row["id"].as<int>();
                long recent_count = row["recent_count"].as<long>();
                bool congested = is_congested(recent_count);
                double hist_avg = historical_avg.count(jid) ? historical_avg[jid] : 0.0;
                bool predicted_congested = is_predicted_congested(hist_avg);

                crow::json::wvalue j;
                j["junction_id"] = jid;
                j["junction_name"] = row["name"].c_str();
                j["recent_count"] = recent_count;
                j["is_congested"] = congested;
                j["historical_avg_for_hour"] = hist_avg;
                j["predicted_congested"] = predicted_congested;
                junctions_json.push_back(std::move(j));

                all_junction_counts.push_back({jid, recent_count});
                if (congested) congested_junctions.push_back({jid, recent_count});
            }
            res["junctions"] = std::move(junctions_json);

            // Notify drivers recently logged at congested junctions, suggesting
            // the least-congested known alternative junction.
            int notified = 0;
            for (auto& cj : congested_junctions) {
                int alt_junction_id = -1;
                long best_count = -1;
                for (auto& jc : all_junction_counts) {
                    if (jc.first == cj.first) continue;
                    if (best_count == -1 || jc.second < best_count) {
                        best_count = jc.second;
                        alt_junction_id = jc.first;
                    }
                }

                pqxx::result drivers = W.exec_params(
                    "SELECT DISTINCT v.email, v.owner_name FROM plate_logs pl "
                    "JOIN vehicles v ON v.plate_number = pl.plate_number "
                    "WHERE pl.junction_id = $1 AND pl.timestamp >= NOW() - INTERVAL '" +
                    std::to_string(CONGESTION_WINDOW_MINUTES) + " minutes' "
                    "AND v.email IS NOT NULL AND v.email <> ''", cj.first);

                for (auto row : drivers) {
                    std::string email = row["email"].c_str();
                    std::string owner = row["owner_name"].c_str();
                    std::ostringstream msg;
                    msg << "Dear " << owner << ", junction " << cj.first
                        << " is currently congested.";
                    if (alt_junction_id != -1) {
                        msg << " Consider using junction " << alt_junction_id << " instead.";
                    }
                    send_email_notification(email, "Traffic Congestion Alert", msg.str());
                    notified++;
                }
            }
            res["drivers_notified"] = notified;

            return crow::response(200, res);
        } catch (const std::exception &e) {
            std::cerr << "Database error: " << e.what() << std::endl;
            return make_error(500, "Failed to generate congestion report.");
        }
    });

    // -----------------------------------------------------------------
    // Week 15: Emergency vehicle prioritisation
    // -----------------------------------------------------------------
    CROW_ROUTE(app, "/api/emergency_alert").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = crow::json::load(req.body);
        if (!body) return make_error(400, "Invalid JSON received");

        std::string plate = "";
        if (body.has("plate_number")) plate = body["plate_number"].s();

        bool has_junction = body.has("junction_id");
        int junction_id = has_junction ? body["junction_id"].i() : -1;

        if (!is_valid_plate(plate)) {
            return make_error(400, "Invalid plate_number format.");
        }
        if (!has_junction) {
            return make_error(400, "junction_id is required.");
        }

        try {
            pqxx::connection C(get_connection_string());
            pqxx::work W(C);

            if (!junction_exists(W, junction_id)) {
                return make_error(404, "junction_id does not reference a known junction.");
            }

            pqxx::result vehicle = W.exec_params(
                "SELECT owner_name, is_emergency FROM vehicles WHERE plate_number = $1", plate);
            if (vehicle.empty()) {
                return make_error(404, "No registered vehicle found for that plate_number.");
            }
            bool is_emergency = vehicle[0]["is_emergency"].as<bool>();
            if (!is_emergency_alert_allowed(is_emergency)) {
                return make_error(403, "Vehicle is not a registered emergency vehicle; alert rejected.");
            }

            // Log the priority event as an entry at this junction.
            W.exec_params(
                "INSERT INTO plate_logs (plate_number, junction_id, direction) VALUES ($1, $2, 'entry')",
                plate, junction_id);

            pqxx::result drivers = W.exec_params(
                "SELECT DISTINCT v.email, v.owner_name FROM plate_logs pl "
                "JOIN vehicles v ON v.plate_number = pl.plate_number "
                "WHERE pl.junction_id = $1 AND pl.timestamp >= NOW() - INTERVAL '" +
                std::to_string(EMERGENCY_LOOKBACK_MINUTES) + " minutes' "
                "AND v.plate_number <> $2 AND v.email IS NOT NULL AND v.email <> ''",
                junction_id, plate);

            W.commit();

            int notified = 0;
            for (auto row : drivers) {
                std::string email = row["email"].c_str();
                std::string owner = row["owner_name"].c_str();
                std::ostringstream msg;
                msg << "Dear " << owner << ", an emergency vehicle is approaching junction "
                    << junction_id << ". Please clear the way immediately.";
                send_email_notification(email, "Emergency Vehicle Alert - Clear the Way", msg.str());
                notified++;
            }

            crow::json::wvalue res;
            res["status"] = "success";
            res["message"] = "Emergency priority event logged.";
            res["plate_number"] = plate;
            res["junction_id"] = junction_id;
            res["drivers_notified"] = notified;
            return crow::response(200, res);
        } catch (const std::exception &e) {
            std::cerr << "Database error: " << e.what() << std::endl;
            return make_error(500, "Failed to process emergency alert.");
        }
    });

    int port = std::atoi(env_or("PORT", "8080").c_str());
    std::cout << "Starting C++ Crow Server on port " << port << "..." << std::endl;
    app.port(port).multithreaded().run();
}
