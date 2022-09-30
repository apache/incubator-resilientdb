#include <pistache/endpoint.h>

using namespace Pistache;

class HelloHandler : public Http::Handler {
 public:
  HTTP_PROTOTYPE(HelloHandler)

  void onRequest(const Http::Request& request, Http::ResponseWriter response) {
    response.send(Http::Code::Ok, "Hello, World\n");
  }
};

int main() {
  Address addr(Ipv4::any(), Port(9080));

  auto opts = Http::Endpoint::options().threads(1);
  Http::Endpoint server(addr);
  server.init(opts);
  server.setHandler(Http::make_handler<HelloHandler>());
  server.serve();
}