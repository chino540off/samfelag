/**
 *  @file peers.hh
 *  @author Olivier Détour (detour.olivier@gmail.com)
 */
#ifndef SAMFELAG_PEERS_HH_
#define SAMFELAG_PEERS_HH_

#include <map>

namespace samfelag
{

class peer
{
public:
  using info_t = bokasafn::net::saddr;

public:
  peer(info_t const & saddr)
  {
    std::cout << "trying to connect to " << saddr << std::endl;
    s_.connect(saddr);
  }
  peer(int fd) : s_(fd) {}

private:
  bokasafn::net::socket<AF_INET, SOCK_STREAM, IPPROTO_TCP> s_;
};

/**
 * @brief
 */
class peers
{
public:
  using data_t = peer::info_t;

public:
  peers(std::string const & id, bokasafn::net::saddr const & listen_saddr, data_t const & data)
    : id_(id), listen_saddr_(listen_saddr), data_(data)
  {
    s_.set_option(bokasafn::net::option<SOL_SOCKET, SO_REUSEADDR, int>(true));
    s_.bind(listen_saddr_);
    s_.listen(10);
  }

public:
  auto
  incoming_connection(bokasafn::net::saddr & caddr)
  {
    auto cs = s_.accept(caddr);
    std::cout << "incoming connection from " << caddr << std::endl;

    clients_.insert({caddr, cs});

    return cs;
  }

public:
  bool
  client_msg(bokasafn::net::saddr const &)
  {
    return true;
  }

public:
  void
  add_peer(std::string const & id, data_t const & data)
  {
    auto it = peers_.find(id);

    if (it == peers_.cend())
    {
      peers_.insert({id, std::make_shared<peer>(data)});
      std::cout << " -- add peer: \'" << id << "\'" << std::endl;
    }
  }

  void
  remove_peer(std::string const & id)
  {
    auto it = peers_.find(id);

    if (it != peers_.end())
    {
      peers_.erase(it);
      std::cout << " -- remove peer: \'" << id << "\'" << std::endl;
    }
  }

public:
  std::string const &
  id() const
  {
    return id_;
  }

  data_t const &
  data() const
  {
    return data_;
  }

  auto const &
  socket() const
  {
    return s_;
  }

private:
  bokasafn::net::socket<AF_INET, SOCK_STREAM, IPPROTO_TCP> s_;

  std::string id_;

  bokasafn::net::saddr listen_saddr_;
  data_t data_;

  std::map<std::string, std::shared_ptr<peer>> peers_;

  // Just for referencing
  std::map<bokasafn::net::saddr, bokasafn::net::socket<AF_INET, SOCK_STREAM, IPPROTO_TCP>> clients_;
};

} /** !samfelag */

#endif /** !SAMFELAG_PEERS_HH_ */
