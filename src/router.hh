#pragma once

#include <memory>
#include <optional>

#include "exception.hh"
#include "network_interface.hh"

#include <array>
#include <unordered_map>

// 存储 Interface 号， 下一跳地址 （主机）router地址
using router_Info = std::pair<size_t, std::optional<Address>>;

// \brief A router that has multiple network interfaces and
// performs longest-prefix-match routing between them.
class Router
{
public:
  // Add an interface to the router
  // \param[in] interface an already-constructed network interface
  // \returns The index of the interface after it has been added to the router
  size_t add_interface( std::shared_ptr<NetworkInterface> interface )
  {
    _interfaces.push_back( notnull( "add_interface", std::move( interface ) ) );
    return _interfaces.size() - 1;
  }

  // Access an interface by index
  std::shared_ptr<NetworkInterface> interface( const size_t N ) { return _interfaces.at( N ); }

  // Add a route (a forwarding rule)
  void add_route( uint32_t route_prefix, // 目的网络
                  uint8_t prefix_length,
                  std::optional<Address> next_hop,
                  size_t interface_num );

  // Route packets between the interfaces
  void route();

private:
  // The router's collection of network interfaces
  std::vector<std::shared_ptr<NetworkInterface>> _interfaces {};

  // array下标表示prefix_length
  std::array<std::unordered_map<uint32_t, router_Info>, 32> routing_table_ {};

  std::optional<router_Info> match( uint32_t router_prefix ) const noexcept;
};
