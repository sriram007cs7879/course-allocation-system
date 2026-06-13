# Course Allocation System

A simple single-file C++ implementation of a course allocation and scheduling system using
a modified Gale-Shapley algorithm.

The program handles:

- student course preferences;
- course priority rankings;
- seat capacities;
- student course limits;
- prerequisites;
- timetable conflicts.

## What the Program Does

The input describes both sides of the allocation:

- Each **student** provides an ordered preference list, completed prerequisite courses, and
  the maximum number of courses they can take.
- Each **course** provides a seat capacity, timetable slot, prerequisite list, and an ordered
  priority list of students.

The program reads both CSV files, validates IDs and preferences, performs the allocation,
prints a course-wise summary, and writes every student's assigned courses to an output CSV.

During allocation, it maintains:

- courses currently assigned to each student;
- courses to which each student has already proposed;
- a queue of students who still need seats;
- a priority queue of tentatively accepted students for every course.

## Algorithm

The implementation is a many-to-many modification of the student-proposing
**Gale-Shapley deferred acceptance algorithm**.

1. Initially, every student is placed in an active queue.
2. The next student selects their highest preferred feasible course.
3. A course is feasible only if:
   - the student has not already proposed to it;
   - all prerequisites are completed;
   - its timetable does not conflict with a currently assigned course.
4. The student proposes to the selected course.
5. If the course has an empty seat, the student is tentatively accepted.
6. If the course is full, the proposer is compared with the course's lowest-priority
   accepted student.
7. If the proposer has higher priority:
   - the course accepts the proposer;
   - the lowest-priority student is displaced;
   - the displaced student returns to the queue and continues applying.
8. Otherwise, the proposal is rejected and the student tries their next preference.
9. The process ends when each student has filled their course limit or has no feasible
   preference remaining.

Assignments remain tentative until the algorithm finishes because a higher-priority student
may arrive later and replace an existing assignee.

### Priority Queue

Every course uses:

```cpp
priority_queue<pair<int, string>>
```

The pair stores the student's course-priority rank and ID. A larger rank means lower
priority, so the heap top is always the worst currently accepted student. A full course can
therefore inspect and replace its weakest assignee efficiently.

Students explicitly listed in a course's priority order receive their listed rank. Students
not listed are placed after all explicitly ranked students, with student ID used as a
deterministic tie-breaker.

### Timetable Conflicts

A preference that conflicts with a student's current tentative assignment is **deferred**,
not permanently rejected. If that student is later displaced from the conflicting course,
the deferred preference may become feasible and is considered again.

### Example

Suppose a course has one seat and currently accepts `S001`. If `S002` proposes later and
the course priority is:

```text
S002 > S001
```

then `S002` takes the seat. `S001` returns to the active queue and applies to the next
feasible course in their preference list.

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

Expected output:

```text
All self-tests passed.
```

## Benchmark

```bash
./course_allocator --benchmark
./course_allocator --benchmark 100000 1000
```

The default benchmark generates 50,000 students and 500 courses.

Example:

```text
Students: 50000
Courses: 500
Allocated seats: 148339
Proposals: 200418
Time: 0.509 seconds
```

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

## Output Statistics

The terminal summary reports:

- **Proposals:** actual student-course applications;
- **Accepted:** successful tentative acceptances, including replacements;
- **Rejected:** applications rejected by full or zero-capacity courses;
- **Displacements:** accepted students replaced by higher-priority applicants;
- **Prerequisite skips:** preferences skipped because requirements were incomplete;
- **Conflict skips:** temporarily infeasible choices caused by timetable clashes.

Because acceptances are tentative, the acceptance count can exceed the final number of
allocated seats when displacements occur.

## Complexity

Let:

- `P` be the number of actual proposals;
- `Cmax` be the largest course capacity;
- `S` be the number of preference entries inspected, including deferred conflict rechecks.

Each student proposes to a course at most once. A course acceptance or replacement takes
`O(log Cmax)` using its priority queue.

The matching phase is approximately:

```text
O(S + P log Cmax)
```

Memory usage is:

```text
O(total preferences + total accepted seats + priority-list data)
```

This is why the program can process large synthetic datasets efficiently.

## License

MIT
