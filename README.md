# 📅 Event Scheduler System (C++)

A **C++-based Event Scheduler** that helps users manage their time effectively by scheduling, updating, and tracking events. The system uses **advanced data structures** (AVL Trees, Min-Heaps, Stacks, Vectors) and **multithreading** for real-time notifications, ensuring efficient and reliable scheduling.

---

## ✨ Features

### 📌 Event Management
- Add, update, and remove events.
- Search events by date or time.
- Prevents overlapping events.
- Supports lazy deletion for efficiency.

### 🌲 Date & Event Tree Management
- Uses an **AVL Tree** for balanced event storage by date.
- Insert, search, and delete events in **O(log n)** time.
- Retrieve all events for a given date.
- Find overlapping events with precise time validation.

### ⏰ Real-Time Notifications
- **Multithreading** continuously monitors upcoming events.
- Notifies users of events due in the next few minutes.
- Runs notifications without blocking other operations.

### ✅ Input Validation
- Validates **date formats** and **time entries**.
- Prevents invalid or illogical inputs.

### 🖥 User Interface
- Command-Line Interface (CLI) for interaction.
- Add, search, update, or remove events easily.
- Error handling ensures smooth user experience.

---

## 🛠 Technologies & Data Structures Used
- **C++ (OOP, STL, Multithreading)**
- **AVL Tree** – Efficient storage & retrieval by date.
- **Min-Heap (Priority Queue)** – Real-time notification management.
- **Stacks** – Iterative traversal of AVL tree.
- **Vectors** – Dynamic and efficient heap implementation.

---

## 📂 Project Structure
