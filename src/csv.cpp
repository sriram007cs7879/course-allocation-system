#include "course_allocation/csv.hpp"

#include <charconv>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

namespace course_allocation {
namespace {

[[nodiscard]] std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

[[nodiscard]] std::vector<std::string> split_list(const std::string& field) {
    std::vector<std::string> values;
    std::stringstream stream(field);
    std::string value;

    while (std::getline(stream, value, ';')) {
        value = trim(std::move(value));
        if (!value.empty()) {
            values.push_back(std::move(value));
        }
    }
    return values;
}

[[nodiscard]] std::size_t parse_size(
    const std::string& value,
    const std::filesystem::path& path,
    std::size_t line_number,
    std::string_view column
) {
    std::size_t parsed = 0;
    const char* begin = value.data();
    const char* end = value.data() + value.size();
    const auto [position, error] = std::from_chars(begin, end, parsed);

    if (error != std::errc{} || position != end) {
        throw std::runtime_error(
            path.string() + ":" + std::to_string(line_number) +
            ": invalid non-negative integer in column " + std::string(column)
        );
    }
    return parsed;
}

[[nodiscard]] std::unordered_map<std::string, std::size_t> header_index(
    const std::vector<std::string>& header
) {
    std::unordered_map<std::string, std::size_t> columns;
    for (std::size_t index = 0; index < header.size(); ++index) {
        columns.emplace(trim(header[index]), index);
    }
    return columns;
}

[[nodiscard]] const std::string& required_field(
    const std::vector<std::string>& row,
    const std::unordered_map<std::string, std::size_t>& columns,
    std::string_view name,
    const std::filesystem::path& path,
    std::size_t line_number
) {
    const auto column = columns.find(std::string(name));
    if (column == columns.end()) {
        throw std::runtime_error(path.string() + ": missing required column " + std::string(name));
    }
    if (column->second >= row.size()) {
        throw std::runtime_error(
            path.string() + ":" + std::to_string(line_number) +
            ": missing value for column " + std::string(name)
        );
    }
    return row[column->second];
}

[[nodiscard]] std::vector<std::vector<std::string>> read_rows(
    const std::filesystem::path& path
) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Unable to open " + path.string());
    }

    std::vector<std::vector<std::string>> rows;
    std::string line;
    while (std::getline(input, line)) {
        if (trim(line).empty() || trim(line).starts_with('#')) {
            continue;
        }
        rows.push_back(parse_csv_row(line));
    }

    if (rows.empty()) {
        throw std::runtime_error(path.string() + " is empty");
    }
    return rows;
}

[[nodiscard]] std::string join_list(const std::vector<std::string>& values) {
    std::ostringstream output;
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) {
            output << ';';
        }
        output << values[index];
    }
    return output.str();
}

}  // namespace

std::vector<std::string> parse_csv_row(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool in_quotes = false;

    for (std::size_t index = 0; index < line.size(); ++index) {
        const char character = line[index];

        if (in_quotes) {
            if (character == '"' && index + 1 < line.size() && line[index + 1] == '"') {
                field.push_back('"');
                ++index;
            } else if (character == '"') {
                in_quotes = false;
            } else {
                field.push_back(character);
            }
        } else if (character == '"') {
            in_quotes = true;
        } else if (character == ',') {
            fields.push_back(trim(std::move(field)));
            field.clear();
        } else {
            field.push_back(character);
        }
    }

    if (in_quotes) {
        throw std::runtime_error("Unterminated quoted CSV field");
    }

    fields.push_back(trim(std::move(field)));
    return fields;
}

std::string escape_csv_field(const std::string& field) {
    if (field.find_first_of(",\"\r\n") == std::string::npos) {
        return field;
    }

    std::string escaped = "\"";
    for (const char character : field) {
        if (character == '"') {
            escaped += "\"\"";
        } else {
            escaped.push_back(character);
        }
    }
    escaped.push_back('"');
    return escaped;
}

std::vector<Student> read_students_csv(const std::filesystem::path& path) {
    const auto rows = read_rows(path);
    const auto columns = header_index(rows.front());
    std::vector<Student> students;

    for (std::size_t index = 1; index < rows.size(); ++index) {
        const std::size_t line_number = index + 1;
        const auto& row = rows[index];

        Student student;
        student.id = trim(required_field(row, columns, "student_id", path, line_number));
        student.name = trim(required_field(row, columns, "name", path, line_number));
        student.max_courses = parse_size(
            required_field(row, columns, "max_courses", path, line_number),
            path,
            line_number,
            "max_courses"
        );

        for (std::string course : split_list(
                 required_field(row, columns, "completed_courses", path, line_number)
             )) {
            student.completed_courses.insert(std::move(course));
        }
        student.preferences = split_list(
            required_field(row, columns, "preferences", path, line_number)
        );
        students.push_back(std::move(student));
    }

    return students;
}

std::vector<Course> read_courses_csv(const std::filesystem::path& path) {
    const auto rows = read_rows(path);
    const auto columns = header_index(rows.front());
    std::vector<Course> courses;

    for (std::size_t index = 1; index < rows.size(); ++index) {
        const std::size_t line_number = index + 1;
        const auto& row = rows[index];

        Course course;
        course.id = trim(required_field(row, columns, "course_id", path, line_number));
        course.name = trim(required_field(row, columns, "name", path, line_number));
        course.capacity = parse_size(
            required_field(row, columns, "capacity", path, line_number),
            path,
            line_number,
            "capacity"
        );
        course.timeslot = trim(required_field(row, columns, "timeslot", path, line_number));
        course.prerequisites = split_list(
            required_field(row, columns, "prerequisites", path, line_number)
        );
        course.priority_order = split_list(
            required_field(row, columns, "priority_order", path, line_number)
        );
        courses.push_back(std::move(course));
    }

    return courses;
}

void write_allocations_csv(
    const std::filesystem::path& path,
    const std::vector<Student>& students,
    const AllocationResult& result
) {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Unable to write " + path.string());
    }

    output << "student_id,name,assigned_courses\n";
    for (const Student& student : students) {
        output << escape_csv_field(student.id) << ','
               << escape_csv_field(student.name) << ','
               << escape_csv_field(join_list(result.student_courses.at(student.id))) << '\n';
    }
}

}  // namespace course_allocation

