# PakAgri--Automated-Crop-Irrigation-System

_Dated: March 2025_

## Overview

The **Precision Crop Automation System** is a C++ console application designed to simulate a smart farming environment. Built entirely with Object-Oriented Programming (OOP) principles, the system allows users to register various crops, simulate environmental sensors, evaluate growth stages, and maintain persistent records of their farm's state.

## Key Features

- **Object-Oriented Design**: Utilizes classes, inheritance, and polymorphism to represent crops, sensors, and agricultural actions.
- **Persistent Storage**: Employs File I/O to automatically save and load crop data (`crops.dat`, `crops.txt`) and moisture thresholds (`thresholds.dat`). Data remains safe across multiple sessions.
- **Sensor Simulation**: Includes simulated `MoistureSensor` and `TemperatureSensor` components that generate randomized readings to mimic real-world environmental changes.
- **Automated Action Suggestions**: Evaluates real-time sensor data against custom crop thresholds and automatically suggests actions such as _Watering_ or _Fertilizing_.
- **Growth Tracking**: Automatically advances growth stages as crops are watered and keeps track of days since the last fertilization.
- **Enhanced Console UI**: Features custom terminal auto-formatting, loading animations, and colored text for a polished user experience.

## System Architecture

- **`Crop` & `Date` classes**: Manage individual crop properties, planting dates, and ID generation.
- **`Sensor` (Base) -> `MoistureSensor` & `TemperatureSensor`**: Provide polymorphic sensor readings.
- **`Action` (Base) -> `WaterAction` & `FertilizeAction`**: Abstract the execution of farm operations.
- **`CropManager`**: Handles the underlying dynamic array model (resizing when necessary) and file stream serializations.
- **`PrecisionFarmSystem`**: The main system controller that integrates sensors and the crop manager to cycle through monitoring stages.

## File Structure

- `project3oop.cpp` / `OOP_Project.cpp`: The main source code containing the entire system.
- `crops.dat` & `thresholds.dat`: Auto-generated binary/text data files storing the current farm state.
- `crops.txt`: A plain-text readable backup of the crop data.
