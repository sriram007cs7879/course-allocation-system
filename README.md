# Course Allocation System

A C++ implementation of a stable course allocation algorithm that assigns students to courses based on their preferences while respecting course priorities, capacities, prerequisites, and timetable conflicts.

## Features

- Stable allocation algorithm
- Preference-based matching
- Course priority handling
- Prerequisite and timetable conflict checks
- CSV input/output
- Built-in self-test and benchmarking modes

## Build

```bash
g++ -std=c++17 -O2 main.cpp -o allocator
```

## Run

```bash
./allocator --students data/students.csv --courses data/courses.csv
```

For self-tests:

```bash
./allocator --self-test
```

For benchmarking:

```bash
./allocator --benchmark
```
