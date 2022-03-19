#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {
    first_unassembled = 0;
    make_heap(window.begin(), window.end(), greater<Slice>());
}

//! \\details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t str_len = data.length();
    // if (data[data.length() - 1] == 'X') {
    //     str_len--;
    // }
    if (eof)
    {
        _eof = eof;
        last_byte = index + str_len;
        if (str_len == 0) {
            _output.end_input();
            return;
        }
    }
    if (str_len == 0) {
        return;
    }
    priority_queue<uint64_t, vector<uint64_t>, greater<uint64_t>> q = StreamReassembler::distinguish(index, index + str_len);
    size_t len = q.size();
    for (size_t i = 0; i < len; i += 2)
    {
        uint64_t l = q.top();
        q.pop();
        uint64_t r = q.top();
        q.pop();
        if (r - l == 0) {
            continue;
        }
        Slice s = {l, r - l, data.substr(l - index, r - l)};
        if (r - l + _unassembled + _output.buffer_size() > _capacity) {
            s.len = _capacity - (_unassembled + _output.buffer_size());
            s.data = data.substr(l - index, s.len);
            window.push_back(s);
            push_heap(window.begin(), window.end(), greater<Slice>());
            _unassembled += s.len;
            break;
        }
        window.push_back(s);
        push_heap(window.begin(), window.end(), greater<Slice>());
        _unassembled += s.len;
    }
    while (!window.empty() && window[0].index == first_unassembled) {
        _output.write(window[0].data);
        first_unassembled += window[0].len;
        _unassembled -= window[0].len;
        if (_eof && last_byte == window[0].index + window[0].len) {
            _output.end_input();
        }
        pop_heap(window.begin(), window.end(), greater<Slice>());
        window.pop_back();
    }

}

priority_queue<uint64_t, vector<uint64_t>, greater<uint64_t>> StreamReassembler::distinguish(uint64_t begin, uint64_t end) {
    priority_queue<uint64_t, vector<uint64_t>, greater<uint64_t>> q;
    if (end < first_unassembled || begin == end)
    {
        return q;
    }
    if (begin < first_unassembled) {
        begin = first_unassembled;
    }
    for (auto var : window) {
        uint64_t lhs =  var.index, rhs =  var.index + var.len;
        if (lhs <= begin && rhs >= end) {
            return q;
        }
        else if ( lhs <= begin && rhs >= begin) {
            begin =  rhs;
        } else if ( lhs <= end &&  rhs >= end) {
            end =  lhs;
        } else if ( lhs > begin &&  rhs < end) {
            q.push(lhs);
            q.push(rhs);
        }
    }
    q.push(begin), q.push(end);
    return q;
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled; }

bool StreamReassembler::empty() const { return _unassembled == 0; }
