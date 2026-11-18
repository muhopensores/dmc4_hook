#include "Server.hpp"
#include <httplib.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>
#include <chrono>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <thread>

namespace debug_server {
namespace {

constexpr const char* kServerName = "dmc4_hook";
constexpr const char* kServerVersion = "S0";
constexpr auto kGameTimeout = std::chrono::seconds(3);

// ---- tools ----

std::mutex g_tools_mutex;
std::vector<Tool> g_tools;

const Tool* find_tool(const std::string& name) {
    std::lock_guard<std::mutex> lock(g_tools_mutex);
    for (const Tool& t : g_tools)
        if (t.name == name) return &t;   // tools are only added before the server starts: the pointer stays valid
    return nullptr;
}

// ---- game-thread jobs ----

struct Job {
    std::function<json()> fn;
    std::promise<json> done;
};
std::mutex g_jobs_mutex;
std::deque<std::shared_ptr<Job>> g_jobs;

json run_on_game_thread(std::function<json()> fn) {
    auto job = std::make_shared<Job>();
    job->fn = std::move(fn);
    std::future<json> f = job->done.get_future();
    {
        std::lock_guard<std::mutex> lock(g_jobs_mutex);
        g_jobs.push_back(job);
    }
    if (f.wait_for(kGameTimeout) != std::future_status::ready)
        throw std::runtime_error("the game thread didn't take the call within 3 s (loading, or the game isn't running "
                                 "its frames)");
    return f.get();
}

// ---- calls ----

std::mutex g_calls_mutex;
std::deque<CallRecord> g_calls;

json call_tool(const std::string& name, const json& args) {
    const Tool* t = find_tool(name);
    if (!t) throw std::runtime_error("no tool '" + name + "'");
    const auto t0 = std::chrono::steady_clock::now();
    bool ok = false;
    auto record = [&] {
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        std::lock_guard<std::mutex> lock(g_calls_mutex);
        g_calls.push_back({name, ok, ms});
        while (g_calls.size() > 20) g_calls.pop_front();
    };
    try {
        json r;
        if (t->game_thread) {
            auto fn = t->run;
            r = run_on_game_thread([fn, args] { return fn(args); });
        } else {
            r = t->run(args);
        }
        ok = true;
        record();
        return r;
    } catch (...) {
        record();
        throw;
    }
}

// ---- JSON-RPC / MCP ----

json rpc_error(const json& id, int code, const std::string& message) {
    return json{{"jsonrpc", "2.0"}, {"id", id}, {"error", {{"code", code}, {"message", message}}}};
}

json rpc_result(const json& id, json result) {
    return json{{"jsonrpc", "2.0"}, {"id", id}, {"result", std::move(result)}};
}

// One request; returns null for a notification (no reply).
json handle(const json& req) {
    if (!req.is_object() || !req.contains("method") || !req["method"].is_string())
        return rpc_error(req.is_object() && req.contains("id") ? req["id"] : json(), -32600, "invalid request");
    const std::string method = req["method"];
    const bool notification = !req.contains("id");
    const json id = notification ? json() : req["id"];
    const json params = req.contains("params") ? req["params"] : json::object();
    if (notification) return json();   // notifications/initialized ...

    try {
        if (method == "initialize") {
            std::string version = "2025-03-26";
            if (params.contains("protocolVersion") && params["protocolVersion"].is_string()) version = params["protocolVersion"];
            return rpc_result(id, {{"protocolVersion", version},
                                   {"capabilities", {{"tools", json::object()}}},
                                   {"serverInfo", {{"name", kServerName}, {"version", kServerVersion}}},
                                   {"instructions", "The running Devil May Cry 4 (DX9) with dmc4_hook. Game-state tools run on "
                                                    "the game thread, once per frame."}});
        }
        if (method == "ping") return rpc_result(id, json::object());
        if (method == "tools/list") {
            json list = json::array();
            std::lock_guard<std::mutex> lock(g_tools_mutex);
            for (const Tool& t : g_tools)
                list.push_back({{"name", t.name}, {"description", t.description}, {"inputSchema", t.schema}});
            return rpc_result(id, {{"tools", list}});
        }
        if (method == "tools/call") {
            const std::string name = params.value("name", "");
            const json args = params.contains("arguments") ? params["arguments"] : json::object();
            try {
                const json r = call_tool(name, args);
                return rpc_result(id, {{"content", json::array({{{"type", "text"}, {"text", r.dump(1)}}})},
                                       {"isError", false}});
            } catch (const std::exception& e) {
                return rpc_result(id, {{"content", json::array({{{"type", "text"}, {"text", e.what()}}})},
                                       {"isError", true}});
            }
        }
        if (find_tool(method)) return rpc_result(id, call_tool(method, params));   // plain RPC for other tools
        return rpc_error(id, -32601, "no method '" + method + "'");
    } catch (const std::exception& e) {
        return rpc_error(id, -32000, e.what());
    }
}

// ---- the HTTP server ----

std::mutex g_server_mutex;
std::unique_ptr<httplib::Server> g_server;
std::thread g_thread;
int g_port = 0;

// ---- log sink ----

struct LogLine {
    long long seq;
    int level;
    std::string text;
};

class RingSink : public spdlog::sinks::base_sink<std::mutex> {
public:
    json read(long long since, int max) {
        std::lock_guard<std::mutex> lock(mutex_);
        json lines = json::array();
        size_t first = 0;
        if (since < 0) {
            const size_t want = (size_t)(-since);
            first = lines_.size() > want ? lines_.size() - want : 0;
        } else {
            while (first < lines_.size() && lines_[first].seq <= since) ++first;
        }
        for (size_t i = first; i < lines_.size() && (int)lines.size() < max; ++i)
            lines.push_back({{"seq", lines_[i].seq}, {"level", lines_[i].level}, {"text", lines_[i].text}});
        return json{{"next", next_ - 1}, {"lines", lines}};
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        std::string text(msg.payload.data(), msg.payload.size());
        lines_.push_back({next_++, (int)msg.level, std::move(text)});
        while (lines_.size() > 4000) lines_.pop_front();
    }
    void flush_() override {}

private:
    std::deque<LogLine> lines_;
    long long next_ = 1;
};
std::shared_ptr<RingSink> g_sink;

} // namespace

void add_tool(Tool tool) {
    std::lock_guard<std::mutex> lock(g_tools_mutex);
    for (Tool& t : g_tools)
        if (t.name == tool.name) { t = std::move(tool); return; }
    g_tools.push_back(std::move(tool));
}

std::vector<std::string> tool_names() {
    std::lock_guard<std::mutex> lock(g_tools_mutex);
    std::vector<std::string> out;
    for (const Tool& t : g_tools) out.push_back(t.name);
    return out;
}

bool start(int port, std::string& error) {
    std::lock_guard<std::mutex> lock(g_server_mutex);
    if (g_server) return true;
    auto server = std::make_unique<httplib::Server>();
    server->Post("/mcp", [](const httplib::Request& req, httplib::Response& res) {
        json body;
        try {
            body = json::parse(req.body);
        } catch (const std::exception&) {
            res.set_content(rpc_error(json(), -32700, "parse error").dump(), "application/json");
            return;
        }
        json reply;
        if (body.is_array()) {
            reply = json::array();
            for (const json& r : body) {
                json one = handle(r);
                if (!one.is_null()) reply.push_back(std::move(one));
            }
            if (reply.empty()) reply = json();
        } else {
            reply = handle(body);
        }
        if (reply.is_null()) {
            res.status = 202;   // notifications only
            return;
        }
        res.set_content(reply.dump(), "application/json");
    });
    server->Get("/mcp", [](const httplib::Request&, httplib::Response& res) {
        res.status = 405;   // no server-sent event stream
    });
    if (!server->bind_to_port("127.0.0.1", port)) {
        error = "can't listen on 127.0.0.1:" + std::to_string(port) + " (in use?)";
        return false;
    }
    g_server = std::move(server);
    g_port = port;
    httplib::Server* s = g_server.get();
    g_thread = std::thread([s] { s->listen_after_bind(); });
    spdlog::info("DebugServer: listening on http://127.0.0.1:{}/mcp", port);
    return true;
}

void stop() {
    std::unique_ptr<httplib::Server> server;
    {
        std::lock_guard<std::mutex> lock(g_server_mutex);
        if (!g_server) return;
        g_server->stop();
        server = std::move(g_server);
    }
    if (g_thread.joinable()) g_thread.join();
    g_port = 0;
    spdlog::info("DebugServer: stopped");
}

bool running() {
    std::lock_guard<std::mutex> lock(g_server_mutex);
    return g_server != nullptr;
}

int port() { return g_port; }

void install_log_sink() {
    if (g_sink) return;
    g_sink = std::make_shared<RingSink>();
    if (auto logger = spdlog::default_logger()) logger->sinks().push_back(g_sink);
}

json log_lines(long long since, int max) {
    if (!g_sink) return json{{"next", 0}, {"lines", json::array()}};
    return g_sink->read(since, max);
}

void on_game_frame() {
    std::deque<std::shared_ptr<Job>> jobs;
    {
        std::lock_guard<std::mutex> lock(g_jobs_mutex);
        jobs.swap(g_jobs);
    }
    for (auto& job : jobs) {
        try {
            job->done.set_value(job->fn());
        } catch (...) {
            job->done.set_exception(std::current_exception());
        }
    }
}

std::vector<CallRecord> recent_calls() {
    std::lock_guard<std::mutex> lock(g_calls_mutex);
    return {g_calls.begin(), g_calls.end()};
}

} // namespace debug_server
