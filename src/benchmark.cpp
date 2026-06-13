#include "course_allocation/allocator.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

[[nodiscard]] std::size_t parse_count(const char* value, const char* name) {
    try {
        const auto parsed = std::stoull(value);
        if (parsed == 0) {
            throw std::invalid_argument("zero");
        }
        return static_cast<std::size_t>(parsed);
    } catch (...) {
        std::cerr << name << " must be a positive integer\n";
        std::exit(1);
    }
}

}  // namespace

int main(int argc, char** argv) {
    const std::size_t student_count = argc > 1 ? parse_count(argv[1], "student_count") : 50'000;
    const std::size_t course_count = argc > 2 ? parse_count(argv[2], "course_count") : 500;
    const std::size_t preferences_per_student = std::min<std::size_t>(20, course_count);

    std::mt19937_64 generator(42);
    std::vector<course_allocation::Course> courses;
    std::vector<course_allocation::Student> students;
    courses.reserve(course_count);
    students.reserve(student_count);

    for (std::size_t index = 0; index < course_count; ++index) {
        course_allocation::Course course;
        course.id = "C" + std::to_string(index);
        course.name = "Generated Course " + std::to_string(index);
        course.capacity = std::max<std::size_t>(1, student_count * 3 / course_count);
        course.timeslot = "T" + std::to_string(index % 25);
        if (index >= 5 && index % 4 == 0) {
            course.prerequisites.push_back("P" + std::to_string(index % 5));
        }
        courses.push_back(std::move(course));
    }

    std::vector<std::size_t> course_indices(course_count);
    std::iota(course_indices.begin(), course_indices.end(), 0);

    for (std::size_t index = 0; index < student_count; ++index) {
        course_allocation::Student student;
        student.id = "S" + std::to_string(index);
        student.name = "Generated Student " + std::to_string(index);
        student.max_courses = 3;
        for (std::size_t prerequisite = 0; prerequisite < 5; ++prerequisite) {
            if ((index + prerequisite) % 3 != 0) {
                student.completed_courses.insert("P" + std::to_string(prerequisite));
            }
        }

        std::shuffle(course_indices.begin(), course_indices.end(), generator);
        for (std::size_t preference = 0; preference < preferences_per_student; ++preference) {
            student.preferences.push_back(courses[course_indices[preference]].id);
        }
        students.push_back(std::move(student));
    }

    const std::size_t explicit_priority_count = std::min<std::size_t>(2'000, student_count);
    for (auto& course : courses) {
        std::vector<std::size_t> student_indices(student_count);
        std::iota(student_indices.begin(), student_indices.end(), 0);
        std::shuffle(student_indices.begin(), student_indices.end(), generator);
        course.priority_order.reserve(explicit_priority_count);
        for (std::size_t rank = 0; rank < explicit_priority_count; ++rank) {
            course.priority_order.push_back(students[student_indices[rank]].id);
        }
    }

    const auto start = std::chrono::steady_clock::now();
    const course_allocation::CourseAllocator allocator(std::move(students), std::move(courses));
    const auto result = allocator.allocate();
    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed = end - start;

    std::size_t allocated_seats = 0;
    for (const auto& [course_id, assigned] : result.course_students) {
        static_cast<void>(course_id);
        allocated_seats += assigned.size();
    }

    std::cout << "students=" << student_count
              << " courses=" << course_count
              << " preferences/student=" << preferences_per_student << '\n'
              << "allocated_seats=" << allocated_seats
              << " proposals=" << result.stats.proposals
              << " displacements=" << result.stats.displacements << '\n'
              << std::fixed << std::setprecision(3)
              << "elapsed_seconds=" << elapsed.count() << '\n';
}
