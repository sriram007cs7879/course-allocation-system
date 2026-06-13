# Course Allocation System

A simple single-file C++ implementation of a course allocation and scheduling system using
a modified Gale-Shapley algorithm.

The program considers:

- student course preferences;
- course priority rankings;
- seat capacities;
- student course limits;
- prerequisites;
- timetable conflicts.

When a course is full, a priority queue keeps track of its lowest-priority accepted student.
A better-ranked applicant can replace that student, who then continues applying to other
preferred courses.

## Files

```text
main.cpp           Complete implementation
data/students.csv  Example student data
data/courses.csv   Example course data
```

## Compile

```bash
g++ -std=c++17 -O2 -Wall -Wextra main.cpp -o course_allocator
```

## Run

```bash
./course_allocator \
  --students data/students.csv \
  --courses data/courses.csv \
  --output allocations.csv
```

## Self-Test

```bash
./course_allocator --self-test
```

## Benchmark

```bash
./course_allocator --benchmark
./course_allocator --benchmark 100000 1000
```

The default benchmark generates 50,000 students and 500 courses.

## CSV Format

List values are separated with semicolons.

`students.csv`:

```csv
student_id,name,max_courses,completed_courses,preferences
S001,Asha Rao,2,CS101;MA101,CS201;MA201;HS101
```

`courses.csv`:

```csv
course_id,name,capacity,timeslot,prerequisites,priority_order
CS201,Algorithms,3,MON-09,CS101,S004;S001;S008
```

## Complexity

Each actual student-course proposal is made at most once. Heap insertion and replacement
take `O(log capacity)`, making the matching phase approximately
`O(proposals * log(max capacity))`.

## License

MIT
