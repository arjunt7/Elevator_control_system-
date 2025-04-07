MultiThreaded Elevator System

The MultiThreaded Elevator System is a POSIX-compliant C application designed to simulate and manage multiple elevators in a building with K floors. The program aims to fulfill passenger requests with minimal turns and elevator movements, using shared memory, message queues, and multi-threading for process communication and concurrency.

Table of Contents
- Features
- Setup and Compilation
  - Prerequisites
  - Building and Running
- Inter-Process Communication
- Performance Evaluation
- Algorithm Overview

Features
- Simulates up to 100 elevators operating concurrently.
- Implements optimized logic for handling passenger pickups and drop-offs.
- Utilizes POSIX threads, shared memory, and message queues for synchronization and data exchange.
- Requires authorization strings from external solver processes before movement.


Setup and Compilation

Prerequisites
Ensure the following are installed on your Linux system:
- Ubuntu 22.04 or a POSIX-compliant Linux distribution
- GCC compiler (gcc)
- POSIX IPC libraries (e.g., <sys/ipc.h>, <pthread.h>)

Building and Running

   - gcc helper-mt.c -lpthread -o helper
   - gcc solution_mt.c -lpthread -o solution

To execute a test case (e.g., test case 2):
    ./helper 2

Inter-Process Communication

Shared Memory
- Holds data related to elevator positions, movement plans, and passenger requests.
- Ensures coordinated access and updates between processes.

Message Queues
- Used by solver processes to send authorization strings for elevator operations.
- Helper process manages state updates and validation based on solver input.

Synchronization
- Mutexes and semaphores maintain thread-safe access to shared resources.
- Signal handlers are used to manage and respond to inter-process events.

Performance Evaluation
The system is evaluated based on the following criteria:
1. Total number of turns required to serve all passenger requests.
2. Total elevator movements (each up/down action counts as one move).
3. Accuracy and reliability of request handling.

Algorithm Overview
1. Initialize all elevators at the ground floor.
2. Detect and queue incoming passenger requests.
3. Determine optimal movement (up, down, stay) for each elevator per turn.
4. Communicate with solver processes for movement authorization.
5. Update elevator states in shared memory and inform the helper process.
6. Repeat until all requests have been processed and fulfilled.
