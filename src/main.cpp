#include "course_allocation/allocator.hpp"
#include "course_allocation/csv.hpp"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

struct Options {
    std::filesystem::path students_path;
    std::filesystem::path courses_path;
    std::filesystem::path output_path = "allocations.csv";
};

void print_usage(const char* program) {
    std::cout
        << "Usage: " << program
        << " --students <students.csv> --courses <courses.csv> [--output <allocations.csv>]\n\n"
        << "CSV list fields use semicolons, for example: CS101;MA101;EE101\n";
}

[[nodiscard]] Options parse_options(int argc, char** argv) {
    Options options;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        const auto require_value = [&](const std::string& name) -> std::string {
            if (index + 1 >= argc) {
                throw std::invalid_argument("Missing value after " + name);
            }
            return argv[++index];
        };

        if (argument == "--students") {
            options.students_path = require_value(argument);
        } else if (argument == "--courses") {
            options.courses_path = require_value(argument);
        } else if (argument == "--output") {
            options.output_path = require_value(argument);
        } else if (argument == "--help" || argument == "-h") {
            print_usage(argv[0]);
            std::exit(0);
        } else {
            throw std::invalid_argument("Unknown argument: " + argument);
        }
    }

    if (options.students_path.empty() || options.courses_path.empty()) {
        throw std::invalid_argument("--students and --courses are required");
    }
    return options;
}

void print_summary(
    const std::vector<course_allocation::Student>& students,
    const std::vector<course_allocation::Course>& courses,
    const course_allocation::AllocationResult& result
) {
    std::unordered_map<std::string, std::string> student_names;
    for (const auto& student : students) {
        student_names.emplace(student.id, student.name);
    }

    std::size_t assigned_students = 0;
    std::size_t assigned_seats = 0;
    for (const auto& [student_id, assigned] : result.student_courses) {
        static_cast<void>(student_id);
        assigned_seats += assigned.size();
        assigned_students += !assigned.empty();
    }

    std::cout << "\nAllocation summary\n"
              << "------------------\n"
              << "Students with at least one course: " << assigned_students << '/'
              << students.size() << '\n'
              << "Total seats assigned: " << assigned_seats << "\n\n";

    for (const auto& course : courses) {
        const auto& assigned = result.course_students.at(course.id);
        std::cout << std::left << std::setw(8) << course.id << " "
                  << std::setw(28) << course.name.substr(0, 27) << " "
                  << assigned.size() << '/' << course.capacity << " seats";

        if (!assigned.empty()) {
            std::cout << " [";
            for (std::size_t index = 0; index < assigned.size(); ++index) {
                if (index != 0) {
                    std::cout << ", ";
                }
                std::cout << student_names.at(assigned[index]);
            }
            std::cout << ']';
        }
        std::cout << '\n';
    }

    const auto& stats = result.stats;
    std::cout << "\nProposals: " << stats.proposals
              << " | accepted: " << stats.acceptances
              << " | rejected: " << stats.rejections
              << " | displacements: " << stats.displacements
              << "\nPrerequisite skips: " << stats.prerequisite_skips
              << " | schedule-conflict skips: " << stats.schedule_conflict_skips
              << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_options(argc, argv);
        const auto students = course_allocation::read_students_csv(options.students_path);
        const auto courses = course_allocation::read_courses_csv(options.courses_path);

        const course_allocation::CourseAllocator allocator(students, courses);
        const auto result = allocator.allocate();

        course_allocation::write_allocations_csv(options.output_path, students, result);
        print_summary(students, courses, result);
        std::cout << "\nWrote student allocations to " << options.output_path << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        print_usage(argv[0]);
        return 1;
    }
}

