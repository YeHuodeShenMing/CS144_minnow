#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  // Your code here.
  // rotr(x, n) x右移n位 将 网络号 位存进去即可
  // 存的 router_Info
  routing_table_[prefix_length][ route_prefix >> ((prefix_length == 0) ? 0 : (32 - prefix_length ))] = { interface_num, next_hop };
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // Your code here.
  for ( const auto& interface : _interfaces ) {
    auto&& data_received = interface->datagrams_received();
    while ( !data_received.empty() ) {
      InternetDatagram dgram { move( data_received.front() ) };
      data_received.pop();

      if ( dgram.header.ttl <= 1 ) {
        continue;
      }

      dgram.header.ttl -= 1;
      dgram.header.compute_checksum();

      const optional<router_Info>& target_router { match( dgram.header.dst ) };
      cout << "target_router.has_value() : " << target_router.has_value() <<endl;
      if ( !target_router.has_value() ) {
        continue;
      }
      const auto& [target_interface, next_hop] { target_router.value() };

      // 对应的interface来发送
      // 如果 optional 包含一个值，则返回该值；如果不包含值（即 optional 为 nullopt），则返回提供的默认值。
      _interfaces[target_interface]->send_datagram(
        dgram, next_hop.value_or( Address::from_ipv4_numeric( dgram.header.dst ) ) );
      cerr << "DEBUG: router = " << _interfaces[target_interface]->name() << endl;
    }
  }
}

// 寻找 对应 Interface
optional<router_Info> Router::match( uint32_t address ) const noexcept
{
  for ( int i = 31; i >= 0; i-- ) {
    // 全0时候匹配 default router
    uint32_t aligned_address = (i == 0) ? 0 : address >> ( 32 - i );
    if ( routing_table_[i].count( aligned_address ) ) {
      // cout << "I_F num" << routing_table_[i].at(aligned_address).first << endl;
      return routing_table_[i].at( aligned_address );
    }
  }
  return nullopt;
}
