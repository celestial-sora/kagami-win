#pragma once
#include <windows.h>
#include <sddl.h>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <cstdint>
#include <chrono>
#include "OutputFormat.h"

namespace kagami {
constexpr uint32_t width = 1280, height = 720, frameBytes = width * height * 4;
struct Header { uint32_t magic, width, height, bytes; };
constexpr Header header{0x314D474B, width, height, frameBytes};
static_assert(sizeof(Header) == 16);
inline bool valid(const Header& h) {
    return h.magic == header.magic && OutputFormat{h.width,h.height,30}.valid() && h.bytes==h.width*h.height*4;
}
struct Frame {Header format=header;std::vector<BYTE> pixels;};
inline bool validPipe(const std::wstring& name) {
    const std::wstring prefix = L"\\\\.\\pipe\\Kagami.S-1-5-21-";
    if (name.size() <= prefix.size() || name.size() > 180 || name.compare(0, prefix.size(), prefix)) return false;
    for (size_t i = prefix.size(); i < name.size(); ++i) if ((name[i] < L'0' || name[i] > L'9') && name[i] != L'-') return false;
    return true;
}
// Every operation is cancellable; never wait on a phone, consumer or pipe indefinitely.
inline bool transfer(HANDLE pipe, void* data, DWORD bytes, bool write, std::atomic<bool>& stop) {
    auto p = static_cast<BYTE*>(data);
    while (bytes && !stop) {
        OVERLAPPED ov{}; ov.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!ov.hEvent) return false;
        DWORD n = 0;
        BOOL ok = write ? WriteFile(pipe, p, bytes, &n, &ov) : ReadFile(pipe, p, bytes, &n, &ov);
        if (!ok && GetLastError() == ERROR_IO_PENDING) {
            DWORD elapsed = 0;
            while (WaitForSingleObject(ov.hEvent, 50) == WAIT_TIMEOUT && !stop && (!write || elapsed < 1000)) elapsed += 50;
            if (stop || (write && elapsed >= 1000)) CancelIoEx(pipe, &ov);
            // Drain completion before releasing the OVERLAPPED/event/buffer.
            ok = GetOverlappedResult(pipe, &ov, &n, TRUE);
        }
        CloseHandle(ov.hEvent);
        if (!ok || !n) return false;
        p += n; bytes -= n;
    }
    return bytes == 0;
}
class FrameReader {
    std::atomic<bool> stopped{true};
    std::thread worker;
    std::mutex lock;
    std::vector<BYTE> latest;
    Header latestFormat=header;
    std::wstring endpoint;
public:
    ~FrameReader() { stop(); }
    void stop() { stopped = true; if (worker.joinable()) worker.join(); std::lock_guard guard(lock); latest.clear(); }
    void start(const std::wstring& name) {
        if (endpoint == name && !stopped) return;
        stop(); endpoint = name;
        if (!validPipe(name)) return;
        stopped = false;
        worker = std::thread([this] {
            while (!stopped) {
                HANDLE pipe = CreateFileW(endpoint.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
                if (pipe == INVALID_HANDLE_VALUE) { Sleep(50); continue; }
                Header h{};
                std::vector<BYTE> pixels;
                while (!stopped && transfer(pipe, &h, sizeof(h), false, stopped) && valid(h)) {
                    pixels.resize(h.bytes);
                    if(!transfer(pipe,pixels.data(),h.bytes,false,stopped))break;
                    std::lock_guard guard(lock); latest = pixels;latestFormat=h;
                }
                CloseHandle(pipe);
                { std::lock_guard guard(lock); latest.clear(); }
            }
        });
    }
    std::vector<BYTE> snapshot() { std::lock_guard guard(lock); return latest; }
    Frame snapshotFrame() {std::lock_guard guard(lock);return {latestFormat,latest};}
};
inline std::wstring userSid() {
    HANDLE token{};
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return {};
    DWORD size{}; GetTokenInformation(token, TokenUser, nullptr, 0, &size);
    std::vector<BYTE> data(size);
    bool ok = GetTokenInformation(token, TokenUser, data.data(), size, &size) != FALSE;
    CloseHandle(token);
    if (!ok) return {};
    LPWSTR sid{};
    if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(data.data())->User.Sid, &sid)) return {};
    std::wstring result(sid); LocalFree(sid); return result;
}
inline std::wstring pipeName() { return L"\\\\.\\pipe\\Kagami." + userSid(); }
class FrameWriter {
    std::atomic<bool> stopped{true};
    std::thread worker;
    std::mutex lock;
    std::vector<BYTE> latest = std::vector<BYTE>(frameBytes, 0);
    Header latestFormat=header;
    unsigned fps=30;
    HANDLE pipe = INVALID_HANDLE_VALUE;
public:
    ~FrameWriter() { stop(); }
    bool start() {
        stop();
        auto sid = userSid(); auto name = L"\\\\.\\pipe\\Kagami." + sid;
        if (!validPipe(name)) return false;
        // Only this desktop user may produce. FrameServer identities may read.
        std::wstring sddl = L"D:P(A;;GA;;;" + sid + L")(A;;GR;;;SY)(A;;GR;;;LS)";
        PSECURITY_DESCRIPTOR descriptor{};
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &descriptor, nullptr)) return false;
        SECURITY_ATTRIBUTES sa{sizeof(sa), descriptor, FALSE};
        pipe = CreateNamedPipeW(name.c_str(), PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, 1, 1920*1080*4 + sizeof(Header), 0, 0, &sa);
        LocalFree(descriptor);
        if (pipe == INVALID_HANDLE_VALUE) return false;
        stopped = false;
        worker = std::thread([this] {
            while (!stopped) {
                OVERLAPPED ov{}; ov.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
                if (!ov.hEvent) break;
                BOOL connected = ConnectNamedPipe(pipe, &ov);
                DWORD error = GetLastError();
                if (!connected && error == ERROR_IO_PENDING) {
                    while (WaitForSingleObject(ov.hEvent, 50) == WAIT_TIMEOUT && !stopped) {}
                    if (stopped) CancelIoEx(pipe, &ov);
                    DWORD ignored{}; connected = GetOverlappedResult(pipe, &ov, &ignored, TRUE);
                } else if (!connected && error == ERROR_PIPE_CONNECTED) connected = TRUE;
                CloseHandle(ov.hEvent);
                if (!connected) continue;
                auto deadline = std::chrono::steady_clock::now();
                while (!stopped) {
                    std::vector<BYTE> pixels;
                    Header h;unsigned rate;
                    { std::lock_guard guard(lock); pixels = latest;h=latestFormat;rate=fps; }
                    if (!transfer(pipe, &h, sizeof(h), true, stopped) || !transfer(pipe, pixels.data(), h.bytes, true, stopped)) break;
                    deadline += std::chrono::microseconds(1000000/rate);
                    std::this_thread::sleep_until(deadline);
                    if (deadline < std::chrono::steady_clock::now()) deadline = std::chrono::steady_clock::now();
                }
                DisconnectNamedPipe(pipe);
            }
        });
        return true;
    }
    bool push(const std::vector<BYTE>& pixels,OutputFormat format={}) {
        if(!format.valid()||pixels.size()!=size_t(format.width)*format.height*4)return false;
        std::lock_guard guard(lock);latest=pixels;latestFormat={header.magic,format.width,format.height,unsigned(pixels.size())};fps=format.fps;return true;
    }
    void slate() { std::lock_guard guard(lock); latest.assign(frameBytes, 0);latestFormat=header;fps=30; }
    void stop() { stopped = true; if (worker.joinable()) worker.join(); if (pipe != INVALID_HANDLE_VALUE) CloseHandle(pipe); pipe = INVALID_HANDLE_VALUE; slate(); }
};
}
