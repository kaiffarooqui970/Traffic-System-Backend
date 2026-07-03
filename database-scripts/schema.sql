-- Traffic System Database Schema
-- Covers weeks 12-15: vehicle registration, plate logging, violations,
-- traffic flow / congestion analysis, and emergency vehicle prioritisation.

-- Week 12: Vehicle Registration & License Plate Recognition
CREATE TABLE IF NOT EXISTS vehicles (
    plate_number  VARCHAR(15) PRIMARY KEY,
    owner_name    VARCHAR(100) NOT NULL,
    email         VARCHAR(100),
    vehicle_type  VARCHAR(50) NOT NULL DEFAULT 'Car',
    is_emergency  BOOLEAN NOT NULL DEFAULT FALSE,
    created_at    TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS junctions (
    id       SERIAL PRIMARY KEY,
    name     VARCHAR(100) NOT NULL UNIQUE,
    location VARCHAR(150)
);

-- plate_number is NOT a FK here on purpose: junction cameras log every
-- plate they see, including unregistered vehicles. log_plate reports
-- registration status separately by looking the plate up in `vehicles`.
CREATE TABLE IF NOT EXISTS plate_logs (
    id           SERIAL PRIMARY KEY,
    plate_number VARCHAR(15) NOT NULL,
    junction_id  INTEGER NOT NULL REFERENCES junctions(id),
    direction    VARCHAR(10) NOT NULL CHECK (direction IN ('entry', 'exit')),
    timestamp    TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_plate_logs_junction_time ON plate_logs(junction_id, timestamp);
CREATE INDEX IF NOT EXISTS idx_plate_logs_plate ON plate_logs(plate_number);

-- Week 13: Monitoring Traffic Violations and Issuing Fines
CREATE TABLE IF NOT EXISTS violations (
    id             SERIAL PRIMARY KEY,
    plate_number   VARCHAR(15) NOT NULL REFERENCES vehicles(plate_number),
    violation_type VARCHAR(30) NOT NULL CHECK (violation_type IN ('speeding', 'illegal_parking', 'red_light')),
    severity       VARCHAR(10) NOT NULL CHECK (severity IN ('low', 'medium', 'high')),
    fine_amount    NUMERIC(10, 2) NOT NULL CHECK (fine_amount >= 0),
    junction_id    INTEGER REFERENCES junctions(id),
    timestamp      TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_violations_plate ON violations(plate_number);

-- Week 14: Analysing Traffic Flow and Managing Congestion
CREATE TABLE IF NOT EXISTS traffic_counts (
    id            SERIAL PRIMARY KEY,
    junction_id   INTEGER NOT NULL REFERENCES junctions(id),
    vehicle_count INTEGER NOT NULL CHECK (vehicle_count >= 0),
    timestamp     TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_traffic_counts_junction_time ON traffic_counts(junction_id, timestamp);
