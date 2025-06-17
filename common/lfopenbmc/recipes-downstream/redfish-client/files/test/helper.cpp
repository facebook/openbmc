#include "helper.hpp"

namespace redfish_client_daemon
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

ReceivedHttpRequest SimpleTestHttpServer::parseSimpleRequest(
    const std::string& requestStr)
{
    ReceivedHttpRequest rv;

    // Parse the first line (method, url, version)
    size_t pos = requestStr.find(' ');
    rv.method = requestStr.substr(0, pos);
    size_t pos2 = requestStr.find(' ', pos + 1);
    rv.path = requestStr.substr(pos + 1, pos2 - pos - 1);
    rv.version = requestStr.substr(pos2 + 1, requestStr.find('\n') - pos2 - 1);

    // Parse the headers
    while ((pos = requestStr.find('\n', pos2)) != std::string::npos)
    {
        pos2 = requestStr.find('\n', pos + 1);
        if (pos2 == std::string::npos)
            break;
        size_t colonPos = requestStr.find(':', pos + 1);
        if (colonPos == std::string::npos || colonPos > pos2)
            continue;
        std::string key = requestStr.substr(pos + 1, colonPos - pos - 1);
        std::string value =
            requestStr.substr(colonPos + 1, pos2 - colonPos - 1);
        rv.headers[key] = value;
    }
    return rv;
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
    boost::asio::streambuf requestBuffer;
    boost::asio::read_until(*socket, requestBuffer, "\r\n\r\n");

    std::string requestStr((std::istreambuf_iterator<char>(&requestBuffer)),
                           std::istreambuf_iterator<char>());
    ReceivedHttpRequest request = parseSimpleRequest(requestStr);

    // Response result.
    std::string response = "HTTP/1.1 200 OK\r\n";

    // Add some response headers.
    for (const auto& header : responseHeaders)
    {
        response += header.first + ": " + header.second + "\r\n";
    }

    // Response body.
    response += "\r\n";
    response += responseGenerator(request);
    boost::asio::write(*socket, boost::asio::buffer(response));
    updateReceivedRequest(request);
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

}; // namespace redfish_client_daemon
