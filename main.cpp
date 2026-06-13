#include <bits/stdc++.h>
using namespace std;

struct Student {
    string id, name;
    int maxCourses = 1;
    unordered_set<string> completed;
    vector<string> preferences;
};

struct Course {
    string id, name, timeSlot;
    int capacity = 0;
    vector<string> prerequisites;
    vector<string> priority;
};

struct Stats {
    long long proposals = 0;
    long long acceptances = 0;
    long long rejections = 0;
    long long displacements = 0;
    long long prerequisiteSkips = 0;
    long long conflictSkips = 0;
};

struct Result {
    map<string, vector<string>> studentCourses;
    map<string, vector<string>> courseStudents;
    Stats stats;
};

string trim(string s) {
    int left = 0;
    while (left < (int)s.size() && isspace((unsigned char)s[left])) {
        left++;
    }

    int right = (int)s.size() - 1;
    while (right >= left && isspace((unsigned char)s[right])) {
        right--;
    }

    return s.substr(left, right - left + 1);
}

vector<string> parseCsvLine(const string& line) {
    vector<string> fields;
    string current;
    bool insideQuotes = false;

    for (int i = 0; i < (int)line.size(); i++) {
        char ch = line[i];

        if (insideQuotes) {
            if (ch == '"' && i + 1 < (int)line.size() && line[i + 1] == '"') {
                current += '"';
                i++;
            } else if (ch == '"') {
                insideQuotes = false;
            } else {
                current += ch;
            }
        } else if (ch == '"') {
            insideQuotes = true;
        } else if (ch == ',') {
            fields.push_back(trim(current));
            current.clear();
        } else {
            current += ch;
        }
    }

    if (insideQuotes) {
        throw runtime_error("Unterminated quoted CSV field");
    }

    fields.push_back(trim(current));
    return fields;
}

vector<string> splitList(const string& text) {
    vector<string> values;
    string value;
    stringstream ss(text);

    while (getline(ss, value, ';')) {
        value = trim(value);
        if (!value.empty()) {
            values.push_back(value);
        }
    }

    return values;
}

vector<vector<string>> readCsv(const string& path) {
    ifstream file(path);
    if (!file) {
        throw runtime_error("Could not open " + path);
    }

    vector<vector<string>> rows;
    string line;

    while (getline(file, line)) {
        string cleaned = trim(line);
        if (!cleaned.empty() && cleaned[0] != '#') {
            rows.push_back(parseCsvLine(line));
        }
    }

    if (rows.empty()) {
        throw runtime_error(path + " is empty");
    }

    return rows;
}

unordered_map<string, int> getColumns(const vector<string>& header) {
    unordered_map<string, int> column;
    for (int i = 0; i < (int)header.size(); i++) {
        column[trim(header[i])] = i;
    }
    return column;
}

string getField(
    const vector<string>& row,
    const unordered_map<string, int>& column,
    const string& name
) {
    if (!column.count(name) || column.at(name) >= (int)row.size()) {
        throw runtime_error("Missing CSV field: " + name);
    }
    return row[column.at(name)];
}

vector<Student> readStudents(const string& path) {
    vector<vector<string>> rows = readCsv(path);
    unordered_map<string, int> column = getColumns(rows[0]);
    vector<Student> students;

    for (int i = 1; i < (int)rows.size(); i++) {
        Student student;
        student.id = trim(getField(rows[i], column, "student_id"));
        student.name = trim(getField(rows[i], column, "name"));
        student.maxCourses = stoi(getField(rows[i], column, "max_courses"));
        student.preferences = splitList(getField(rows[i], column, "preferences"));

        for (const string& course : splitList(
                 getField(rows[i], column, "completed_courses")
             )) {
            student.completed.insert(course);
        }

        students.push_back(student);
    }

    return students;
}

vector<Course> readCourses(const string& path) {
    vector<vector<string>> rows = readCsv(path);
    unordered_map<string, int> column = getColumns(rows[0]);
    vector<Course> courses;

    for (int i = 1; i < (int)rows.size(); i++) {
        Course course;
        course.id = trim(getField(rows[i], column, "course_id"));
        course.name = trim(getField(rows[i], column, "name"));
        course.capacity = stoi(getField(rows[i], column, "capacity"));
        course.timeSlot = trim(getField(rows[i], column, "timeslot"));
        course.prerequisites = splitList(getField(rows[i], column, "prerequisites"));
        course.priority = splitList(getField(rows[i], column, "priority_order"));
        courses.push_back(course);
    }

    return courses;
}

