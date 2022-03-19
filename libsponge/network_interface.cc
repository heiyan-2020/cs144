#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();

    if(_arp_table.count(next_hop_ip) > 0) {
        EthernetFrame _new_frame;
        _new_frame.header().src = _ethernet_address;
        _new_frame.header().dst = _arp_table[next_hop_ip].first;
        _new_frame.header().type = EthernetHeader::TYPE_IPv4;
        _new_frame.payload() = dgram.serialize();
        _frames_out.push(_new_frame);
    } else {
        if (_dgrams_cache.count(next_hop_ip) == 0 || _time_stamp - _dgrams_cache[next_hop_ip].second > 5000) {
            send_arpMessage(ETHERNET_BROADCAST, next_hop_ip, ARPMessage::OPCODE_REQUEST);
            _dgrams_cache[next_hop_ip].second = _time_stamp;
        }
        _dgrams_cache[next_hop_ip].first.first.push(dgram);
        _dgrams_cache[next_hop_ip].first.second = next_hop_ip;
    }
    return;
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    EthernetAddress dstAddr = frame.header().dst;
    if (dstAddr == _ethernet_address || dstAddr == ETHERNET_BROADCAST) {
        if (frame.header().type == EthernetHeader::TYPE_IPv4) {
            InternetDatagram rtnDgram;
            rtnDgram.parse(frame.payload());
            return rtnDgram;
        } else if (frame.header().type == EthernetHeader::TYPE_ARP) {
            ARPMessage arp_message;
            arp_message.parse(frame.payload());
            _arp_table[arp_message.sender_ip_address] = {arp_message.sender_ethernet_address , _time_stamp};

            if (arp_message.opcode == ARPMessage::OPCODE_REQUEST && arp_message.target_ip_address == _ip_address.ipv4_numeric()) {
                send_arpMessage(
                    arp_message.sender_ethernet_address, arp_message.sender_ip_address, ARPMessage::OPCODE_REPLY);
            } else if (arp_message.opcode == ARPMessage::OPCODE_REPLY) {
                match_and_resend(arp_message.sender_ip_address);
            }
        }
    } else {
        return {};
    }
    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) { 
    _time_stamp += ms_since_last_tick;
    auto itr = _arp_table.begin();
    queue<uint32_t> remove_queue;
    while (itr != _arp_table.end()) {
        if (_time_stamp - itr->second.second > 30000) {
            remove_queue.push(itr->first);
        }
        itr++;
    }
    while (remove_queue.size()) {
        _arp_table.erase(remove_queue.front());
        remove_queue.pop();
    }
}

void NetworkInterface::send_arpMessage(EthernetAddress dst_ethAddr, uint32_t dst_ipAddr, uint16_t opcode) { 
    EthernetFrame _new_frame;
    _new_frame.header().src = _ethernet_address;
    _new_frame.header().dst = dst_ethAddr;
    _new_frame.header().type = EthernetHeader::TYPE_ARP;
    ARPMessage request;
    request.opcode = opcode;
    request.sender_ip_address = _ip_address.ipv4_numeric();
    request.sender_ethernet_address = _ethernet_address;
    request.target_ip_address = dst_ipAddr;
    if (opcode == ARPMessage::OPCODE_REPLY) {
        request.target_ethernet_address = dst_ethAddr;
    }
    _new_frame.payload() = request.serialize();
    _frames_out.push(_new_frame);
}

void NetworkInterface::match_and_resend(uint32_t ip) {
    while (_dgrams_cache[ip].first.first.size()) {
        InternetDatagram dgram = _dgrams_cache[ip].first.first.front();
        EthernetFrame _new_frame;
        _new_frame.header().src = _ethernet_address;
        _new_frame.header().dst = _arp_table[ip].first;
        _new_frame.header().type = EthernetHeader::TYPE_IPv4;
        _new_frame.payload() = dgram.serialize();
        _frames_out.push(_new_frame);
        _dgrams_cache[ip].first.first.pop();
    }
    _dgrams_cache.erase(ip);
}