# Allocation Algorithm

## Model

The system solves a capacity-constrained, many-to-many matching problem:

- each student has an ordered course preference list and a maximum course load;
- each course has a capacity and an ordered student priority list;
- prerequisites are checked against courses completed before allocation;
- courses sharing a timeslot cannot both be assigned to the same student.

Students propose in preference order. Courses tentatively retain their highest-priority
eligible proposers up to capacity.

## Modified Gale-Shapley Procedure

1. Add every student to the active queue.
2. An active student proposes to the highest-ranked course that:
   - has not already rejected or tentatively accepted that student;
   - has satisfied prerequisites;
   - does not conflict with an existing tentative assignment.
3. If the course has a free seat, it tentatively accepts the student.
4. Otherwise, compare the proposer with the course's worst tentative assignee:
   - if the proposer has higher priority, replace the worst assignee;
   - otherwise reject the proposal.
5. A displaced student returns to the active queue and continues proposing.
6. Stop when no student can make another feasible proposal or every student has filled
   their quota.

Each course stores tentative assignees in a max-heap keyed by `(priority rank, student ID)`.
The heap root is therefore the worst currently accepted student and can be replaced in
`O(log capacity)` time.

## Determinism

Explicit priority order is used first. Students omitted from a course's priority list are
tied after listed students and are ordered lexicographically by ID. This makes repeated
runs deterministic.

## Complexity

Let:

- `P` be the number of proposals;
- `Cmax` be the largest course capacity;
- `D` be the number of dynamic timetable-conflict checks.

The matching phase runs in `O(P log Cmax + D)`. Every actual student-course proposal is
made at most once. Memory usage is linear in input size plus accepted seats.