void validateInput(const vector<Student>& students, const vector<Course>& courses) {
    unordered_set<string> studentIds, courseIds;

    for (const Student& student : students) {
        if (student.id.empty() || !studentIds.insert(student.id).second) {
            throw runtime_error("Invalid or duplicate student ID: " + student.id);
        }
        if (student.maxCourses < 0) {
            throw runtime_error("Student course limit cannot be negative");
        }
    }

    for (const Course& course : courses) {
        if (course.id.empty() || !courseIds.insert(course.id).second) {
            throw runtime_error("Invalid or duplicate course ID: " + course.id);
        }
        if (course.capacity < 0) {
            throw runtime_error("Course capacity cannot be negative");
        }
    }

    for (const Student& student : students) {
        unordered_set<string> seen;
        for (const string& courseId : student.preferences) {
            if (!courseIds.count(courseId)) {
                throw runtime_error(student.id + " prefers unknown course " + courseId);
            }
            if (!seen.insert(courseId).second) {
                throw runtime_error(student.id + " lists " + courseId + " twice");
            }
        }
    }

    for (const Course& course : courses) {
        unordered_set<string> seen;
        for (const string& studentId : course.priority) {
            if (!studentIds.count(studentId)) {
                throw runtime_error(course.id + " ranks unknown student " + studentId);
            }
            if (!seen.insert(studentId).second) {
                throw runtime_error(course.id + " ranks " + studentId + " twice");
            }
        }
    }
}

bool prerequisitesSatisfied(const Student& student, const Course& course) {
    for (const string& prerequisite : course.prerequisites) {
        if (!student.completed.count(prerequisite)) {
            return false;
        }
    }
    return true;
}

bool hasConflict(
    const unordered_set<string>& assigned,
    const Course& candidate,
    const unordered_map<string, int>& courseIndex,
    const vector<Course>& courses
) {
    if (candidate.timeSlot.empty()) {
        return false;
    }

    for (const string& courseId : assigned) {
        const Course& current = courses[courseIndex.at(courseId)];
        if (!current.timeSlot.empty() && current.timeSlot == candidate.timeSlot) {
            return true;
        }
    }

    return false;
}

Result allocateCourses(const vector<Student>& students, const vector<Course>& courses) {
    validateInput(students, courses);

    int n = static_cast<int>(students.size());
    int m = static_cast<int>(courses.size());

    unordered_map<string, int> studentIndex, courseIndex;
    vector<unordered_map<string, int>> rank(m);

    for (int i = 0; i < n; i++) {
        studentIndex[students[i].id] = i;
    }

    for (int i = 0; i < m; i++) {
        courseIndex[courses[i].id] = i;
        for (int j = 0; j < (int)courses[i].priority.size(); j++) {
            rank[i][courses[i].priority[j]] = j;
        }
    }

    // Each course keeps its worst currently accepted student on top.
    using Candidate = pair<int, string>;
    vector<priority_queue<Candidate>> accepted(m);
    vector<unordered_set<string>> assigned(n);
    vector<unordered_set<string>> proposed(n);

    queue<int> active;
    vector<bool> inQueue(n, false);

    auto addToQueue = [&](int student) {
        if (!inQueue[student]) {
            active.push(student);
            inQueue[student] = true;
        }
    };

    auto candidateRank = [&](int course, const string& studentId) {
        if (rank[course].count(studentId)) {
            return rank[course][studentId];
        }
        return (int)courses[course].priority.size();
    };

    for (int i = 0; i < n; i++) {
        addToQueue(i);
    }

    Result result;

    while (!active.empty()) {
        int student = active.front();
        active.pop();
        inQueue[student] = false;

        while ((int)assigned[student].size() < students[student].maxCourses) {
            int chosenCourse = -1;

            for (const string& courseId : students[student].preferences) {
                if (proposed[student].count(courseId)) {
                    continue;
                }

                int course = courseIndex.at(courseId);

                if (!prerequisitesSatisfied(students[student], courses[course])) {
                    proposed[student].insert(courseId);
                    result.stats.prerequisiteSkips++;
                    continue;
                }

                // A conflicting course is deferred because a later displacement may
                // remove the current assignment and make this course feasible.
                if (hasConflict(assigned[student], courses[course], courseIndex, courses)) {
                    result.stats.conflictSkips++;
                    continue;
                }

                chosenCourse = course;
                break;
            }

            if (chosenCourse == -1) {
                break;
            }

            const Course& course = courses[chosenCourse];
            const string& studentId = students[student].id;
            proposed[student].insert(course.id);
            result.stats.proposals++;

            Candidate candidate = {candidateRank(chosenCourse, studentId), studentId};

            if (course.capacity == 0) {
                result.stats.rejections++;
            } else if ((int)accepted[chosenCourse].size() < course.capacity) {
                accepted[chosenCourse].push(candidate);
                assigned[student].insert(course.id);
                result.stats.acceptances++;
            } else if (candidate < accepted[chosenCourse].top()) {
                string displacedId = accepted[chosenCourse].top().second;
                accepted[chosenCourse].pop();
                accepted[chosenCourse].push(candidate);

                int displaced = studentIndex.at(displacedId);
                assigned[displaced].erase(course.id);
                addToQueue(displaced);

                assigned[student].insert(course.id);
                result.stats.acceptances++;
                result.stats.displacements++;
            } else {
                result.stats.rejections++;
            }
        }
    }

    for (int i = 0; i < n; i++) {
        for (const string& courseId : students[i].preferences) {
            if (assigned[i].count(courseId)) {
                result.studentCourses[students[i].id].push_back(courseId);
            }
        }
    }

    for (int i = 0; i < m; i++) {
        vector<Candidate> studentsInCourse;
        while (!accepted[i].empty()) {
            studentsInCourse.push_back(accepted[i].top());
            accepted[i].pop();
        }

        sort(studentsInCourse.begin(), studentsInCourse.end());
        for (const auto& [priorityRank, studentId] : studentsInCourse) {
            (void)priorityRank;
            result.courseStudents[courses[i].id].push_back(studentId);
        }
    }

    return result;
}

