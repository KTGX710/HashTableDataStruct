#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iomanip>
#include <memory>

// Forward declarations
class Course;
class DataNode;
class CourseBuilder;
class DataStructure;
class LineParser;
class FileReader;
class GUI;
class Menu;

// Course class to store course information
class Course {
private:
    std::string courseName;
    std::string courseTitle;
    std::vector<std::string> coursePrerequisites;

public:
    // Constructor
    Course(const std::string& name, const std::string& title,
           const std::vector<std::string>& prereqs)
    : courseName(name), courseTitle(title), coursePrerequisites(prereqs) {}

    // Getters
    const std::string& getName() const { return courseName; }
    const std::string& getTitle() const { return courseTitle; }
    const std::vector<std::string>& getPrerequisites() const { return coursePrerequisites; }

    // toString method
    std::string toString() const {
        std::string prereqs = "None";

        if (!coursePrerequisites.empty()) {
            prereqs.clear();
            for (size_t i = 0; i < coursePrerequisites.size(); ++i) {
                prereqs += coursePrerequisites[i];
                if (i < coursePrerequisites.size() - 1) {
                    prereqs += ", ";
                }
            }
        }

        return courseName + ": " + courseTitle + "; Prerequisites: " + prereqs;
    }
};

// DataNode class to store Course object and ADT metadata
class DataNode {
public:
    std::unique_ptr<Course> course;
    std::unique_ptr<DataNode> nextNode;

    // Constructor
    DataNode(std::unique_ptr<Course> c) : course(std::move(c)), nextNode(nullptr) {}

    // Destructor - automatically managed by unique_ptr
};

// CourseBuilder class to validate input and build Course objects
class CourseBuilder {
public:
    // Validate that String follows schema: ABCD123
    static bool courseNameValidator(const std::string& courseName) {
        if (courseName.empty()) return false;

        if (courseName.length() == 7) {
            // Check first 4 characters are alphabetic
            for (size_t i = 0; i < 4; ++i) {
                if (!std::isalpha(courseName[i])) return false;
            }

            // Check last 3 characters are numeric
            for (size_t i = 4; i < 7; ++i) {
                if (!std::isdigit(courseName[i])) return false;
            }

            return true;
        }

        return false;
    }

    // Validates that String is not null, empty, or contains escape characters
    static bool courseDataValidator(const std::string& data) {
        if (data.empty()) return false;

        for (char c : data) {
            if (c == '\n' || c == '\r' || c == '\t') return false;
        }

        return true;
    }

    // Trim front and tail end whitespace
    static std::string trim(const std::string& input) {
        if (input.empty()) return input;

        size_t start = 0;
        size_t end = input.length() - 1;

        // Move start forward past leading whitespace
        while (start < input.length() && std::isspace(input[start])) {
            ++start;
        }

        // If entire string was whitespace
        if (start == input.length()) return "";

        // Move end backward past trailing whitespace
        while (end >= start && std::isspace(input[end])) {
            --end;
        }

        return input.substr(start, end - start + 1);
    }

    // Remove quotation marks and escape characters
    static std::string filter(const std::string& input) {
        if (input.empty()) return "";

        std::string output = "";
        for (char c : input) {
            if (c != '"' && c != '\'' && c != '\n' && c != '\r' && c != '\t') {
                output += c;
            }
        }

        return output;
    }

    // Build Course object using validator and constructor
    static std::unique_ptr<Course> builder(const std::vector<std::string>& input) {
        if (input.empty()) return nullptr;

        // Defensive copy
        std::vector<std::string> courseData;
        for (const auto& item : input) {
            courseData.push_back(trim(filter(item)));
        }

        // Need at least name and title
        if (courseData.size() < 2) return nullptr;

        // Validate Course Name
        if (!courseNameValidator(courseData[0])) return nullptr;

        // Validate Course Title
        if (!courseDataValidator(courseData[1])) return nullptr;

        // Establish prerequisites list
        std::vector<std::string> coursePrereq;

        // Check if prerequisite data exists in input
        if (courseData.size() > 2) {
            // Build prerequisites list
            for (size_t i = 2; i < courseData.size(); ++i) {
                const std::string& tempCourse = courseData[i];
                if (courseNameValidator(tempCourse)) {
                    coursePrereq.push_back(tempCourse);
                }
            }
        }

        return std::make_unique<Course>(courseData[0], courseData[1], coursePrereq);
    }
};

