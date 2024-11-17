// server.cpp
#include "httplib.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

// Directory to store uploaded files
const std::string UPLOAD_DIR = "./uploads/";

// Function to generate a unique filename using timestamp
std::string generate_unique_filename(const std::string& original_filename) {
    // Get current time since epoch in milliseconds
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch())
                      .count();

    // Create a timestamp string
    std::ostringstream ss;
    ss << now_ms;

    // Combine timestamp with original filename
    return ss.str() + "_" + original_filename;
}

int main() {
    // Ensure the upload directory exists
    if (!fs::exists(UPLOAD_DIR)) {
        fs::create_directory(UPLOAD_DIR);
    }

    // Create a server
    httplib::Server svr;

    // Route for serving the upload page
    svr.Get("/", [&](const httplib::Request& req, httplib::Response& res) {
        // Serve the index.html file
        std::ifstream html_file("index.html");
        if (html_file.is_open()) {
            std::string html_content((std::istreambuf_iterator<char>(html_file)),
                                      std::istreambuf_iterator<char>());
            res.set_content(html_content, "text/html");
        } else {
            res.status = 404;
            res.set_content("404 Not Found", "text/plain");
        }
    });

    // Route for handling file uploads
    svr.Post("/upload", [&](const httplib::Request& req, httplib::Response& res) {
        // Check if the request has multipart content
        if (req.has_file("file")) {
            auto file = req.get_file_value("file");
            std::string original_filename = file.filename;

            // Generate a unique filename to prevent collisions
            std::string unique_filename = generate_unique_filename(original_filename);

            // Path to save the uploaded file
            std::string file_path = UPLOAD_DIR + unique_filename;

            // Save the file
            std::ofstream ofs(file_path, std::ios::binary);
            if (ofs) {
                ofs.write(file.content.c_str(), file.content.size());
                ofs.close();

                // Generate a download link
                std::string download_link = "/download/" + unique_filename;

                // Simple HTML response with the download link
                std::string response_html = R"(
                    <!DOCTYPE html>
                    <html lang="en">
                    <head>
                        <meta charset="UTF-8">
                        <title>Upload Successful</title>
                        <style>
                            body { font-family: Arial, sans-serif; margin: 50px; }
                            .container { max-width: 500px; margin: auto; }
                            .link { margin-top: 20px; word-wrap: break-word; background-color: #e7f3fe; padding: 15px; border-left: 6px solid #2196F3; }
                            a { color: #2196F3; text-decoration: none; }
                            a:hover { text-decoration: underline; }
                        </style>
                    </head>
                    <body>
                        <div class="container">
                            <h2>File Uploaded Successfully!</h2>
                            <p>Share this link to download the file:</p>
                            <div class="link">
                                <a href=")" + download_link + R"(")> click </a>
                            </div>
                            <br>
                            <a href="/">Upload Another File</a>
                        </div>
                    </body>
                    </html>
                )";

                res.set_content(response_html, "text/html");
            } else {
                res.status = 500;
                res.set_content("Failed to save the file.", "text/plain");
            }
        } else {
            res.status = 400;
            res.set_content("No file uploaded.", "text/plain");
        }
    });

    // Route for handling file downloads
    svr.Get(R"(/download/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() == 2) { // Full match + 1 group
            std::string unique_filename = req.matches[1];
            std::string file_path = UPLOAD_DIR + unique_filename;

            if (fs::exists(file_path)) {
                size_t underscore_pos = unique_filename.find('_');
                std::string original_filename = (underscore_pos != std::string::npos)
                                                    ? unique_filename.substr(underscore_pos + 1)
                                                    : unique_filename;

                std::ifstream ifs(file_path, std::ios::binary | std::ios::ate);
                if (!ifs) {
                    res.status = 500;
                    res.set_content("Failed to open file.", "text/plain");
                    return;
                }
                size_t file_size = ifs.tellg();

                res.set_header("Content-Disposition", "attachment; filename=\"" + original_filename + "\"");

                res.set_content_provider(
                    file_size,                        // Content length
                    "application/octet-stream",       // Content type
                    [file_path](size_t offset, size_t length, httplib::DataSink& sink) -> bool {
                        std::ifstream ifs(file_path, std::ios::binary);
                        if (!ifs) {
                            sink.done();
                            return false;
                        }
                        ifs.seekg(offset, std::ios::beg);
                        char buffer[8192];
                        size_t to_read = length;
                        while (to_read > 0 && ifs.read(buffer, std::min(sizeof(buffer), to_read))) {
                            sink.write(buffer, ifs.gcount());
                            to_read -= ifs.gcount();
                        }
                        if (ifs.gcount() > 0 && to_read > 0) {
                            sink.write(buffer, ifs.gcount());
                            to_read -= ifs.gcount();
                        }
                        sink.done();
                        return true;
                    }
                );
            } else {
                res.status = 404;
                res.set_content("File not found.", "text/plain");
            }
        } else {
            res.status = 400;
            res.set_content("Invalid download link.", "text/plain");
        }
    });

    std::cout << "Server is running on http://localhost:8080/\n";
    svr.listen("0.0.0.0", 8080);

    return 0;
}

