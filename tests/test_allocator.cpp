#include "course_allocation/allocator.hpp"
#include "course_allocation/csv.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using course_allocation::Course;
using course_allocation::CourseAllocator;
using course_allocation::Student;

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void test_course_priority_displaces_weaker_candidate() {
    std::vector<Student> students{
        {
            .id = "alice",
            .name = "Alice",
            .max_courses = 1,
            .completed_courses = {},
            .preferences = {"CS101"},
        },
        {
            .id = "bob",
            .name = "Bob",
            .max_courses = 1,
            .completed_courses = {},
            .preferences = {"CS101"},
        },
    };
    std::vector<Course> courses{
        {
            .id = "CS101",
            .name = "Programming",
            .capacity = 1,
            .timeslot = "M09",
            .prerequisites = {},
            .priority_order = {"bob", "alice"},
        },
    };

    const auto result = CourseAllocator(students, courses).allocate();
    require(result.course_students.at("CS101") == std::vector<std::string>{"bob"},
            "Bob should displace Alice based on course priority");
    require(result.student_courses.at("alice").empty(), "Alice should be displaced");
    require(result.stats.displacements == 1, "Exactly one displacement should occur");
}

void test_prerequisites_are_enforced() {
    std::vector<Student> students{
        {
            .id = "ready",
            .name = "Ready",
            .max_courses = 1,
            .completed_courses = {"CS101"},
            .preferences = {"CS201"},
        },
        {
            .id = "not-ready",
            .name = "Not Ready",
            .max_courses = 1,
            .completed_courses = {},
            .preferences = {"CS201"},
        },
    };
    std::vector<Course> courses{
        {
            .id = "CS201",
            .name = "Algorithms",
            .capacity = 2,
            .timeslot = "T10",
            .prerequisites = {"CS101"},
            .priority_order = {"not-ready", "ready"},
        },
    };

    const auto result = CourseAllocator(students, courses).allocate();
    require(result.course_students.at("CS201") == std::vector<std::string>{"ready"},
            "Only the student with CS101 should be eligible");
    require(result.stats.prerequisite_skips == 1, "One prerequisite skip should be recorded");
}

void test_student_quota_and_schedule_conflicts() {
    std::vector<Student> students{
        {
            .id = "s1",
            .name = "Student",
            .max_courses = 2,
            .completed_courses = {},
            .preferences = {"A", "B", "C"},
        },
    };
    std::vector<Course> courses{
        {
            .id = "A",
            .name = "A",
            .capacity = 1,
            .timeslot = "M09",
            .prerequisites = {},
            .priority_order = {"s1"},
        },
        {
            .id = "B",
            .name = "B",
            .capacity = 1,
            .timeslot = "M09",
            .prerequisites = {},
            .priority_order = {"s1"},
        },
        {
            .id = "C",
            .name = "C",
            .capacity = 1,
            .timeslot = "T10",
            .prerequisites = {},
            .priority_order = {"s1"},
        },
    };

    const auto result = CourseAllocator(students, courses).allocate();
    require(result.student_courses.at("s1") == std::vector<std::string>({"A", "C"}),
            "The allocator should skip B because it conflicts with A");
    require(result.stats.schedule_conflict_skips >= 1, "A conflict skip should be recorded");
}

void test_rejected_student_continues_to_next_preference() {
    std::vector<Student> students{
        {
            .id = "preferred",
            .name = "Preferred",
            .max_courses = 1,
            .completed_courses = {},
            .preferences = {"A"},
        },
        {
            .id = "fallback",
            .name = "Fallback",
            .max_courses = 1,
            .completed_courses = {},
            .preferences = {"A", "B"},
        },
    };
    std::vector<Course> courses{
        {
            .id = "A",
            .name = "A",
            .capacity = 1,
            .timeslot = "M09",
            .prerequisites = {},
            .priority_order = {"preferred", "fallback"},
        },
        {
            .id = "B",
            .name = "B",
            .capacity = 1,
            .timeslot = "T09",
            .prerequisites = {},
            .priority_order = {"fallback"},
        },
    };

    const auto result = CourseAllocator(students, courses).allocate();
    require(result.student_courses.at("fallback") == std::vector<std::string>{"B"},
            "A rejected student should continue to the next preference");
}

void test_displacement_reconsiders_deferred_schedule_choice() {
    std::vector<Student> students{
        {
            .id = "first",
            .name = "First",
            .max_courses = 2,
            .completed_courses = {},
            .preferences = {"A", "B", "C"},
        },
        {
            .id = "priority",
            .name = "Priority",
            .max_courses = 1,
            .completed_courses = {},
            .preferences = {"A"},
        },
    };
    std::vector<Course> courses{
        {
            .id = "A",
            .name = "A",
            .capacity = 1,
            .timeslot = "M09",
            .prerequisites = {},
            .priority_order = {"priority", "first"},
        },
        {
            .id = "B",
            .name = "B",
            .capacity = 1,
            .timeslot = "M09",
            .prerequisites = {},
            .priority_order = {"first"},
        },
        {
            .id = "C",
            .name = "C",
            .capacity = 1,
            .timeslot = "T10",
            .prerequisites = {},
            .priority_order = {"first"},
        },
    };

    const auto result = CourseAllocator(students, courses).allocate();
    require(result.student_courses.at("first") == std::vector<std::string>({"B", "C"}),
            "After losing A, the student should reconsider the previously conflicting B");
    require(result.student_courses.at("priority") == std::vector<std::string>{"A"},
            "The higher-priority student should receive A");
}

void test_csv_quotes_and_escaped_quotes() {
    const auto fields = course_allocation::parse_csv_row(
        "S1,\"Rao, Anya\",2,\"CS101;MA101\",\"CS201;EE200\""
    );
    require(fields.size() == 5, "CSV parser should return five fields");
    require(fields[1] == "Rao, Anya", "CSV parser should preserve commas inside quotes");
    require(course_allocation::escape_csv_field("Ada \"Ace\", Rao") == "\"Ada \"\"Ace\"\", Rao\"",
            "CSV writer should escape quotes and commas");
}

void test_invalid_preference_is_rejected() {
    bool threw = false;
    try {
        static_cast<void>(CourseAllocator(
            {{
                .id = "s",
                .name = "S",
                .max_courses = 1,
                .completed_courses = {},
                .preferences = {"UNKNOWN"},
            }},
            {{
                .id = "KNOWN",
                .name = "Known",
                .capacity = 1,
                .timeslot = {},
                .prerequisites = {},
                .priority_order = {},
            }}
        ));
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    require(threw, "Unknown preferred courses should fail validation");
}

}  // namespace

int main() {
    try {
        test_course_priority_displaces_weaker_candidate();
        test_prerequisites_are_enforced();
        test_student_quota_and_schedule_conflicts();
        test_rejected_student_continues_to_next_preference();
        test_displacement_reconsiders_deferred_schedule_choice();
        test_csv_quotes_and_escaped_quotes();
        test_invalid_preference_is_rejected();
        std::cout << "All course allocation tests passed.\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
