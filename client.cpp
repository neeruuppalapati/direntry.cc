// client.cpp
#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <string>

using boost::asio::ip::tcp;

// Helper function to upload a file
void upload_file(tcp::socket& socket, const std::string& filepath) {
    std::ifstream infile(filepath, std::ios::binary | std::ios::ate);
    if (!infile) {
        std::cerr << "Cannot open file: " << filepath << std::endl;
        return;
    }

    size_t filesize = infile.tellg();
    infile.seekg(0, std::ios::beg);

    // Extract filename from filepath
    std::string filename;
    size_t pos = filepath.find_last_of("/\\");
    if (pos != std::string::npos)
        filename = filepath.substr(pos + 1);
    else
        filename = filepath;

    // Send UPLOAD command
    std::string command = "UPLOAD " + filename + " " + std::to_string(filesize) + "\n";
    boost::asio::write(socket, boost::asio::buffer(command));

    // Send file data
    char buffer[1024];
    while (infile.read(buffer, sizeof(buffer)) || infile.gcount() > 0) {
        boost::asio::write(socket, boost::asio::buffer(buffer, infile.gcount()));
    }

    std::cout << "File " << filename << " uploaded successfully.\n";
}

// Helper function to download a file
void download_file(tcp::socket& socket, const std::string& filename) {
    // Send DOWNLOAD command
    std::string command = "DOWNLOAD " + filename + "\n";
    boost::asio::write(socket, boost::asio::buffer(command));

    // Read response
    boost::asio::streambuf buf;
    boost::asio::read_until(socket, buf, "\n");
    std::istream input(&buf);
    std::string status;
    input >> status;

    if (status == "OK") {
        size_t filesize;
        input >> filesize;
        std::cout << "Downloading " << filename << " (" << filesize << " bytes)" << std::endl;

        // Open file for writing
        std::ofstream outfile("downloaded_" + filename, std::ios::binary);
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

        std::cout << "File " << filename << " downloaded successfully as downloaded_" << filename << "\n";

    } else {
        std::cerr << "Error downloading file: " << status << std::endl;
    }
}

int main() {
    try {
        boost::asio::io_context io_context;

        // Replace with your server's IP address and port
        std::string server_ip = "127.0.0.1";
        int server_port = 12345;

        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(server_ip, std::to_string(server_port));
        tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);

        std::cout << "Connected to server " << server_ip << ":" << server_port << "\n";

        while (true) {
            std::cout << "\nChoose an option:\n";
            std::cout << "1. Upload a file\n";
            std::cout << "2. Download a file\n";
            std::cout << "3. Exit\n";
            std::cout << "Enter choice: ";

            int choice;
            std::cin >> choice;

            if (choice == 1) {
                std::cout << "Enter the path of the file to upload: ";
                std::string filepath;
                std::cin >> filepath;
                upload_file(socket, filepath);
            } else if (choice == 2) {
                std::cout << "Enter the filename to download: ";
                std::string filename;
                std::cin >> filename;
                download_file(socket, filename);
            } else if (choice == 3) {
                std::cout << "Exiting...\n";
                break;
            } else {
                std::cout << "Invalid choice. Try again.\n";
            }
        }

    } catch (std::exception& e) {
        std::cerr << "Client exception: " << e.what() << "\n";
    }

    return 0;
}