string joinList(const vector<string>& values) {
    string answer;
    for (int i = 0; i < (int)values.size(); i++) {
        if (i) {
            answer += ';';
        }
        answer += values[i];
    }
    return answer;
}

string escapeCsv(const string& field) {
    if (field.find_first_of(",\"\n\r") == string::npos) {
        return field;
    }

    string answer = "\"";
    for (char ch : field) {
        answer += ch == '"' ? "\"\"" : string(1, ch);
    }
    return answer + '"';
}

void writeOutput(const string& path, const vector<Student>& students, const Result& result) {
    ofstream file(path);
    if (!file) {
        throw runtime_error("Could not write " + path);
    }

    file << "student_id,name,assigned_courses\n";
    for (const Student& student : students) {
        file << escapeCsv(student.id) << ','
             << escapeCsv(student.name) << ','
             << escapeCsv(joinList(result.studentCourses.at(student.id))) << '\n';
    }
}

void printSummary(
    const vector<Student>& students,
    const vector<Course>& courses,
    const Result& result
) {
    unordered_map<string, string> studentName;
    for (const Student& student : students) {
        studentName[student.id] = student.name;
    }

    int studentsAllocated = 0;
    int totalSeats = 0;

    for (const Student& student : students) {
        int count = static_cast<int>(result.studentCourses.at(student.id).size());
        studentsAllocated += count > 0;
        totalSeats += count;
    }

    cout << "\nAllocation summary\n";
    cout << "------------------\n";
    cout << "Students allocated: " << studentsAllocated << '/' << students.size() << '\n';
    cout << "Total seats assigned: " << totalSeats << "\n\n";

    for (const Course& course : courses) {
        const vector<string>& chosen = result.courseStudents.at(course.id);
        cout << left << setw(8) << course.id
             << setw(30) << course.name.substr(0, 28)
             << chosen.size() << '/' << course.capacity << " seats";

        if (!chosen.empty()) {
            cout << " [";
            for (int i = 0; i < (int)chosen.size(); i++) {
                if (i) {
                    cout << ", ";
                }
                cout << studentName[chosen[i]];
            }
            cout << ']';
        }
        cout << '\n';
    }

    const Stats& s = result.stats;
    cout << "\nProposals: " << s.proposals
         << " | accepted: " << s.acceptances
         << " | rejected: " << s.rejections
         << " | displacements: " << s.displacements << '\n';
    cout << "Prerequisite skips: " << s.prerequisiteSkips
         << " | conflict skips: " << s.conflictSkips << '\n';
}

void runSelfTest() {
    vector<Student> students = {
        {"alice", "Alice", 2, {"CS100"}, {"A", "B", "C"}},
        {"bob", "Bob", 1, {"CS100"}, {"A"}},
        {"cara", "Cara", 1, {}, {"A", "C"}}
    };

    vector<Course> courses = {
        {"A", "Algorithms", "MON-09", 1, {"CS100"}, {"bob", "alice", "cara"}},
        {"B", "Databases", "MON-09", 1, {}, {"alice", "bob", "cara"}},
        {"C", "Humanities", "TUE-10", 2, {}, {"cara", "alice", "bob"}}
    };

    Result result = allocateCourses(students, courses);

    if (result.studentCourses["alice"] != vector<string>({"B", "C"})) {
        throw runtime_error("Self-test failed: Alice's allocation");
    }
    if (result.studentCourses["bob"] != vector<string>({"A"})) {
        throw runtime_error("Self-test failed: Bob's allocation");
    }
    if (result.studentCourses["cara"] != vector<string>({"C"})) {
        throw runtime_error("Self-test failed: prerequisite handling");
    }

    cout << "All self-tests passed.\n";
}

