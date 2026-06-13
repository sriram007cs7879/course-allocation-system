# Course Allocation System

A scalable C++20 course allocation and scheduling engine based on a modified
student-proposing Gale-Shapley algorithm.

The allocator combines ranked student preferences with course priorities while enforcing
seat capacities, student course-load limits, prerequisite requirements, and timetable
conflicts. Each course uses a heap to retain its strongest tentative candidates and
efficiently displace the current weakest candidate when a higher-priority student proposes.

## Features

- Many-to-many matching with student quotas and course capacities
- Ordered student preferences and course-specific priority rankings
- Prerequisite eligibility checks
- Timetable-conflict prevention
- Heap-based tentative acceptance and displacement
- Deterministic tie-breaking
- CSV input and output with quoted-field support
- Automated unit tests and a synthetic large-dataset benchmark
- No third-party runtime dependencies

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Run the Sample

```bash
./build/course_allocator \
  --students data/students.csv \
  --courses data/courses.csv \
  --output allocations.csv
```

The command prints course occupancy and proposal statistics, then writes:

```csv
student_id,name,assigned_courses
S001,Asha Rao,CS201;HS101
S002,Vikram Shah,EE201;HS101
```

## Input Format

List-valued fields are separated by semicolons.

### `students.csv`

| Column | Meaning |
|---|---|
| `student_id` | Unique student identifier |
| `name` | Display name |
| `max_courses` | Maximum number of allocated courses |
| `completed_courses` | Previously completed courses |
| `preferences` | Ordered course choices, best first |

### `courses.csv`

| Column | Meaning |
|---|---|
| `course_id` | Unique course identifier |
| `name` | Course title |
| `capacity` | Number of available seats |
| `timeslot` | Scheduling slot; equal non-empty slots conflict |
| `prerequisites` | Required previously completed courses |
| `priority_order` | Ordered student priority list, best first |

Students omitted from a priority list are placed after explicitly ranked students and
tie-broken by student ID.

## Benchmark

The benchmark generates a deterministic synthetic allocation problem:

```bash
./build/course_allocator_benchmark              # 50,000 students, 500 courses
./build/course_allocator_benchmark 100000 1000
```

It reports allocated seats, proposal count, displacement count, and wall-clock runtime.
To keep generated datasets memory-efficient, each synthetic course explicitly ranks up to
2,000 students; remaining students use deterministic ID tie-breaking.

## Algorithm

Students propose to feasible courses in preference order. A course with spare capacity
accepts immediately. A full course compares the proposer with the worst currently accepted
student, stored at the root of a max-heap. A stronger proposer replaces that student, who
then resumes proposing.

For `P` proposals, largest capacity `Cmax`, and `D` timetable-conflict checks, the matching
phase runs in `O(P log Cmax + D)`. See [docs/ALGORITHM.md](docs/ALGORITHM.md) for details.

## Project Structure

```text
include/course_allocation/  Public models, allocator, and CSV API
src/                        Allocator, CLI, CSV parser, and benchmark
tests/                      Unit tests
data/                       Runnable sample dataset
docs/                       Algorithm documentation
```

## License

MIT