// Hash Table data structure to store Course nodes using chaining
class DataStructure {
private:
    std::vector<std::unique_ptr<DataNode>> buckets;
    size_t capacity;
    size_t size;
    mutable std::vector<Course*> sortedCourses;
    mutable bool sorted;
    static const double LOAD_FACTOR_THRESHOLD;

public:
    // Constructor: Initialize hash table with default capacity
    DataStructure() : capacity(1024), size(0), sorted(false) {
        buckets.resize(capacity);
    }

    // Overloaded Constructor: Allows custom capacity
    DataStructure(size_t cap) : capacity(cap > 16 ? cap : 16), size(0), sorted(false) {
        buckets.resize(capacity);
    }

    // Destructor: Clean up all allocated memory - automatically handled by unique_ptr
    ~DataStructure() = default;

    // Hash Function: Compute index for a course name using polynomial rolling hash
    size_t hash(const std::string& key) const {
        size_t hashValue = 0;
        const size_t base = 31;

        for (char c : key) {
            hashValue = (hashValue * base + static_cast<size_t>(c)) % capacity;
        }

        return hashValue;
    }

    // Resize: Expand the hash table when load factor exceeds threshold
    void resize() {
        size_t oldCapacity = capacity;
        std::vector<std::unique_ptr<DataNode>> oldBuckets = std::move(buckets);

        // Double the capacity
        capacity *= 2;

        // Create a new buckets array with doubled size
        buckets.resize(capacity);

        // Rehash all existing nodes into new buckets
        for (size_t index = 0; index < oldCapacity; ++index) {
            if (oldBuckets[index] != nullptr) {
                std::unique_ptr<DataNode> currentNode = std::move(oldBuckets[index]);

                while (currentNode != nullptr) {
                    std::unique_ptr<DataNode> nextNode = std::move(currentNode->nextNode);

                    // Compute new index using updated capacity
                    size_t newIndex = hash(currentNode->course->getName());

                    // Insert node at head of newBuckets[newIndex] (chaining)
                    currentNode->nextNode = std::move(buckets[newIndex]);
                    buckets[newIndex] = std::move(currentNode);

                    currentNode = std::move(nextNode);
                }
            }
        }
    }

    // Insert: Add a new course to the hash table (with duplicate check)
    void insert(std::unique_ptr<Course> course) {
        if (course == nullptr) {
            std::cout << "Unable to insert empty course!" << std::endl;
            return;
        }

        const std::string& key = course->getName();
        size_t index = hash(key);

        // Check for duplicate course in the chain
        std::unique_ptr<DataNode>& currentNode = buckets[index];

        while (currentNode != nullptr) {
            if (currentNode->course->getName() == key) {
                std::cout << "Duplicate course: " << key << std::endl;
                return;  // Exit insert
            }
            currentNode = std::move(currentNode->nextNode);
        }

        // Create a new DataNode to store the course
        auto newNode = std::make_unique<DataNode>(std::move(course));

        // Insert at head of chain
        newNode->nextNode = std::move(buckets[index]);
        buckets[index] = std::move(newNode);

        // Increment size (track number of courses)
        ++size;

        // Check if load factor exceeds threshold, resize if necessary
        if ((double)size / capacity > LOAD_FACTOR_THRESHOLD) {
            resize();
        }

        // Invalidate sorted cache
        sorted = false;
    }

    // Inject: Replace the entire hash table with a new one built from a list of courses
    void inject(const std::vector<std::unique_ptr<Course>>& newCourses) {
        if (newCourses.empty()) {
            std::cout << "Warning: Empty or null course list. No change made." << std::endl;
            return;
        }

        // Clear current hash table - this will automatically clean up all memory
        buckets.clear();
        buckets.resize(capacity);
        size = 0;

        // Insert each course from newCourses list into the new hash table
        for (const auto& course : newCourses) {
            if (course == nullptr) {
                std::cout << "Skipping null course." << std::endl;
                continue;
            }

            const std::string& key = course->getName();
            size_t index = hash(key);

            // Check for duplicate in the chain
            std::unique_ptr<DataNode>& currentNode = buckets[index];
            while (currentNode != nullptr) {
                if (currentNode->course->getName() == key) {
                    std::cout << "Duplicate course: " << key << " ; skipping" << std::endl;
                    continue;  // Skip insertion
                }
                currentNode = std::move(currentNode->nextNode);
            }

            // Create new node - properly move the course ownership
            auto newNode = std::make_unique<DataNode>(std::move(course));

            // Insert at head of chain
            newNode->nextNode = std::move(buckets[index]);
            buckets[index] = std::move(newNode);
        }

        // Invalidate sorted cache
        sorted = false;
    }

