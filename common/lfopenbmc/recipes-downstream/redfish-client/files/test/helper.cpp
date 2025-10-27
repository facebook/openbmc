#include "helper.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <numeric>

namespace redfish_client::core
{

using ReceivedHttpRequest = SimpleTestHttpServer::ReceivedHttpRequest;

SimpleTestHttpServer::SimpleTestHttpServer(
    ResponseGenerator responseGenerator,
    const std::unordered_map<std::string, std::string>& responseHeaders) :
    responseGenerator(responseGenerator), responseHeaders(responseHeaders),
    // Start on port 0 to let the OS pick an available port.
    acceptor(ioContext,
             boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0)),
    port(acceptor.local_endpoint().port())
{
    startAccept();
    serverThread = std::make_unique<std::thread>([this]() {
        ioContext.run();
        serverThreadStopped = true;
    });
}

SimpleTestHttpServer::SimpleTestHttpServer(
    const std::string& responseStr,
    const std::unordered_map<std::string, std::string>& responseHeaders) :
    SimpleTestHttpServer(
        [responseStr](const ReceivedHttpRequest&) { return responseStr; },
        responseHeaders)
{}

SimpleTestHttpServer::~SimpleTestHttpServer()
{
    if (!serverThreadStopped)
    {
        stopListening();
    }
}

void SimpleTestHttpServer::stopListening()
{
    acceptor.cancel();
    serverThread->join();
}

unsigned short SimpleTestHttpServer::getPort()
{
    return port;
}

std::vector<ReceivedHttpRequest> SimpleTestHttpServer::getReceivedRequests()
{
    std::lock_guard<std::mutex> lock(requestsMutex);
    return requests;
}

void SimpleTestHttpServer::handleAccept(
    const boost::system::error_code& error,
    std::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    if (error)
    {
        // Server stopping, no need to process this request.
        return;
    }
    boost::beast::error_code ec;
    // This buffer is required to persist across reads
    boost::beast::flat_buffer buffer;
    boost::beast::http::request<boost::beast::http::string_body> req;
    boost::beast::http::read(*socket, buffer, req, ec);
    // Response result.
    std::string response;
    if (!ec)
    {
        ReceivedHttpRequest request{
            .method = req.method_string(),
            .path = req.target(),
            .body = req.body(),
        };
        for (const auto& header : req.base())
        {
            request.headers[header.name_string()] = header.value();
        }
        updateReceivedRequest(request);
        response = "HTTP/1.1 200 OK\r\n";
        // Add some response headers.
        response = std::accumulate(
            responseHeaders.begin(), responseHeaders.end(), response,
            [](const std::string& acc, const auto& header) {
                return acc + header.first + ": " + header.second + "\r\n";
            });
        // Response body.
        response += "\r\n";
        response += responseGenerator(request);
    }
    else
    {
        response = "HTTP/1.1 400 Bad Request\r\n";
    }

    boost::asio::write(*socket, boost::asio::buffer(response));
    startAccept();
}

void SimpleTestHttpServer::updateReceivedRequest(
    const ReceivedHttpRequest& request)
{
    std::lock_guard<std::mutex> lock(requestsMutex);
    requests.push_back(request);
}

void SimpleTestHttpServer::startAccept()
{
    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(ioContext);
    acceptor.async_accept(
        *socket, boost::bind(&SimpleTestHttpServer::handleAccept, this,
                             boost::asio::placeholders::error, socket));
}

}; // namespace redfish_client::core