void runBenchmark(int studentCount, int courseCount) {
    if (studentCount <= 0 || courseCount <= 0) {
        throw runtime_error("Benchmark sizes must be positive");
    }

    mt19937 rng(42);
    vector<Student> students(studentCount);
    vector<Course> courses(courseCount);

    for (int i = 0; i < courseCount; i++) {
        courses[i].id = "C" + to_string(i);
        courses[i].name = "Course " + to_string(i);
        courses[i].timeSlot = "T" + to_string(i % 25);
        courses[i].capacity = max(1, studentCount * 3 / courseCount);
        if (i >= 5 && i % 4 == 0) {
            courses[i].prerequisites.push_back("P" + to_string(i % 5));
        }
    }

    vector<int> courseOrder(courseCount);
    iota(courseOrder.begin(), courseOrder.end(), 0);

    for (int i = 0; i < studentCount; i++) {
        students[i].id = "S" + to_string(i);
        students[i].name = "Student " + to_string(i);
        students[i].maxCourses = 3;

        for (int prerequisite = 0; prerequisite < 5; prerequisite++) {
            if ((i + prerequisite) % 3 != 0) {
                students[i].completed.insert("P" + to_string(prerequisite));
            }
        }

        shuffle(courseOrder.begin(), courseOrder.end(), rng);
        for (int j = 0; j < min(20, courseCount); j++) {
            students[i].preferences.push_back(courses[courseOrder[j]].id);
        }
    }

    vector<int> studentOrder(studentCount);
    iota(studentOrder.begin(), studentOrder.end(), 0);

    for (Course& course : courses) {
        shuffle(studentOrder.begin(), studentOrder.end(), rng);
        for (int i = 0; i < min(2000, studentCount); i++) {
            course.priority.push_back(students[studentOrder[i]].id);
        }
    }

    auto start = chrono::steady_clock::now();
    Result result = allocateCourses(students, courses);
    auto finish = chrono::steady_clock::now();

    long long seats = 0;
    for (const auto& [courseId, chosen] : result.courseStudents) {
        (void)courseId;
        seats += static_cast<long long>(chosen.size());
    }

    double seconds = chrono::duration<double>(finish - start).count();
    cout << "Students: " << studentCount << '\n';
    cout << "Courses: " << courseCount << '\n';
    cout << "Allocated seats: " << seats << '\n';
    cout << "Proposals: " << result.stats.proposals << '\n';
    cout << "Time: " << fixed << setprecision(3) << seconds << " seconds\n";
}

void printUsage(const char* program) {
    cout << "Usage:\n";
    cout << "  " << program
         << " --students data/students.csv --courses data/courses.csv"
            " [--output allocations.csv]\n";
    cout << "  " << program << " --self-test\n";
    cout << "  " << program << " --benchmark [students] [courses]\n";
}

int main(int argc, char* argv[]) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    try {
        if (argc >= 2 && string(argv[1]) == "--self-test") {
            runSelfTest();
            return 0;
        }

        if (argc >= 2 && string(argv[1]) == "--benchmark") {
            int students = argc >= 3 ? stoi(argv[2]) : 50000;
            int courses = argc >= 4 ? stoi(argv[3]) : 500;
            runBenchmark(students, courses);
            return 0;
        }

        string studentFile, courseFile, outputFile = "allocations.csv";

        for (int i = 1; i < argc; i++) {
            string argument = argv[i];
            if (argument == "--students" && i + 1 < argc) {
                studentFile = argv[++i];
            } else if (argument == "--courses" && i + 1 < argc) {
                courseFile = argv[++i];
            } else if (argument == "--output" && i + 1 < argc) {
                outputFile = argv[++i];
            } else if (argument == "--help" || argument == "-h") {
                printUsage(argv[0]);
                return 0;
            } else {
                throw runtime_error("Invalid command-line arguments");
            }
        }

        if (studentFile.empty() || courseFile.empty()) {
            printUsage(argv[0]);
            return 1;
        }

        vector<Student> students = readStudents(studentFile);
        vector<Course> courses = readCourses(courseFile);
        Result result = allocateCourses(students, courses);

        writeOutput(outputFile, students, result);
        printSummary(students, courses, result);
        cout << "\nOutput written to " << outputFile << '\n';
    } catch (const exception& error) {
        cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