    // Remove: Delete a course by courseName
    void remove(const std::string& courseName) {
        if (courseName.empty()) {
            std::cout << "Unable to remove empty course!" << std::endl;
            return;
        }

        size_t index = hash(courseName);

        // Traverse chain to find the node
        std::unique_ptr<DataNode>& currentNode = buckets[index];
        std::unique_ptr<DataNode>* previous = nullptr;  // Track previous node for deletion

        // Search through chain until node found or end reached
        while (currentNode != nullptr) {
            // Check if current node matches course name
            if (currentNode->course->getName() == courseName) {
                // Handle deletion based on position (head vs. middle)
                if (previous == nullptr) {
                    // Node is at head of chain: update bucket head
                    buckets[index] = std::move(currentNode->nextNode);
                } else {
                    // Node is in middle: bypass it by linking previous to next
                    (*previous)->nextNode = std::move(currentNode->nextNode);
                }

                // Decrement size
                --size;

                // Invalidate sorted cache
                sorted = false;

                return;
            }

            // Move forward in chain
            previous = &currentNode;
            currentNode = std::move(currentNode->nextNode);
        }

        // If loop ends, course not found
        std::cout << "Course not found: " << courseName << std::endl;
    }

    // Return sorted course list - only sorts when needed
    const std::vector<Course*>& getSorted() const {
        if (!sorted) {
            sort();
        }
        return sortedCourses;
    }

    // Sort: Extract all courses, and sort by name; should be called whenever table is updated
    void sort() const {
        // Clear old list
        sortedCourses.clear();

        // Traverse all buckets and collect courses
        for (size_t index = 0; index < capacity; ++index) {
            DataNode* currentNode = buckets[index].get();
            while (currentNode != nullptr) {
                sortedCourses.push_back(currentNode->course.get());
                currentNode = currentNode->nextNode.get();
            }
        }

        // Sort list using std::sort
        std::sort(sortedCourses.begin(), sortedCourses.end(),
                  [](const Course* a, const Course* b) {
                      return a->getName() < b->getName();
                  });

        // Mark cache as valid
        sorted = true;
    }

    // Get a course by name (search)
    std::unique_ptr<Course> get(const std::string& courseName) const {
        if (courseName.empty()) return nullptr;

        size_t index = hash(courseName);

        // Traverse chain to find course
        DataNode* currentNode = buckets[index].get();

        while (currentNode != nullptr) {
            // Compare course names
            if (currentNode->course->getName() == courseName) {
                // Return the course object
                return std::make_unique<Course>(*currentNode->course);
            }
            currentNode = currentNode->nextNode.get();
        }

        // Not found ; return null
        return nullptr;
    }

    // DEBUG: Print all buckets for debugging
    void printAllBuckets() const {
        for (size_t index = 0; index < capacity; ++index) {
            std::cout << "Bucket " << index << ": ";
            DataNode* currentNode = buckets[index].get();
            while (currentNode != nullptr) {
                std::cout << currentNode->course->toString() << std::endl;
                currentNode = currentNode->nextNode.get();
            }
        }
    }

private:
    void destroy() {
        buckets.clear();
        size = 0;
        sortedCourses.clear();
        sorted = false;
    }
};

const double DataStructure::LOAD_FACTOR_THRESHOLD = 0.75;

// LineParser class to parse lines from String input into CourseBuilder
class LineParser {
public:
    // Splits unparsed line into list of Strings by delimiter, filtering quotation marks
    static std::vector<std::string> split(const std::string& input, const std::string& delimiter = ",") {
        if (input.empty()) return {};

        std::vector<std::string> result;
        size_t start = 0;
        size_t length = input.length();

        // Loop through input and extract fields
        while (start < length) {
            size_t index = start;

            // Find the next delimiter, but be careful about quoted strings
            bool inQuotes = false;
            char quoteChar = '\0';

            while (index < length) {
                char c = input[index];

                if (c == '"' || c == '\'') {
                    if (!inQuotes) {
                        inQuotes = true;
                        quoteChar = c;
                    } else if (c == quoteChar) {
                        inQuotes = false;
                        quoteChar = '\0';
                    }
                } else if (c == delimiter[0] && !inQuotes) {
                    break;
                }

                ++index;
            }

            // Extract substring from start to index and append to result list
            std::string substring = input.substr(start, index - start);

            // Filter out quotation marks from the substring
            std::string filteredSubstring = "";
            for (char c : substring) {
                if (c != '"' && c != '\'') {  // Remove both double and single quotes
                    filteredSubstring += c;
                }
            }

            result.push_back(filteredSubstring);

            // Move start to next after delimiter
            start = index + 1;

            // Check for and skip consecutive delimiter
            while (start < length && input[start] == delimiter[0]) {
                ++start;
            }
        }

        return result;
    }

