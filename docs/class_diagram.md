# Class Diagram: Database Entities

Reflects `database-scripts/schema.sql`.

```mermaid
classDiagram
    class Vehicle {
        +string plate_number PK
        +string owner_name
        +string email
        +string vehicle_type
        +bool is_emergency
        +datetime created_at
    }

    class Junction {
        +int id PK
        +string name
        +string location
    }

    class PlateLog {
        +int id PK
        +string plate_number
        +int junction_id FK
        +string direction
        +datetime timestamp
    }

    class Violation {
        +int id PK
        +string plate_number FK
        +string violation_type
        +string severity
        +decimal fine_amount
        +int junction_id FK
        +datetime timestamp
    }

    class TrafficCount {
        +int id PK
        +int junction_id FK
        +int vehicle_count
        +datetime timestamp
    }

    Vehicle "1" --> "0..*" Violation : commits
    Vehicle "1" --> "0..*" PlateLog : seen as
    Junction "1" --> "0..*" PlateLog : recorded at
    Junction "1" --> "0..*" Violation : occurred at
    Junction "1" --> "0..*" TrafficCount : measured at
```

**Notes**

- `PlateLog.plate_number` is intentionally not a foreign key to `Vehicle`:
  junction cameras log every plate they see, including unregistered
  vehicles. The `log_plate` endpoint reports registration status by looking
  the plate up in `Vehicle` separately, rather than requiring it to exist.
- `Violation.junction_id` is nullable (a violation is not always tied to a
  specific junction).
