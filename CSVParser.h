#ifndef NSU_OOP_2021_LABS_CSVPARSER_H
#define NSU_OOP_2021_LABS_CSVPARSER_H

#include <string>
#include <tuple>
#include <sstream>
#include <exception>
#include <vector>

template<typename T>
inline bool convertDataElement(std::string extractedString, T &t) {
    std::stringstream stringstream(extractedString);
    stringstream >> t;
    if (stringstream.fail() || stringstream.bad()) {
        return false;
    }
    return true;
}

template<>
inline bool convertDataElement(std::string extractedString, std::string &string) {
    string = extractedString;
    return true;
}

template<typename ...Args>
class CSVParserIterator {
public:
    typedef  std::tuple<Args...> MyTupleType;
protected:
    std::istream *istream;
    unsigned currentLineNumber;
    char lineDelimiter;
    char columnDelimiter;
    char escapeSymbol;
    MyTupleType currentRow;

    template<std::size_t position>
    void convertData(
            unsigned lineNumber,
            std::vector<std::string> data,
            MyTupleType &myTuple,
            std::enable_if_t<(position == std::tuple_size<MyTupleType>::value), void> * = nullptr
    ) {
        // nothing
    }

    template<std::size_t position, typename MyTupleType>
    void convertData(
            unsigned lineNumber,
            std::vector<std::string> data,
            MyTupleType &myTuple,
            std::enable_if_t<(position < std::tuple_size<MyTupleType>::value), void> * = nullptr
    ) {
        std::string extractedString = data[position];
        using Type = std::tuple_element_t<position, MyTupleType>;
        Type value;
        if (!convertDataElement(extractedString, value)) {
            throw std::runtime_error(
                    std::string()
                    + "Failed to convert data on the line "
                    + std::to_string(lineNumber)
                    + " and column (of the CSV table) "
                    + std::to_string(position + 1)
                    + ", failed to convert extracted string "
                    + "'" + extractedString + "'"
                    + " to the defined type."
            );
        }
        std::get<position>(myTuple) = value;
        this->convertData<position + 1>(lineNumber, data, myTuple);
    }

public:
    CSVParserIterator(
            std::istream *istream,
            unsigned initialLineNumber,
            char lineDelimiter,
            char columnDelimiter,
            char escapeSymbol
    ) {
        // this must match the constructor for the iterator in the end position
        this->istream = istream->good() ? istream : nullptr;
        this->currentLineNumber = istream->good() ? initialLineNumber : -1;
        this->lineDelimiter = lineDelimiter;
        this->columnDelimiter = columnDelimiter;
        this->escapeSymbol = escapeSymbol;

        // read first
        ++(*this);
    }

    CSVParserIterator(const CSVParserIterator &other) {
        this->istream = other.istream;
        this->currentLineNumber = other.currentLineNumber;
        this->lineDelimiter = other.lineDelimiter;
        this->columnDelimiter = other.columnDelimiter;
        this->escapeSymbol = other.escapeSymbol;
    }

    CSVParserIterator() {
        istream = nullptr;
        this->currentLineNumber = -1;
    }

