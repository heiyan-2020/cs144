#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time - _last_seg_time; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    _last_seg_time = _time;
    // cerr << "*****************************" << endl;
    // cerr << this << endl << seg.header().to_string() << endl;
    // cerr << "len = " <<seg.length_in_sequence_space() << endl;
    // cerr << "*****************************" << endl;
    if (seg.header().rst) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _alive = false;
    } else {
        _receiver.segment_received(seg);
        if (_receiver.stream_out().eof() && !_sender.stream_in().input_ended()) {
            _linger_after_streams_finish = false;
        }
        if (seg.header().ack) {
            WrappingInt32 ackno = seg.header().ackno;
            size_t win = seg.header().win;
            _sender.ack_received(ackno, win);
        }
        if (seg.length_in_sequence_space()) {
            _sender.fill_window();
            if (_sender.segments_out().empty()) {
                _sender.send_empty_segment();
            }
            trans();
        }
        if (_receiver.ackno().has_value() && !seg.length_in_sequence_space() && seg.header().seqno == _receiver.ackno().value() - 1) {
            _sender.send_empty_segment();
            trans();
        }
    }
}

bool TCPConnection::active() const {
    return _alive;
}

size_t TCPConnection::write(const string &data) {
    size_t rtn = _sender.stream_in().write(data);
    _sender.fill_window();
    return rtn;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _time += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    trans();
    if (_receiver.stream_out().input_ended() && _sender.stream_in().eof() && _sender.bytes_in_flight() == 0 &&
        _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2) {
        if (_linger_after_streams_finish && _time - _last_seg_time >= 10 * _cfg.rt_timeout) {
            _alive = false;
        } else if (!_linger_after_streams_finish) {
            _alive = false;
        }
    }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    if (_receiver.stream_out().input_ended()) {
        _linger_after_streams_finish = false;
    }
    trans();
}

void TCPConnection::connect() {
    _sender.fill_window();
    trans();

}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            TCPSegment seg;
            seg.header().rst = true;
            _segments_out.push(seg);
            _alive = false;
            _sender.stream_in().set_error();
            _receiver.stream_out().set_error();
            trans();
        }
    } catch (const exception &e) {
        // std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::trans() {
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        TCPSegment segg;
        segg.header().rst = true;
        _segments_out.push(segg);
        _alive = false;
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        return;
    }
    while (!_sender.segments_out().empty()) {
        TCPSegment seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
        }
        seg.header().win = _receiver.window_size();
        segments_out().push(seg);
        // cerr << "-----------------------------------------" << endl;
        // cerr << this << endl << seg.header().to_string() << endl;
        // cerr << "len = " << seg.payload().str().length() << endl << "-----------------------------------------" << endl;
    }
}
