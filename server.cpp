// server.cpp
#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <thread>
#include <string>

using boost::asio::ip::tcp;

// Directory to store uploaded files
const std::string STORAGE_DIR = "./server_storage/";

void handle_client(tcp::socket socket) {
    try {
        boost::asio::streambuf buf;
        boost::asio::read_until(socket, buf, "\n");
        std::istream input(&buf);
        std::string command;
        input >> command;

        if (command == "UPLOAD") {
            std::string filename;
            size_t filesize;
            input >> filename >> filesize;
            std::cout << "Receiving file: " << filename << " (" << filesize << " bytes)" << std::endl;

            // Open file for writing
            std::ofstream outfile(STORAGE_DIR + filename, std::ios::binary);
            if (!outfile) {
                std::cerr << "Failed to open file for writing.\n";
                return;
            }

            size_t remaining = filesize;
            while (remaining > 0) {
                size_t len = boost::asio::read(socket, buf, boost::asio::transfer_at_least(1));
                std::istream data_stream(&buf);
                std::vector<char> data(len);
                data_stream.read(data.data(), len);
                outfile.write(data.data(), len);
                remaining -= len;
            }

            std::cout << "File " << filename << " received successfully.\n";

        } else if (command == "DOWNLOAD") {
            std::string filename;
            input >> filename;
            std::cout << "Client requests file: " << filename << std::endl;

            // Open file for reading
            std::ifstream infile(STORAGE_DIR + filename, std::ios::binary | std::ios::ate);
            if (!infile) {
                std::cerr << "File not found: " << filename << std::endl;
                std::string msg = "ERROR File not found\n";
                boost::asio::write(socket, boost::asio::buffer(msg));
                return;
            }

            size_t filesize = infile.tellg();
            infile.seekg(0, std::ios::beg);

            // Send confirmation and filesize
            std::string msg = "OK " + std::to_string(filesize) + "\n";
            boost::asio::write(socket, boost::asio::buffer(msg));

            // Send file data
            char buffer[1024];
            while (infile.read(buffer, sizeof(buffer)) || infile.gcount() > 0) {
                boost::asio::write(socket, boost::asio::buffer(buffer, infile.gcount()));
            }

            std::cout << "File " << filename << " sent successfully.\n";

        } else {
            std::cerr << "Unknown command received: " << command << std::endl;
        }

    } catch (std::exception& e) {
        std::cerr << "Exception in thread: " << e.what() << "\n";
    }
}

int main() {
    try {
        boost::asio::io_context io_context;

        // Listen on port 12345
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12345));

        std::cout << "Server started. Listening on port 12345...\n";

        while (true) {
            tcp::socket socket(io_context);
            acceptor.accept(socket);
            std::cout << "New connection from " << socket.remote_endpoint() << std::endl;

            // Handle client in a separate thread
            std::thread(handle_client, std::move(socket)).detach();
        }

    } catch (std::exception& e) {
        std::cerr << "Server exception: " << e.what() << "\n";
    }

    return 0;
}