    // Parses line from file into Course object and returns Course Object
    static std::unique_ptr<Course> parse(const std::string& input, const std::string& delimiter = ",", int lineNumber = 0) {
        // Get each string separated by delimiter
        std::vector<std::string> parts = split(input, delimiter);

        if (parts.size() < 2) {
            std::cout << "Invalid line format at line: " << lineNumber << std::endl;
            return nullptr;
        }

        // Pass to CourseBuilder
        auto course = CourseBuilder::builder(parts);

        if (course == nullptr) {
            std::cout << "Failed to build course at line: " << lineNumber << std::endl;
            return nullptr;
        }

        return course;
    }
};

// FileReader class to read course data from a file and load it into the DataStructure
class FileReader {
public:
    // Reads file line by line and delegates parsing
    static void readFile(DataStructure& dataStruct, const std::string& fileName) {
        if (fileName.empty()) {
            std::cout << "Invalid file name" << std::endl;
            return;
        }

        std::ifstream file(fileName);
        if (!file.is_open()) {
            std::cout << "Failed to open file: " << fileName << std::endl;
            return;
        }

        // Initialize parser
        LineParser parser;

        // Initialize temp list to store Course objects
        std::vector<std::unique_ptr<Course>> newCourses;

        // Track line numbers for diagnostics
        int lineNumber = 0;

        // Read file line by line
        std::string line;
        while (std::getline(file, line)) {
            ++lineNumber;

            // Skip null or empty lines
            if (line.empty()) continue;

            // Extract course from line, and set delimiter to comma assuming CSV
            auto course = parser.parse(line, ",", lineNumber);

            if (course != nullptr) {
                newCourses.push_back(std::move(course));
            }
        }

        // Close file to release resources
        file.close();

        // Build Data Structure
        dataStruct.inject(newCourses);

        std::cout << "Successfully read file: " << fileName << std::endl;
    }
};

// GUI class to encapsulate static menu displays
class GUI {
public:
    static void printMenu() {
        std::cout << "\n==================================" << std::endl;
        std::cout << "     Welcome to ABC University    " << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "Please select a menu option:" << std::endl;
        std::cout << "1) Load data to application" << std::endl;
        std::cout << "2) Display CS courses (alphanumeric)" << std::endl;
        std::cout << "3) Search for individual course" << std::endl;
        std::cout << "9) Quit application" << std::endl;
        std::cout << "----------------------------------" << std::endl;
        std::cout << "Enter your choice: ";
    }

    static void printSearchMenu() {
        std::cout << "Search Categories:" << std::endl;
        std::cout << "1) Course Name" << std::endl;
        std::cout << "2) Course Title" << std::endl;
        std::cout << "3) Prerequisite" << std::endl;
        std::cout << "Enter selection: ";
    }

    static void promptSearchCriteria() {
        std::cout << "Enter search text: ";
    }

    static void printNoResults() {
        std::cout << "No matching courses found." << std::endl;
    }

    static void printCourse(const Course* course) {
        if (course == nullptr) {
            std::cout << "Course does not exist" << std::endl;
            return;
        }
        std::cout << course->toString() << std::endl;
    }

    static void printCourseListHeader() {
        std::cout << "-------- Course List --------" << std::endl;
    }

    static void printGoodbye() {
        std::cout << "Exiting application..." << std::endl;
    }

    static void clearScreen() {
        // Clear console screen using ANSI escape codes
        std::cout << "\033[2J\033[1;1H";
    }

    static void waitForInput() {
        std::cout << "Press Enter to continue...";
        std::cin.ignore();
        std::cin.get();
    }

    // Prompt for file name
    static std::string promptFileName() {
        std::cout << "Enter the name of the course data file: ";
        std::string fileName;
        std::getline(std::cin, fileName);
        return fileName;
    }
};

// Menu class to encapsulate menu option functionality
class Menu {
public:
    // Load data from file to memory
    static void load(DataStructure& dataStruct) {
        GUI::clearScreen();
        std::cout << "Loading data..." << std::endl;

        // Prompt user for file name
        std::string fileName = GUI::promptFileName();

        // Get a String list of the lines from the input file
        FileReader::readFile(dataStruct, fileName);
    }

