# Use Case Diagram

Covers all actors and use cases implemented across weeks 12-15. Mermaid has
no dedicated UML use-case diagram syntax, so this is drawn as a flowchart:
actors are rectangles outside the system boundary, use cases are stadium
("pill") shapes inside it, solid lines are associations, dashed labeled
arrows are `<<include>>` relationships.

```mermaid
flowchart LR
    VehicleOwner["Vehicle Owner"]
    TrafficDept["Traffic Dept Staff / Camera System"]
    EmergencyDispatcher["Emergency Dispatcher"]

    subgraph System["Traffic Management System"]
        UC1(["Register Vehicle"])
        UC2(["Log Plate at Junction"])
        UC3(["Report Violation"])
        UC4(["Log Traffic Count"])
        UC5(["View Congestion Report"])
        UC6(["Trigger Emergency Alert"])
        UC7(["Receive Email Notification"])
    end

    VehicleOwner --> UC1
    VehicleOwner --> UC7

    TrafficDept --> UC2
    TrafficDept --> UC3
    TrafficDept --> UC4
    TrafficDept --> UC5

    EmergencyDispatcher --> UC6

    UC3 -.->|"<<include>>"| UC7
    UC5 -.->|"<<include>>"| UC7
    UC6 -.->|"<<include>>"| UC7
    UC2 -.->|"<<include>>"| UC1
```

**Notes**

- `Log Plate at Junction` includes a lookup against `Register Vehicle`'s data
  to determine whether the seen plate belongs to a registered vehicle.
- `Report Violation`, `View Congestion Report` (when congestion is detected),
  and `Trigger Emergency Alert` all include sending an email notification.
- `Traffic Dept Staff / Camera System` represents the source of camera feed
  data / junction sensors in this simulation (plate and count data are
  submitted directly via the API rather than through real image recognition).
