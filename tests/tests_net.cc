#include <cstring>

#include <bokasafn/epoll.hh>
#include <bokasafn/utils/signal.hh>
#include <samfelag/membership.hh>
#include <samfelag/peers.hh>

int
main(int argc, char const ** argv)
{
  if (argc != 3)
    return 1;

  bokasafn::epoll<20> e;
  bokasafn::utils::signal_handler<SIGINT>([&](int) { e.stop(); });

  std::string id = argv[ 1 ];
  std::uint16_t port = 12345;

  bokasafn::net::saddr ga{"239.255.255.250", port};
  bokasafn::net::saddr sa{INADDR_ANY, port};
  bokasafn::net::saddr joinable_sa{argv[ 2 ], port};

  samfelag::peers pe{id, sa, joinable_sa};

  samfelag::membership<decltype(pe)> mb{ga, sa, pe};

  try
  {
    e.add(pe.socket().fd(), [&e, &pe](int) {
      bokasafn::net::saddr caddr{};

      auto cs = pe.incoming_connection(caddr);
      e.add(cs.fd(), [&pe, &caddr](int) { return pe.client_msg(caddr); });

      return true;
    });

    e.timer(1s, [&mb](int) { return mb.keepalive(); });
    e.add(mb.socket().fd(), [&mb](int) { return mb.recvrpc(); });

    mb.join();

    e.start(500ms);
  }
  catch (std::exception const & e)
  {
    std::cout << e.what() << std::endl;
  }

  mb.leave();

  return 0;
}
