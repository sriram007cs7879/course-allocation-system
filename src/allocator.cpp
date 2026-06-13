#include "course_allocation/allocator.hpp"

#include <algorithm>
#include <functional>
#include <limits>
#include <queue>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace course_allocation {
namespace {

using CandidateQuality = std::pair<std::size_t, std::string>;

struct StudentState {
    std::unordered_set<std::string> assignments;
    std::unordered_set<std::string> proposed_courses;
};

struct CourseState {
    std::priority_queue<CandidateQuality> accepted;
};

[[nodiscard]] bool has_completed_prerequisites(const Student& student, const Course& course) {
    return std::ranges::all_of(course.prerequisites, [&](const std::string& prerequisite) {
        return student.completed_courses.contains(prerequisite);
    });
}

[[nodiscard]] bool has_schedule_conflict(
    const StudentState& state,
    const Course& candidate,
    const std::unordered_map<std::string, const Course*>& courses_by_id
) {
    if (candidate.timeslot.empty()) {
        return false;
    }

    return std::ranges::any_of(state.assignments, [&](const std::string& assigned_id) {
        const Course& assigned = *courses_by_id.at(assigned_id);
        return !assigned.timeslot.empty() && assigned.timeslot == candidate.timeslot;
    });
}

}  // namespace

CourseAllocator::CourseAllocator(std::vector<Student> students, std::vector<Course> courses)
    : students_(std::move(students)), courses_(std::move(courses)) {
    validate();
}

void CourseAllocator::validate() const {
    std::unordered_set<std::string> student_ids;
    std::unordered_set<std::string> course_ids;

    for (const Student& student : students_) {
        if (student.id.empty()) {
            throw std::invalid_argument("Student IDs cannot be empty");
        }
        if (!student_ids.insert(student.id).second) {
            throw std::invalid_argument("Duplicate student ID: " + student.id);
        }
    }

    for (const Course& course : courses_) {
        if (course.id.empty()) {
            throw std::invalid_argument("Course IDs cannot be empty");
        }
        if (!course_ids.insert(course.id).second) {
            throw std::invalid_argument("Duplicate course ID: " + course.id);
        }
    }

    for (const Student& student : students_) {
        std::unordered_set<std::string> seen_preferences;
        for (const std::string& course_id : student.preferences) {
            if (!course_ids.contains(course_id)) {
                throw std::invalid_argument(
                    "Student " + student.id + " prefers unknown course " + course_id
                );
            }
            if (!seen_preferences.insert(course_id).second) {
                throw std::invalid_argument(
                    "Student " + student.id + " lists course " + course_id + " more than once"
                );
            }
        }
    }

    for (const Course& course : courses_) {
        std::unordered_set<std::string> seen_students;
        for (const std::string& student_id : course.priority_order) {
            if (!student_ids.contains(student_id)) {
                throw std::invalid_argument(
                    "Course " + course.id + " prioritizes unknown student " + student_id
                );
            }
            if (!seen_students.insert(student_id).second) {
                throw std::invalid_argument(
                    "Course " + course.id + " lists student " + student_id + " more than once"
                );
            }
        }
    }
}

AllocationResult CourseAllocator::allocate() const {
    std::unordered_map<std::string, const Student*> students_by_id;
    std::unordered_map<std::string, const Course*> courses_by_id;
    std::unordered_map<std::string, std::unordered_map<std::string, std::size_t>> priority_ranks;
    std::unordered_map<std::string, StudentState> student_states;
    std::unordered_map<std::string, CourseState> course_states;

    for (const Student& student : students_) {
        students_by_id.emplace(student.id, &student);
        student_states.emplace(student.id, StudentState{});
    }

    for (const Course& course : courses_) {
        courses_by_id.emplace(course.id, &course);
        course_states.emplace(course.id, CourseState{});

        auto& ranks = priority_ranks[course.id];
        for (std::size_t rank = 0; rank < course.priority_order.size(); ++rank) {
            ranks.emplace(course.priority_order[rank], rank);
        }
    }

    const auto quality_for = [&](const Course& course, const std::string& student_id) {
        const auto& ranks = priority_ranks.at(course.id);
        const auto rank_it = ranks.find(student_id);
        const std::size_t fallback_rank = course.priority_order.size();
        return CandidateQuality{
            rank_it == ranks.end() ? fallback_rank : rank_it->second,
            student_id
        };
    };

    std::queue<std::string> active_students;
    std::unordered_set<std::string> queued_students;

    const auto enqueue = [&](const std::string& student_id) {
        if (queued_students.insert(student_id).second) {
            active_students.push(student_id);
        }
    };

    for (const Student& student : students_) {
        enqueue(student.id);
    }

    AllocationResult result;

    while (!active_students.empty()) {
        const std::string student_id = active_students.front();
        active_students.pop();
        queued_students.erase(student_id);

        const Student& student = *students_by_id.at(student_id);
        StudentState& student_state = student_states.at(student_id);

        while (student_state.assignments.size() < student.max_courses) {
            const Course* selected_course = nullptr;

            for (const std::string& course_id : student.preferences) {
                if (student_state.proposed_courses.contains(course_id)) {
                    continue;
                }

                const Course& course = *courses_by_id.at(course_id);
                if (!has_completed_prerequisites(student, course)) {
                    student_state.proposed_courses.insert(course_id);
                    ++result.stats.prerequisite_skips;
                    continue;
                }

                if (has_schedule_conflict(student_state, course, courses_by_id)) {
                    ++result.stats.schedule_conflict_skips;
                    continue;
                }

                selected_course = &course;
                break;
            }

            if (selected_course == nullptr) {
                break;
            }

            const Course& course = *selected_course;
            student_state.proposed_courses.insert(course.id);
            ++result.stats.proposals;

            CourseState& course_state = course_states.at(course.id);
            const CandidateQuality candidate = quality_for(course, student.id);

            if (course.capacity == 0) {
                ++result.stats.rejections;
                continue;
            }

            if (course_state.accepted.size() < course.capacity) {
                course_state.accepted.push(candidate);
                student_state.assignments.insert(course.id);
                ++result.stats.acceptances;
                continue;
            }

            const CandidateQuality worst_accepted = course_state.accepted.top();
            if (candidate < worst_accepted) {
                course_state.accepted.pop();
                course_state.accepted.push(candidate);

                StudentState& displaced_state = student_states.at(worst_accepted.second);
                displaced_state.assignments.erase(course.id);
                enqueue(worst_accepted.second);

                student_state.assignments.insert(course.id);
                ++result.stats.acceptances;
                ++result.stats.displacements;
            } else {
                ++result.stats.rejections;
            }
        }
    }

    for (const Student& student : students_) {
        const StudentState& state = student_states.at(student.id);
        auto& assigned = result.student_courses[student.id];

        for (const std::string& preferred_course : student.preferences) {
            if (state.assignments.contains(preferred_course)) {
                assigned.push_back(preferred_course);
            }
        }
    }

    for (const Course& course : courses_) {
        auto heap = course_states.at(course.id).accepted;
        std::vector<CandidateQuality> accepted;
        accepted.reserve(heap.size());

        while (!heap.empty()) {
            accepted.push_back(heap.top());
            heap.pop();
        }
        std::ranges::sort(accepted);

        auto& assigned = result.course_students[course.id];
        for (const auto& [rank, student_id] : accepted) {
            static_cast<void>(rank);
            assigned.push_back(student_id);
        }
    }

    return result;
}

}  // namespace course_allocation
