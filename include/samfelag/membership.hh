/**
 *  @file membership.hh
 *  @author Olivier Détour (detour.olivier@gmail.com)
 */
#ifndef SAMFELAG_MEMBERSHIP_HH_
#define SAMFELAG_MEMBERSHIP_HH_

#include <atomic>
#include <iostream>
#include <string>
#include <thread>

#include <bokasafn/net/socket.hh>

using namespace std::chrono_literals;

namespace samfelag
{

namespace rpc
{

enum class msg_type_t
{
  discover_req = 1,
  discover_resp = 2,
  leave_req = 3,
  leave_resp = 4,
};

template <typename T>
struct discover_request_t
{
  constexpr static msg_type_t type = msg_type_t::discover_req;

  template <typename... Args>
  discover_request_t(std::string const & id_, Args &&... args) : info(std::forward<Args>(args)...)
  {
    memset(id, 0, sizeof(id));
    id_.copy(id, id_.size() + 1);
  }

  char id[ 32 ];
  T info;
};

template <typename T>
struct discover_response_t
{
  constexpr static msg_type_t type = msg_type_t::discover_resp;

  template <typename... Args>
  discover_response_t(std::string const & id_, Args &&... args) : info(std::forward<Args>(args)...)
  {
    memset(id, 0, sizeof(id));
    id_.copy(id, id_.size() + 1);
  }

  char id[ 32 ];
  T info;
};

template <typename T>
struct leave_request_t
{
  constexpr static msg_type_t type = msg_type_t::leave_req;

  leave_request_t(std::string const & id_)
  {
    memset(id, 0, sizeof(id));
    id_.copy(id, id_.size() + 1);
  }

  char id[ 32 ];
};

template <typename T>
struct leave_response_t
{
  constexpr static msg_type_t type = msg_type_t::leave_resp;
};

template <typename T>
struct msg_t
{
  template <typename... Args>
  msg_t(Args &&... args) : type(T::type), data(std::forward<Args>(args)...)
  {
  }

  msg_type_t type;
  T data;
};

} /** !rpc */

/**
 * @brief
 */
template <typename peers_manager_t>
class membership
{
public:
  using peer_info_t = typename peers_manager_t::data_t;

  using discover_request_t = rpc::discover_request_t<peer_info_t>;
  using discover_response_t = rpc::discover_response_t<peer_info_t>;
  using leave_request_t = rpc::leave_request_t<peer_info_t>;
  using leave_response_t = rpc::leave_response_t<peer_info_t>;

public:
  membership(bokasafn::net::saddr const & group_saddr,
             bokasafn::net::saddr const & listen_saddr,
             peers_manager_t & peers_mgt)
    : group_saddr_(group_saddr)
    , listen_saddr_(listen_saddr)
    , in_cluster_(false)
    , peers_mgt_(peers_mgt)
  {
    s_.set_option(bokasafn::net::option<SOL_SOCKET, SO_REUSEADDR, int>(true));
    s_.bind(listen_saddr_);
    s_.set_option(bokasafn::net::option<IPPROTO_IP, IP_MULTICAST_LOOP, int>(false));
  }

public:
  bool
  recvrpc()
  {
    bokasafn::net::saddr a;
    char buffer[ 1024 ];
    auto type = reinterpret_cast<rpc::msg_type_t *>(buffer);

    auto s = s_.recvfrom(a, buffer, sizeof(buffer));
    if (s < 0)
      return false;

    switch (*type)
    {
      case rpc::msg_type_t::discover_req:
      {
        auto req = reinterpret_cast<rpc::msg_t<discover_request_t> *>(buffer);
        // std::cout << "(" << a << ") --> Discover Request from " << req->data.id << std::endl;

        peers_mgt_.add_peer(req->data.id, req->data.info);

        // std::cout << "(" << a << ") <-- Discover Response sent to " << req->data.id <<
        // std::endl;
        send<discover_response_t>(a, peers_mgt_.id(), peers_mgt_.data());
        break;
      }

      case rpc::msg_type_t::discover_resp:
      {
        auto resp = reinterpret_cast<rpc::msg_t<discover_response_t> *>(buffer);
        // std::cout << "(" << a << ") --> Discover Response from " << resp->data.id << std::endl;

        peers_mgt_.add_peer(resp->data.id, resp->data.info);
        break;
      }

      case rpc::msg_type_t::leave_req:
      {
        auto req = reinterpret_cast<rpc::msg_t<leave_request_t> *>(buffer);
        // std::cout << "(" << a << ") --> Leave Request from " << req->data.id << std::endl;

        peers_mgt_.remove_peer(req->data.id);
        break;
      }

      default:
        break;
    }

    return true;
  }

public:
  bool
  keepalive() const
  {
    if (in_cluster_)
    {
      // std::cout << "<-- Discover Request sent" << std::endl;
      send_group<discover_request_t>(peers_mgt_.id(), peers_mgt_.data());
    }

    return in_cluster_;
  }

public:
  void
  join()
  {
    s_.join_mgroup(group_saddr_);

    in_cluster_ = true;
  }

  void
  leave()
  {
    // std::cout << "<-- Leave Request sent" << std::endl;
    send_group<leave_request_t>(peers_mgt_.id());

    s_.leave_mgroup(group_saddr_);

    in_cluster_ = false;
  }

public:
  template <typename R, typename... Args>
  size_t
  send_group(Args &&... args) const
  {
    rpc::msg_t<R> msg(std::forward<Args>(args)...);

    return s_.sendto(group_saddr_, &msg, sizeof(msg));
  }

  template <typename R, typename... Args>
  size_t
  send(bokasafn::net::saddr & addr, Args &&... args) const
  {
    rpc::msg_t<R> msg(std::forward<Args>(args)...);

    return s_.sendto(addr, &msg, sizeof(msg));
  }

public:
  auto const &
  socket() const
  {
    return s_;
  }

private:
  bokasafn::net::saddr group_saddr_;
  bokasafn::net::saddr listen_saddr_;

  bokasafn::net::socket<AF_INET, SOCK_DGRAM, IPPROTO_UDP> s_;

  std::atomic_bool in_cluster_;

  peers_manager_t & peers_mgt_;
};

} /** !samfelag */

#endif /** !SAMFELAG_MEMBERSHIP_HH_ */
