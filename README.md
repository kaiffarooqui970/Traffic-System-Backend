# 🚦 Automated Traffic Management System - Backend API

A high-performance, microservices-oriented backend architecture designed to manage city traffic systems, log vehicle data, and automate violation processing. 

This system bridges the rapid-development capabilities of a **Python/Django API Gateway** with the high-speed processing of a **C++ Core Engine**, persisting data securely in a **PostgreSQL** database.

## 🏗️ Architecture Overview

The backend utilizes a decoupled gateway-engine pattern:
1. **Client / Postman:** Sends JSON payload requests.
2. **Django API Gateway (Port 8000):** Receives client traffic, validates initial routing, and forwards requests to the internal core engine.
3. **C++ Crow Engine (Port 8080):** The high-speed core that parses JSON, runs strict Regex validation on license plates, and handles complex business logic.
4. **PostgreSQL Database:** Persists valid transactions securely via `libpqxx`.

## ✨ Current Features
- **Secure API Routing:** Django handles external-facing endpoints and cleanly delegates tasks.
- **Vehicle Registration:** End-to-end processing of new vehicles into the system.
- **Strict Data Validation:** C++ Regex engine enforces standardized license plate formats (e.g., `L-AB-1234`) before database insertion.
- **Transactional Database Insertion:** C++ communicates directly with PostgreSQL to ensure atomic data saving.

## 🚀 Upcoming Roadmap (Weeks 12 & 13)
- [ ] Junction Plate Logging: Endpoints to ingest real-time camera data.
- [ ] Automated Violation Detection: Algorithmic checks for speeding and red-light running.
- [ ] Fine Issuance: Automated calculation of penalties based on violation severity.
- [ ] Email Notifications: Automated alerts sent to registered vehicle owners.

---

## 🛠️ Technology Stack

**API Gateway:**
* Python 3
* Django

**Core Processing Engine:**
* C++ 17
* [Crow](https://crowcpp.org/) (Fast C++ Microframework)
* CMake & Make (Build system)

**Database:**
* PostgreSQL
* `libpqxx` (C++ PostgreSQL driver)

---

## 💻 Local Setup & Installation (macOS)

### 1. Prerequisites
Ensure you have the following installed via Homebrew:
```bash
brew install postgresql libpq libpqxx cmake

2. Database Setup
Create the database and initialize the schema:

Bash
createdb traffic_system_db
psql -d traffic_system_db -c "CREATE TABLE vehicles (
    plate_number VARCHAR(15) PRIMARY KEY,
    owner_name VARCHAR(100) NOT NULL,
    email VARCHAR(100),
    vehicle_type VARCHAR(50)
);"
3. Build & Run the C++ Core Engine
Navigate to the backend directory, compile, and execute:

Bash
cd crow-backend
cmake .
make
./server
(The C++ engine will now listen on http://localhost:8080)

4. Start the Django API Gateway
Open a new terminal window, activate your environment, and run the gateway:

Bash
cd django-gateway
source venv/bin/activate  # If using a virtual environment
python manage.py runserver
(The Django gateway will now listen on http://localhost:8000)

📡 API Documentation
Register a Vehicle
Registers a new vehicle into the system. The license plate must follow the strict A-A-1 format rule.

Endpoint: POST /gateway/register_vehicle

Request Body (JSON):

JSON
{
    "plate_number": "L-AB-1234",
    "owner_name": "kaif",
    "email": "kaif@example.com",
    "vehicle_type": "SUV"
}
Success Response (200 OK):

JSON
{
    "status": "success",
    "message": "Vehicle registered and saved to database!",
    "registered_plate": "L-AB-1234"
}
Error Response (400 Bad Request - Invalid Plate):

JSON
{
    "status": "error",
    "message": "Registration failed: Illogical license plate."
}
Developed by Kaif Ahmed Farooqui, ALI 
