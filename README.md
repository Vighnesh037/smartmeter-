# 🔌 IoT-Based Intelligent Smart Socket

## 📌 Overview
This project implements a smart energy monitoring system using ESP32, ZMPT101B voltage sensor, and ACS712 current sensor. The system measures real-time voltage, current, power, and energy consumption, stores data in Firebase, and performs cost-optimized scheduling using tariff and peak-hour analysis.

---

## 🚀 Features

- Real-time Voltage & Current Measurement
- RMS Calculation using ESP32
- Power and Energy (kWh) Calculation
- Monthly Unit & Bill Estimation
- Hour-wise Energy Aggregation
- Peak Usage Detection
- Cost-Based Smart Scheduling
- Firebase Cloud Integration
- Web Dashboard Visualization

---

## 🏗 System Architecture

ESP32 → Firebase Realtime Database → Web Dashboard

---

## ⚙️ Hardware Used

- ESP32 Dev Module
- ZMPT101B Voltage Sensor
- ACS712 Current Sensor
- 16x2 I2C LCD
- AC Load (Charger for testing)

---

## ☁️ Cloud Platform

- Firebase Realtime Database
- Real-time data synchronization

---

## 🧠 Analytics Implemented

1. Hourly Energy Tracking
2. Peak Usage Detection
3. Tariff-Based Cost Optimization
4. Suggested Optimal Usage Time

---

## 💰 Tariff Logic

- Peak Hours (6PM–10PM) → ₹7/unit
- Normal Hours → ₹5/unit
- Off-Peak Hours → ₹3/unit

---

## 📊 Firebase Data Structure
