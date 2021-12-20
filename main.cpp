#include <iostream>
#include <fstream>
#include <tuple>
#include "CSVParser.h"

// SFINAE
template<typename Ch, typename Tr, size_t position, typename ...Args>
void ostream_tuple_output_helper(
        std::basic_ostream<Ch, Tr> &os,
        std::tuple<Args...> const &t,
        std::enable_if_t<(position == sizeof...(Args)), void> * = nullptr
) {
    // end of the recursion
}

template<typename Ch, typename Tr, size_t position, typename ...Args>
void ostream_tuple_output_helper(
        std::basic_ostream<Ch, Tr> &os,
        std::tuple<Args...> const &t,
        std::enable_if_t<(position < sizeof...(Args)), void> * = nullptr
) {
    if (position > 0 && position < sizeof ...(Args)) {
        os << ", ";
    }
    os << std::get<position>(t);
    ostream_tuple_output_helper<Ch, Tr, position + 1, Args...>(os, t);
}

template<typename Ch, typename Tr, typename ...Args>
std::basic_ostream<Ch, Tr> &operator<<(std::basic_ostream<Ch, Tr> &os, std::tuple<Args...> const &t) {
    os << "(";
    ostream_tuple_output_helper<Ch, Tr, 0, Args...>(os, t);
    os << ")";
    return os;
}

void subtask1() {
    std::tuple<int, double, char, std::string> tuple1(1, 1.0, 'j', "hello world");
    std::tuple<> tuple2;
    std::tuple<int, int, int, int, int> tuple3(1, 2, 3, 4, 5);
    std::tuple<int, double, char, std::string, char> tuple4(1, 1.0, 'j', "hello world", 'b');

    std::cout << tuple1 << std::endl;
    std::cout << tuple2 << std::endl;
    std::cout << tuple3 << std::endl;
    std::cout << tuple4 << std::endl;
}

void subtask23() {
    std::ifstream file("test.csv");
    CSVParser<float, std::string, double> parser(file, 1, '\n', ',', '"');
    for (auto rs : parser) {
        std::cout << rs << std::endl;
    }

    std::ifstream file2("test-2.csv");
    CSVParser<float, std::string, double> parser2(file2, 1, '|', ':', '\'');
    for (auto rs : parser2) {
        std::cout << rs << std::endl;
    }
}

int main(const int argc, const char *argv[]) {
    std::cout << "Sub-task 1" << std::endl;
    subtask1();
    std::cout << "Sub-tasks 2, 3" << std::endl;
    subtask23();
}