    CSVParserIterator &operator++() {
        if (this->istream) {
            // stream is ended, must match the empty constructor initialization values
            if (this->istream->eof()) {
                this->istream = nullptr;
                this->currentLineNumber = -1;
                return *this;
            }

            std::string line;
            std::getline(*this->istream, line, this->lineDelimiter);

            // extracting
            std::vector<std::string> stringData;

            // analyzing symbol by symbol
            const int STATE_OUTSIDE_ESCAPE = 1;
            const int STATE_INSIDE_ESCAPE = 2;
            int state = STATE_OUTSIDE_ESCAPE;

            // as with the line number we start counting from 1
            int col;
            std::string value;
            char currentSymbol;
            for (col = 1; col <= line.size(); ++col) {
                currentSymbol = line[col - 1];
                switch (state) {
                    case STATE_OUTSIDE_ESCAPE: {
                        if (currentSymbol == this->columnDelimiter) {
                            stringData.push_back(value);
                            value.clear();
                        } else if (currentSymbol == this->escapeSymbol) {
                            state = STATE_INSIDE_ESCAPE;
                        } else {
                            value += currentSymbol;
                        }
                        break;
                    }
                    case STATE_INSIDE_ESCAPE: {
                        if (currentSymbol == this->escapeSymbol) {
                            state = STATE_OUTSIDE_ESCAPE;
                        } else {
                            value += currentSymbol;
                        }
                        break;
                    }
                    default:
                        throw std::runtime_error("Unreachable code");
                }
            }

            if (state == STATE_INSIDE_ESCAPE) {
                throw std::runtime_error(
                        std::string()
                        + "Error parsing at line "
                        + std::to_string(this->currentLineNumber)
                        + " and column "
                        + std::to_string(col)
                        + ". Line interrupted inside escaped section"
                );
            }

            if (!value.empty()) {
                stringData.push_back(value);
                value.clear();
            } else if (currentSymbol == this->columnDelimiter) {
                // 1,2,3, <- the very last value is empty but is present
                stringData.push_back(value);
            }

            if (stringData.size() != sizeof...(Args)) {
                throw std::runtime_error(
                        std::string()
                        + "Error parsing at line "
                        + std::to_string(this->currentLineNumber)
                        + " and column "
                        + std::to_string(col)
                        + ". Expected to find "
                        + std::to_string(sizeof...(Args))
                        + " entries in the line but extracted "
                        + std::to_string(stringData.size()) + ".");
            }

            this->convertData<0>(
                    this->currentLineNumber,
                    stringData,
                    this->currentRow
            );

            this->currentLineNumber++;
        }

        return *this;
    }

    CSVParserIterator operator++(int) {
        CSVParserIterator tmp(*this);
        ++(*this);
        return tmp;
    }

    std::tuple<Args...> const &operator*() const {
        return this->currentRow;
    }

    std::tuple<Args...> const *operator->() const {
        return &this->currentRow;
    }

    bool operator==(CSVParserIterator const &other) {
        return this->istream == other.istream && this->currentLineNumber == other.currentLineNumber;
    }

    bool operator!=(CSVParserIterator const &other) {
        return !(*this == other);
    }
};

template<typename ...Args>
class CSVParser {
private:
    std::istream *istream;
    char lineDelimiter;
    char columnDelimiter;
    char escapeSymbol;
    // from what line we are starting to count counting starts from 1 and can be altered by
    // linesToSkip parameter (for example, the header)
    unsigned startLine;
public:
    typedef CSVParserIterator<Args...> Iterator;

    CSVParser(
            std::istream &istream,
            int linesToSkip = 0,
            char lineDelimiter = '\n',
            char columnDelimiter = ',',
            char escapeSymbol = '"'
    ) {
        this->istream = &istream;
        this->lineDelimiter = lineDelimiter;
        this->columnDelimiter = columnDelimiter;
        this->escapeSymbol = escapeSymbol;

        if (this->istream->bad() || this->istream->fail()) {
            throw std::runtime_error("Impossible to read from input stream");
        }

        this->startLine = 1;
        for (int i = 0; i < linesToSkip; ++i) {
            // we don't want to go beyond the end of file
            if (this->istream->peek() == EOF) {
                break;
            }
            std::string string;
            std::getline(*this->istream, string, this->lineDelimiter);
            startLine++;
        }
    }

    Iterator begin() const {
        return Iterator(
                this->istream,
                this->startLine,
                this->lineDelimiter,
                this->columnDelimiter,
                this->escapeSymbol
        );
    }

    Iterator end() const {
        return Iterator();
    }
};


#endif //NSU_OOP_2021_LABS_CSVPARSER_H
