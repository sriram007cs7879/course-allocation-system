#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

namespace course_allocation {

struct Student {
    std::string id;
    std::string name;
    std::size_t max_courses = 1;
    std::unordered_set<std::string> completed_courses;
    std::vector<std::string> preferences;
};

struct Course {
    std::string id;
    std::string name;
    std::size_t capacity = 0;
    std::string timeslot;
    std::vector<std::string> prerequisites;
    std::vector<std::string> priority_order;
};

struct AllocationStats {
    std::size_t proposals = 0;
    std::size_t acceptances = 0;
    std::size_t rejections = 0;
    std::size_t displacements = 0;
    std::size_t prerequisite_skips = 0;
    std::size_t schedule_conflict_skips = 0;
};

struct AllocationResult {
    std::map<std::string, std::vector<std::string>> student_courses;
    std::map<std::string, std::vector<std::string>> course_students;
    AllocationStats stats;
};

}  // namespace course_allocation

