#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

// #include <iostream>
#include <utility>

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

  // Ethernet 地址已知
  if ( ARP_Cache.count( next_hop_32_ ) ) {
    const EthernetAddress& dst { ARP_Cache[next_hop_32_].first };
    return transmit( { { dst, ethernet_address_, EthernetHeader::TYPE_IPv4 }, serialize( dgram ) } );
  }
  datagrams_waiting_[next_hop_32_].emplace_back( dgram );

  // 5秒内不连续发送ARP
  if ( waiting_timer_.count( next_hop_32_ ) ) {
    return;
  }

  // 否则，发送ARP广播, 更新计时队列
  waiting_timer_.emplace( next_hop_32_, Timer {} );

  // arp_msg的目标以太地址为 {}
  ARPMessage arp_request = make_arp_message( ARPMessage::OPCODE_REQUEST, {}, next_hop_32_ );

  return transmit(
    { { ETHERNET_BROADCAST, ethernet_address_, EthernetHeader::TYPE_ARP }, serialize( arp_request ) } );
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Your code here.
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST ) {
    return;
  }

  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram dgram;
    if ( parse( dgram, frame.payload ) ) {
      datagrams_received_.emplace( move( dgram ) );
    }
    return;
  }

  if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arp_recv;
    if ( parse( arp_recv, frame.payload ) ) {
      const EthernetAddress sender_Ethernet_address = arp_recv.sender_ethernet_address;
      const AddressNumeric sender_Ip_address = arp_recv.sender_ip_address;
      ARP_Cache[sender_Ip_address] = make_pair( sender_Ethernet_address, Timer {} );

      // 如果 请求
      if ( arp_recv.opcode == ARPMessage::OPCODE_REQUEST
           && arp_recv.target_ip_address == ip_address_.ipv4_numeric() ) {
        ARPMessage arp_reply
          = make_arp_message( ARPMessage::OPCODE_REPLY, sender_Ethernet_address, sender_Ip_address );

        return transmit(
          { { sender_Ethernet_address, ethernet_address_, EthernetHeader::TYPE_ARP }, serialize( arp_reply ) } );
      }

      // 如果 响应
      if ( datagrams_waiting_.count( sender_Ip_address ) ) {
        ARP_Cache[sender_Ip_address] = { sender_Ethernet_address, Timer {} };
        for ( const auto& dgram : datagrams_waiting_[sender_Ip_address] ) {
          transmit(
            { { sender_Ethernet_address, ethernet_address_, EthernetHeader::TYPE_IPv4 }, serialize( dgram ) } );
        }
        waiting_timer_.erase( sender_Ip_address );
        datagrams_waiting_.erase( sender_Ip_address );
      }
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  // 删除ARP_Cache中超过30s条目
  erase_if( ARP_Cache, [&]( auto&& item ) -> bool {
    return item.second.second.tick( ms_since_last_tick ).expired( KEEP_ARP_CACHE_30 );
  } );

  // ARP信号重发
  erase_if( waiting_timer_, [&]( auto&& item ) -> bool {
    return item.second.tick( ms_since_last_tick ).expired( RESEND_DELAY_5 );
  } );
}

ARPMessage NetworkInterface::make_arp_message( const uint16_t opcode,
                                               const EthernetAddress& target_ethernet_address,
                                               const uint32_t target_ip_address ) noexcept
{
  ARPMessage arp_msg;
  arp_msg.opcode = opcode;
  arp_msg.sender_ethernet_address = ethernet_address_;
  arp_msg.sender_ip_address = ip_address_.ipv4_numeric();

  arp_msg.target_ethernet_address = target_ethernet_address; // 广播地址
  arp_msg.target_ip_address = target_ip_address;
  return arp_msg;
}