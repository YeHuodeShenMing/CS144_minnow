#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  // Your code here.
  AddressNumeric next_hop_32_ = next_hop.ipv4_numeric();

  EthernetFrame frame_;
  if ( create_frame_IPv4( dgram, next_hop_32_, frame_ ) ) {
    transmit( frame_ );
    return;
  }

  if ( ( curr_time - recent_arp_requests_[next_hop_32_] ) > RESEND_DELAY
       || ( !recent_arp_requests_[next_hop_32_] && !curr_time ) ) {
    send_arp_request( next_hop_32_ );
    recent_arp_requests_[next_hop_32_] = curr_time;
  }

  waiting_datagrams_[next_hop_32_] = dgram;
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Your code here.
  if ( frame.header.dst != ethernet_address_ || frame.header.dst != ETHERNET_BROADCAST ) {
    return;
  }

  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram dgram;
    if ( parse( dgram, frame.payload ) ) {
      datagrams_received_.push( dgram );
    }
  }

  if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arp_recv;
    if ( parse( arp_recv, frame.payload ) ) {
      ARP_Cache[arp_recv.sender_ip_address] = arp_recv.sender_ethernet_address;

      recent_arp_requests_[arp_recv.sender_ip_address] = curr_time;

      if ( reply_arp_request( arp_recv ) ) {
        return;
      }

      if ( create_frame_ARP( arp_recv ) ) {
        return;
      }
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  curr_time += ms_since_last_tick;
  for ( auto it = recent_arp_requests_.begin(); it != recent_arp_requests_.end(); ) {
    // ARP映射记录时间超过30秒删除
    if ( it->second + 30 * 1000 <= curr_time ) {
      ARP_Cache.erase( it->first );
      it = recent_arp_requests_.erase( it );
      // currentTime = 0;
    } else {
      ++it;
    }
  }
}

// IPv4 创建 Ethernet frame
bool NetworkInterface::create_frame_IPv4( const InternetDatagram& dgram,
                                          AddressNumeric next_hop,
                                          EthernetFrame& frame )
{
  auto it = ARP_Cache.find( next_hop );

  if ( it != ARP_Cache.end() ) {
    // 构建以太网帧
    frame.header.type = EthernetHeader::TYPE_IPv4;
    frame.header.dst = it->second;
    frame.header.src = ethernet_address_;

    // 整个IP数据报作为以太层的PayLoad
    frame.payload = serialize( dgram );
    return true;
  }
  return false;
}

// ARP 创建 Ethernet frame
bool NetworkInterface::create_frame_ARP( const ARPMessage& arp_msg )
{
  if ( arp_msg.opcode == ARPMessage::OPCODE_REPLY && arp_msg.target_ip_address == ip_address_.ipv4_numeric() ) {
    EthernetFrame frame_;

    // 判断waiting里面有没有这个
    auto it = waiting_datagrams_.find( arp_msg.sender_ip_address );
    if ( it != waiting_datagrams_.end() ) {
      frame_.header.type = EthernetHeader::TYPE_IPv4;
      frame_.header.src = ethernet_address_;
      frame_.header.dst = arp_msg.sender_ethernet_address;
      frame_.payload = serialize( it->second );

      transmit( frame_ );
      waiting_datagrams_.erase( it );
    }
    return true;
  }
  return false;
}

// ARP请求分组获得目标host的MAC地址
void NetworkInterface::send_arp_request( AddressNumeric next_hop_ip )
{
  // 创建ARP请求
  ARPMessage arp_request;
  arp_request.opcode = ARPMessage::OPCODE_REQUEST;
  arp_request.sender_ethernet_address = ethernet_address_;
  arp_request.sender_ip_address = ip_address_.ipv4_numeric();

  arp_request.target_ethernet_address = { 0, 0, 0, 0, 0, 0 }; // 广播地址
  arp_request.target_ip_address = next_hop_ip;

  // 开始用arp构建以太网帧
  EthernetFrame frame_;
  frame_.header.dst = ETHERNET_BROADCAST;
  frame_.header.type = EthernetHeader::TYPE_ARP;
  frame_.header.src = ethernet_address_;

  // 序列化ARP请求
  frame_.payload = serialize( arp_request );

  transmit( frame_ );
}

// 判断ARP报文是否为 请求 报文， 并 响应
bool NetworkInterface::reply_arp_request( const ARPMessage& arp_msg )
{
  if ( arp_msg.opcode == ARPMessage::OPCODE_REQUEST && arp_msg.target_ip_address == ip_address_.ipv4_numeric() ) {
    ARPMessage arp_reply;
    arp_reply.opcode = ARPMessage::OPCODE_REPLY;
    arp_reply.sender_ethernet_address = ethernet_address_;
    arp_reply.sender_ip_address = ip_address_.ipv4_numeric();
    arp_reply.target_ip_address = arp_msg.sender_ip_address;
    arp_reply.target_ethernet_address = arp_msg.sender_ethernet_address;

    EthernetFrame reply_frame_;
    reply_frame_.header.type = EthernetHeader::TYPE_ARP;
    reply_frame_.header.dst = arp_msg.sender_ethernet_address;
    reply_frame_.header.src = ethernet_address_;

    reply_frame_.payload = serialize( arp_reply );
    transmit( reply_frame_ );
    return true;
  }
  return false;
}