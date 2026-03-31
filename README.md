**Project Title**
CAN-Based Pick-to-Light Warehouse System

**Overview**
This project implements a Pick-to-Light system used in warehouses to guide operators for accurate item picking.
The system uses a Client-Server architecture over CAN protocol, where a central controller (Server) communicates with multiple nodes (Clients) to indicate picking locations using visual signals.
Each node responds with acknowledgment, ensuring reliable communication and error-free operation.

**Features**
CAN-based communication between nodes
Client-Server architecture
Real-time pick indication using LEDs / display
ACK-based reliable data transfer
EEPROM-based persistent Node ID storage
State machine-based system design

**Working Principle**
Server sends pick command to a specific node
Target client activates LED/display
Operator picks item
Client sends ACK back to server
Server confirms completion

**State Machine Design**
**States:**
Idle State → Waiting for command
Configuration State → Node ID setup
Operation State → Active picking

**Communication Details**
Protocol: CAN
Data:
Node ID
Command
ACK response

**Usage**
Flash server code to master ECU
Flash client code to nodes
Configure Node IDs
Start system → send pick requests

**Requirements**
PIC Microcontroller (e.g., PIC18F4580)
MCP2551 CAN Transceiver
LEDs / Display (for indication)
EEPROM
CAN Bus wiring

**Limitations**
Limited number of nodes
Basic error handling
No GUI interface

**Future Improvements**
Add barcode scanner integration
Wireless communication (LoRa / Wi-Fi)
GUI dashboard for warehouse control
Multi-item picking optimization

**Key Concepts Used**
CAN Protocol
Embedded C
State Machines
EEPROM handling
Client-Server communication

**Applications**
Warehouses
Logistics & supply chain
Inventory management systems
