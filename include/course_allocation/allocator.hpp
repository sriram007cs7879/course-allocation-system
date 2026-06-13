#pragma once

#include "course_allocation/models.hpp"

#include <vector>

namespace course_allocation {

class CourseAllocator {
public:
    CourseAllocator(std::vector<Student> students, std::vector<Course> courses);

    [[nodiscard]] AllocationResult allocate() const;

private:
    std::vector<Student> students_;
    std::vector<Course> courses_;

    void validate() const;
};

}  // namespace course_allocation

