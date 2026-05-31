#include "crow_all.h"
#include <iostream>
#include <string>
#include <regex>
#include <pqxx/pqxx> 

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/api/register_vehicle").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "Invalid JSON received");

        // --- FIXED: Safely extracting data without ternary operators ---
        std::string plate = "UNKNOWN";
        if (body.has("plate_number")) {
            plate = body["plate_number"].s();
        }

        std::string owner = "UNKNOWN";
        if (body.has("owner_name")) {
            owner = body["owner_name"].s();
        }

        std::string email = "";
        if (body.has("email")) {
            email = body["email"].s();
        }

        std::string type = "Car";
        if (body.has("vehicle_type")) {
            type = body["vehicle_type"].s();
        }
        // ---------------------------------------------------------------

        // Regex Validation
        std::regex plate_format("^[A-Z]{1,3}-[A-Z]{1,2}-[0-9]{1,4}$");
        if (!std::regex_match(plate, plate_format)) {
            crow::json::wvalue error_res;
            error_res["status"] = "error";
            error_res["message"] = "Registration failed: Illogical license plate.";
            return crow::response(400, error_res); 
        }
        
        // DATABASE CONNECTION & INSERTION
        try {
            pqxx::connection C("dbname=traffic_system_db");
            if (C.is_open()) {
                pqxx::work W(C);
                std::string sql = "INSERT INTO vehicles (plate_number, owner_name, email, vehicle_type) VALUES ($1, $2, $3, $4)";
                W.exec_params(sql, plate, owner, email, type);
                W.commit();
                std::cout << "Successfully saved to Database: " << plate << std::endl;
            }
        } catch (const std::exception &e) {
            std::cerr << "Database error: " << e.what() << std::endl;
            crow::json::wvalue error_res;
            error_res["status"] = "error";
            error_res["message"] = "Failed to save to database. Is PostgreSQL running?";
            return crow::response(500, error_res);
        }

        // Return Success
        crow::json::wvalue res;
        res["status"] = "success";
        res["message"] = "Vehicle registered and saved to database!";
        res["registered_plate"] = plate;
        
        return crow::response(200, res);
    });

    std::cout << "Starting C++ Crow Server on port 8080..." << std::endl;
    app.port(8080).multithreaded().run();
}