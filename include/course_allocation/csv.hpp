#pragma once

#include "course_allocation/models.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace course_allocation {

[[nodiscard]] std::vector<Student> read_students_csv(const std::filesystem::path& path);
[[nodiscard]] std::vector<Course> read_courses_csv(const std::filesystem::path& path);

void write_allocations_csv(
    const std::filesystem::path& path,
    const std::vector<Student>& students,
    const AllocationResult& result
);

[[nodiscard]] std::vector<std::string> parse_csv_row(const std::string& line);
[[nodiscard]] std::string escape_csv_field(const std::string& field);

}  // namespace course_allocation

