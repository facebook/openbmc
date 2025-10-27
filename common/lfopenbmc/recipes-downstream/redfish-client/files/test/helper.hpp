#pragma once

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/system/error_code.hpp>

namespace redfish_client::core
{

class SimpleTestHttpServer
{
  public:
    struct ReceivedHttpRequest
    {
        std::string method;
        std::string path;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
    };
    using ResponseGenerator =
        std::function<std::string(const ReceivedHttpRequest&)>;

    SimpleTestHttpServer(
        ResponseGenerator responseGenerator,
        const std::unordered_map<std::string, std::string>& responseHeaders);

    SimpleTestHttpServer(
        const std::string& responseStr,
        const std::unordered_map<std::string, std::string>& responseHeaders);

    ~SimpleTestHttpServer();

    SimpleTestHttpServer(const SimpleTestHttpServer&) = delete;
    SimpleTestHttpServer(SimpleTestHttpServer&&) = delete;
    SimpleTestHttpServer& operator=(const SimpleTestHttpServer&) = delete;
    SimpleTestHttpServer& operator=(SimpleTestHttpServer&&) = delete;

    void stopListening();

    unsigned short getPort();

    std::vector<ReceivedHttpRequest> getReceivedRequests();

  private:
    void handleAccept(const boost::system::error_code& error,
                      std::shared_ptr<boost::asio::ip::tcp::socket> socket);

    void updateReceivedRequest(const ReceivedHttpRequest& request);

    void startAccept();

    ResponseGenerator responseGenerator;
    std::unordered_map<std::string, std::string> responseHeaders;
    std::vector<ReceivedHttpRequest> requests;
    std::mutex requestsMutex;
    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::acceptor acceptor;
    unsigned short port;
    std::unique_ptr<std::thread> serverThread;
    std::atomic<bool> serverThreadStopped{false};
};

}; // namespace redfish_client::core