    // Search for Course object from input criteria
    static std::vector<Course*> search(const DataStructure& dataStruct,
                                       const std::string& criteria,
                                       const std::string& category) {
        // Initialize list to return
        std::vector<Course*> results;

        // Iterate through each index in the Data Structure
        for (size_t index = 0; index < dataStruct.getSorted().size(); ++index) {
            const Course* course = dataStruct.getSorted()[index];

            // Check for matching course by given category
            if (category == "name") {
                if (course->getName() == criteria) {
                    results.push_back(const_cast<Course*>(course));
                }
            } else if (category == "title") {
                if (course->getTitle().find(criteria) != std::string::npos) {
                    results.push_back(const_cast<Course*>(course));
                }
            } else if (category == "prereq") {
                const auto& prereqs = course->getPrerequisites();
                for (const auto& prereq : prereqs) {
                    if (prereq == criteria) {
                        results.push_back(const_cast<Course*>(course));
                        break;
                    }
                }
            }
        }

        return results;
                                       }

                                       // Display all CS courses in alphanumeric order
                                       static void displayCSCourses(const DataStructure& dataStruct) {
                                           GUI::printCourseListHeader();

                                           // Get sorted courses
                                           const auto& sortedCourses = dataStruct.getSorted();

                                           // Filter for Computer Science courses (first 2 characters are "CS")
                                           for (const Course* course : sortedCourses) {
                                               if (course->getName().substr(0, 2) == "CS") {
                                                   GUI::printCourse(course);
                                               }
                                           }
                                       }

                                       // Display all courses in alphanumeric order
                                       static void displayAllCourses(const DataStructure& dataStruct) {
                                           GUI::printCourseListHeader();

                                           const auto& sortedCourses = dataStruct.getSorted();

                                           for (const Course* course : sortedCourses) {
                                               GUI::printCourse(course);
                                           }
                                       }

                                       // Display list of courses
                                       static void displayList(const std::vector<Course*>& courses) {
                                           GUI::printCourseListHeader();

                                           for (const Course* course : courses) {
                                               GUI::printCourse(course);
                                           }
                                       }

                                       // Search individual course functionality
                                       static void searchIndividualCourse(DataStructure& dataStruct) {
                                           GUI::printSearchMenu();

                                           int choice;
                                           std::cin >> choice;
                                           std::cin.ignore();  // Clear the newline character

                                           // Validate choice - only one option allowed
                                           if (choice < 1 || choice > 3) {
                                               std::cout << "Invalid selection" << std::endl;
                                               return;
                                           }

                                           std::string category;
                                           switch (choice) {
                                               case 1: category = "name"; break;
                                               case 2: category = "title"; break;
                                               case 3: category = "prereq"; break;
                                           }

                                           GUI::promptSearchCriteria();
                                           std::string criteria;
                                           std::getline(std::cin, criteria);

                                           // Validate search criteria
                                           if (criteria.empty()) {
                                               std::cout << "Search criteria cannot be empty." << std::endl;
                                               return;
                                           }

                                           auto results = search(dataStruct, criteria, category);

                                           if (results.empty()) {
                                               GUI::printNoResults();
                                           } else {
                                               displayList(results);
                                           }
                                       }
};

// Main function
int main() {
    DataStructure courseList;
    bool dataLoaded = false;  // Track whether data has been loaded

    int choice;
    do {
        GUI::printMenu();
        std::cin >> choice;
        std::cin.ignore();  // Clear the newline character

        switch (choice) {
            case 1:
                Menu::load(courseList);
                dataLoaded = true;  // Mark that data has been loaded
                GUI::waitForInput();
                GUI::clearScreen();
                break;

            case 2:
                if (!dataLoaded) {
                    GUI::clearScreen();
                    std::cout << "Please load data first before displaying courses." << std::endl;
                    GUI::waitForInput();
                    GUI::clearScreen();
                    break;
                }
                GUI::clearScreen();
                Menu::displayCSCourses(courseList);
                GUI::waitForInput();
                GUI::clearScreen();
                break;

            case 3:
                if (!dataLoaded) {
                    GUI::clearScreen();
                    std::cout << "Please load data first before searching courses." << std::endl;
                    GUI::waitForInput();
                    GUI::clearScreen();
                    break;
                }
                GUI::clearScreen();
                Menu::searchIndividualCourse(courseList);
                GUI::waitForInput();
                GUI::clearScreen();
                break;

            case 9:
                GUI::clearScreen();
                GUI::printGoodbye();
                GUI::waitForInput();
                return 0;

            default:
                GUI::clearScreen();
                GUI::printMenu();
                std::cout << "Invalid menu option. Please try again." << std::endl;
        }
    } while (choice != 9);

    return 0;
}
