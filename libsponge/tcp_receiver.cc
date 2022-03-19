#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader header = seg.header();
    uint64_t amount = seg.length_in_sequence_space();
    WrappingInt32 seqno = WrappingInt32(header.seqno.raw_value());
    bool old_eof = _eof;
    if (header.syn) {
        has_syn = true;
        SYN = WrappingInt32(seqno.raw_value());
        seqno = seqno + 1;
        amount -= 1;
    }
    if (has_syn && header.fin) {
        _eof = true;
        amount -= 1;
    }
    if (has_syn && (amount > 0 || _eof)) {
        string data = seg.payload().copy();
        uint64_t stream_no = unwrap(seqno, SYN, checkpoint) - 1;
        if (_eof && amount == 0 && stream_no > 0) {
            stream_no -= 1;
        }
        // cerr << "data = " << data << endl;
        // cerr << "first_un = " << _reassembler.first_unassembled << endl;
        // cerr << "buffer.size() = " << stream_out().buffer_size() << endl;
        // cerr << "window_size = " << _capacity - stream_out().buffer_size() << endl;
        // cerr << "stream_no = " << stream_no << endl << "--------------" << endl;
        if (stream_no >= _reassembler.first_unassembled + _capacity - stream_out().buffer_size()) {
            _eof = old_eof;
            return;
        }
        //如果超出window大小，需要提前截断
        if (stream_no + amount > _reassembler.first_unassembled + _capacity - stream_out().buffer_size()) {
            data = data.substr(0, _reassembler.first_unassembled + _capacity  - stream_out().buffer_size() - stream_no);
        }
        _reassembler.push_substring(data, stream_no, header.fin);
        checkpoint = max(stream_no + amount, checkpoint);
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!has_syn) {
        return optional<WrappingInt32>();
    } else {
        uint64_t abs_seqno = _reassembler.first_unassembled + 1;
        if (_eof && _reassembler.first_unassembled == checkpoint) {
            abs_seqno += 1;
        }
        return wrap(abs_seqno, SYN);
    }
}

size_t TCPReceiver::window_size() const {
    return _capacity - stream_out().buffer_size();
}
