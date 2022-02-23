#include "byte_stream.hh"
#include <deque>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) { this->_capacity = capacity; }

size_t ByteStream::write(const string &data) { 
    size_t count = 0;
    while (buffer.size() < _capacity && count < data.size()) {
        buffer.push_back(data[count++]);
    }
    _end = false;
    bytesWrite += count;
    return count;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    const size_t valid_len = len <= buffer.size() ? len : buffer.size();
    string result;
    size_t count = 0;
    while (count < valid_len) {
        result += buffer[count++];
    }
    return result;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    const size_t valid_len = len <= buffer.size() ? len : buffer.size();
    size_t count = 0;
    while (count < valid_len) {
        buffer.pop_front();
        count++;
    }
    bytesRead += count;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    const size_t valid_len = len <= buffer.size() ? len : buffer.size();
    string result;
    size_t count = 0;
    while (count < valid_len) {
        result += buffer.front();
        buffer.pop_front();
        count++;
    }
    bytesRead += count;
    return result;
}

void ByteStream::end_input() { _end = true; }

bool ByteStream::input_ended() const { return _end; }

size_t ByteStream::buffer_size() const { return buffer.size(); }

bool ByteStream::buffer_empty() const { return buffer.empty(); }

bool ByteStream::eof() const { return _end && buffer.empty(); }

size_t ByteStream::bytes_written() const { return bytesWrite; }

size_t ByteStream::bytes_read() const { return bytesRead; }

size_t ByteStream::remaining_capacity() const { return _capacity - buffer.size(); }
