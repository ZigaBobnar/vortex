#include <Vortex/Vortex.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <Maze/Maze.hpp>
#include <Server/Http/HttpServer.h>
#include <Core/Logging.h>
#include <Core/Util/String.h>
#include <Core/Modules/DependencyInjection.h>
#include <Core/Modules/ModuleLoader.h>
#include <boost/filesystem.hpp>
#ifdef HAS_FEATURE_MONGO
#include <mongocxx/instance.hpp>
#endif

namespace Util = Vortex::Core::Util;

namespace Vortex {
    std::vector<std::thread> running_servers;

    void start_vortex(std::vector<std::string> args) {
        Core::Logging::Logger::initialize();

#ifdef HAS_FEATURE_MONGO
        mongocxx::instance instance{};
#endif

        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i] == "help" || args[i] == "--help" || args[i] == "-h")
                return show_help();
        }

        if (args.size() <= 1) {
            // No additional arguments were supplied
            start_from_config("server_config.json");
        }
        else if (args[1] == "start") {
            bool errors_detected = false;

            int port = 8080;
            std::string address = "0.0.0.0";
            std::string data_dir = "";
            int thread_count = 4;
            std::string type = "http";

            if (args.size() > 2) {
                for (size_t i = 2; i < args.size(); ++i) {
                    std::string arg = args[i];
                    bool arg_has_error = false;

                    if (arg.length() > 2) {
                        if (Util::String::starts_with(arg, "-p=") || Util::String::starts_with(arg, "--port=")) {
                            std::vector<std::string> val = Util::String::split(arg, "=");

                            try {
                                port = std::stoi(val[1]);
                            }
                            catch (std::invalid_argument) {
                                arg_has_error = true;
                            }
                        }
                        else if (Util::String::starts_with(arg, "-a=") || Util::String::starts_with(arg, "--address=")) {
                            std::vector<std::string> val = Util::String::split(arg, "=");

                            try {
                                address = val[1];
                            }
                            catch (std::invalid_argument) {
                                arg_has_error = true;
                            }
                        }
                        else if (Util::String::starts_with(arg, "-d=") || Util::String::starts_with(arg, "--data_dir=")) {
                            std::vector<std::string> val = Util::String::split(arg, "=");

                            try {
                                data_dir = val[1];
                            }
                            catch (std::invalid_argument) {
                                arg_has_error = true;
                            }
                        }
                        else if (Util::String::starts_with(arg, "-t=") || Util::String::starts_with(arg, "--thread_count=")) {
                            std::vector<std::string> val = Util::String::split(arg, "=");

                            try {
                                thread_count = stoi(val[1]);
                            }
                            catch (std::invalid_argument) {
                                arg_has_error = true;
                            }
                        }
                        else {
                            arg_has_error = true;
                        }
                    }
                    else {
                        arg_has_error = true;
                    }

                    if (arg_has_error) {
                        VORTEX_CRITICAL("Invalid syntax for argument: {0}", arg);
                        errors_detected = true;
                    }
                }
            }

            if (errors_detected) {
                exit_with_error(1000);
            }
            else {
                Maze::Element config(Maze::Type::Object), server_config(Maze::Type::Object);
                server_config.set("port", port);
                server_config.set("address", address);
                server_config.set("thread_count", thread_count);
                server_config.set("type", type);
                config.set("server", server_config);

                if (data_dir.length() > 0) {
                    try {
                        boost::filesystem::path path_obj(data_dir);

                        if (boost::filesystem::exists(path_obj) && boost::filesystem::is_directory(path_obj)) {
                            data_dir = boost::filesystem::canonical(data_dir).string();
                            config.set("vortex", Maze::Element({ "data_dir" }, { data_dir }));
                        }
                        else {
                            VORTEX_CRITICAL("Given data_dir path is not a directory.");
                            exit_with_error(1007);
                        }
                    }
                    catch (const boost::filesystem::filesystem_error& e) {
                        VORTEX_CRITICAL("data_dir filesystem error: {0}", e.what());
                        exit_with_error(1008);
                    }
                }

                start_http_server(config);
            }

        }
        else if (args[1] == "console") {
            start_console();
        }
        else if (Util::String::starts_with(args[1], "-c=") || Util::String::starts_with(args[1], "--config=")) {
            std::vector<std::string> val = Util::String::split(args[1], "=");

            std::string config_path;
            try {
                config_path = val[1];
            }
            catch (...) {
                VORTEX_CRITICAL("Invalid syntax for argument: {0}", args[1]);
                exit_with_error(1001);
            }

            start_from_config(config_path);
        }
        else {
            VORTEX_CRITICAL("Invalid command line arguments. [{0}]", args[1]);
            VORTEX_CRITICAL("Use ./vortex help to find more info on how to use the program.");
            exit_with_error(1002);
        }

        for (size_t i = 0; i < running_servers.size(); ++i) {

            if (running_servers[i].joinable()) {
                running_servers[i].join();
            }
        }
    }

    void exit_with_error(int error_code) {
        VORTEX_CRITICAL("Errors were detected (Code: {0}). Aborting...", error_code);
        exit(error_code);
    }

    void show_help() {
        std::cout << "Vortex Help" << std::endl
            << " Usage: ./vortex [command] [--arg1=val1 ...]" << std::endl
            << std::endl
            << std::endl
            << " Available commands:" << std::endl
            << "  (empty)        Tries to run the vortex servers from config file" << std::endl
            << "    --config=[path/to/config.json]" << std::endl
            << "    -c=[path/to/config.json]     Default: server_config.json" << std::endl
            << std::endl
            << "  start          Starts the vortex server" << std::endl
            << "    --port=[port]" << std::endl
            << "    -p=[port]                    Default: 8080" << std::endl
            << "    --address=[ip]" << std::endl
            << "    -a=[ip]                      Default: 127.0.0.1" << std::endl
            << "    --data_dir=[path/to/data]" << std::endl
            << "    -d=[path/to/data]            Default: (empty)" << std::endl
            << "    --thread_count=[num]" << std::endl
            << "    -t=[num]                     Default: 4" << std::endl
            << std::endl
            << "  console        Starts vortex shell in interactive mode" << std::endl
            << std::endl
            << "  help           Displays help" << std::endl;
    }

    void start_console() {
        while (true) {
            std::cout << "Vortex>";

            std::string str;
            std::getline(std::cin, str);

            if (str.length() == 0) {
                continue;
            }
            else if (str == "start") {
                start_server();
            }
            else if (str == "exit" || str == "q" || str == "quit") {
                std::cout << "Exiting Vortex..." << std::endl;
                exit(0);
            }
            else if (str == "help") {
                show_help();
            }
            else {
                std::cout << "'" << str << "' is not valid command. " << std::endl;
            }
        }
    }

    void start_from_config(const std::string& config_file_name) {
        Maze::Element config(Maze::Type::Object);

        std::ifstream config_file(config_file_name);
        if (config_file.is_open()) {
            std::stringstream buffer;
            buffer << config_file.rdbuf();

            try {
                config = Maze::Element::from_json(buffer.str());
            }
            catch (...) {
                VORTEX_CRITICAL("Unable to parse config file '{0}'.", config_file_name);
                exit_with_error(1003);
            }

            config_file.close();
        }
        else {
            VORTEX_CRITICAL("Unable to find config file '{0}'.", config_file_name);
            exit_with_error(1004);
        }

        if (config.is_array("servers") && config.get("servers").has_children()) {
            const Maze::Element& servers = config.get("servers");

            for (const Maze::Element& server : servers) {
                Maze::Element global_config = config;
                global_config.remove("servers");

                if (server.is_int()) {
                    if (global_config.is_object("server")) {
                        Maze::Element& conf = global_config.get_ref("server");
                        conf.set("port", server.get_int());
                    }
                    else {
                        global_config.set("server", Maze::Element({ "port" }, { server.get_int() }));
                    }

                    running_servers.push_back(std::thread(start_http_server, global_config));
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                else if (server.is_object()) {
                    global_config.apply(server);

                    running_servers.push_back(std::thread(start_http_server, global_config));
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                else {
                    VORTEX_CRITICAL("Server config file is invalid: '{0}'", server.to_json());
                    exit_with_error(1005);
                }
            }
        }
        else {
            VORTEX_CRITICAL("Unable to find any servers to start in config file.");
            exit_with_error(1006);
        }
    }

    void start_server() {
        start_server(Maze::Element(Maze::Type::Object));
    }

    void start_server(const Maze::Element& config) {
        if (config.is_object("server")) {
            const Maze::Element& server_config = config.get("server");

            if (server_config.is_string("type")) {
                const std::string& type = server_config.get("type").s();

                if (type == "https") {
                    VORTEX_CRITICAL("Https server is not yet implemented.");

                    return;
                }
                else if (type == "websocket") {
                    VORTEX_CRITICAL("Websocket server is not yet implemented.");

                    return;
                }
                else if (type == "http") {
                    running_servers.push_back(std::thread(start_http_server, config));
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));

                    return;
                }
                else {
                    VORTEX_CRITICAL("Server type '{0}' does not exist.", type);

                    return;
                }
            }
        }

        running_servers.push_back(std::thread(start_http_server, config));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    using Vortex::Core::Modules::DependencyInjector;
    using Vortex::Core::Modules::Module;
    DependencyInjector* init_di(const Maze::Element& config);

    void start_http_server(const Maze::Element& config) {
        DependencyInjector* program_di = init_di(config);

        Vortex::Server::Http::HttpServer server;

        server.start(config, program_di->di_scope());
    }

    DependencyInjector* init_di(const Maze::Element& config) {
        DependencyInjector* di_scope = DependencyInjector::di_global()->di_scope();

        Maze::Element module_config = config.get("modules");

        if (module_config.is_array("load")) {
            for (const auto& module_conf : module_config.get("load")) {
                if (module_conf.is_string()) {
                    std::string module_name = module_conf.get_string();

                    Module* module = di_scope->module_loader()->load_module(module_name);

                    if (module != nullptr) {
                        module->register_di(di_scope);
                    }
                    else {
                        VORTEX_ERROR("Unable to load module '{0}'", module_name);
                    }
                }
            }
        }

        return di_scope;
    }

}  // namespace Vortex
