#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <iostream>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that es the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    ,_rto(retx_timeout)
    , _stream(capacity)
    , _window_size(1) {}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _last_ackno; }

void TCPSender::fill_window() {
    while (_window_size) {
        bool syn = false, fin = false;
        if (!_next_seqno && _window_size >= 1) {
            syn = true, _window_size--;
        }
        if (!fin_sent && _stream.input_ended() && _window_size >= 1) {
            fin = true, _window_size--;
            fin_sent = true;
        }
        if (_stream.buffer_empty() && !syn && !fin) {
            return;
        }
        size_t occupied_limit = min(_window_size, TCPConfig::MAX_PAYLOAD_SIZE);
        occupied_limit = min(occupied_limit, _stream.buffer_size());
        if (fin && occupied_limit < _stream.buffer_size()) {
            fin = false, fin_sent = false;
            occupied_limit++, _window_size++;
        }
        string payload = _stream.peek_output(occupied_limit);
        _stream.pop_output(occupied_limit);
        _window_size -= occupied_limit;
        send_seg(payload, syn, fin);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    //remove any segements in outstanding segs whose absolute seqno < ackno.
    uint64_t seqno = unwrap(ackno, _isn, _next_seqno);
    if (seqno > _next_seqno) {
        //impossible seqno is ignored!
        return;
    }
    while (!_cache.empty() && _cache.top().abs_seqno < seqno) {
        _cache.pop();
        _rto = _initial_retransmission_timeout;
        timer.start_timer(_alive_time, _rto);
        _consecutive_restransmissions = 0;
    }
    if (seqno == _next_seqno) {
        _last_ackno = seqno;
    }
    if (_cache.empty() && timer.running()) {
        timer.stop_timer();
    }
    // fill the window
    if (!window_size) {
        _window_size = 1;
        smell = true;
    } else {
        if (window_size < _next_seqno - seqno) {
            _window_size = 0;
        } else {
            _window_size = window_size - (_next_seqno - seqno);
        }
        smell = false;
    }
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    _alive_time += ms_since_last_tick;
    if (timer.check(_alive_time)) {
        retrans();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_restransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
     seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}

void TCPSender::send_seg(const string& data, const bool syn, const bool fin) {
    TCPHeader header;
    TCPSegment seg;
    header.syn = syn;
    header.fin = fin;
    header.seqno = wrap(_next_seqno, _isn);
    seg.parse(header.serialize() + data);
    if (seg.length_in_sequence_space()) {
        _cache.push({_next_seqno, header.serialize() + data});
        //start timer if not running.
        if (!timer.running()) {
            timer.start_timer(_alive_time, _rto);
        }
    }
    _segments_out.push(seg);
    _next_seqno += seg.length_in_sequence_space();
}

void TCPSender::retrans() {
    Buffer seg_string = _cache.top().seg;
    TCPSegment seg;
    seg.parse(seg_string);
    _segments_out.push(seg);
    if (!smell) {
        _consecutive_restransmissions++;
        _rto *= 2;
    }
    timer.start_timer(_alive_time, _rto);
}
