#include "calculus.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <windows.h>
namespace ms {
namespace {
struct Handle {
    HANDLE value = nullptr;
    Handle() = default;
    explicit Handle(HANDLE h) : value(h) {}
    ~Handle() {
        if (value && value != INVALID_HANDLE_VALUE)
            CloseHandle(value);
    }
    Handle(const Handle &) = delete;
    Handle &operator=(const Handle &) = delete;
};
CalculusResult request(char op, Expr f, int order, std::optional<double> point, int direction,
                       std::optional<std::pair<double, double>> bounds) {
    wchar_t exe[32768];
    if (!GetModuleFileNameW(nullptr, exe, 32768))
        throw std::runtime_error("Cannot locate application");
    auto worker = std::filesystem::path(exe).parent_path() / L"cas" / L"mathsolver_cas_worker.exe";
    if (!std::filesystem::exists(worker))
        throw std::runtime_error(
            "缺少原生符号计算组件 cas/mathsolver_cas_worker.exe，请使用完整安装包");
    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    Handle childIn, parentIn, parentOut, childOut;
    if (!CreatePipe(&childIn.value, &parentIn.value, &sa, 0) ||
        !CreatePipe(&parentOut.value, &childOut.value, &sa, 0))
        throw std::runtime_error("Cannot create CAS pipes");
    SetHandleInformation(parentIn.value, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(parentOut.value, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOW si{};
    Handle errorSink(CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_WRITE, &sa, OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL, nullptr));
    if (errorSink.value == INVALID_HANDLE_VALUE)
        throw std::runtime_error("Cannot create CAS error sink");
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = childIn.value;
    si.hStdOutput = childOut.value;
    si.hStdError = errorSink.value;
    PROCESS_INFORMATION pi{};
    std::wstring command = L"\"" + worker.wstring() + L"\"";
    if (!CreateProcessW(worker.c_str(), command.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                        nullptr, worker.parent_path().c_str(), &si, &pi))
        throw std::runtime_error("无法启动 C++ 符号计算组件，请检查 DLL 是否完整");
    Handle process(pi.hProcess), thread(pi.hThread);
    CloseHandle(childIn.value);
    childIn.value = nullptr;
    CloseHandle(childOut.value);
    childOut.value = nullptr;
    std::ostringstream input;
    std::string p;
    if (point) {
        std::ostringstream n;
        n << std::setprecision(17) << *point;
        p = n.str();
    }
    input << op << ' ' << std::quoted(latex(f)) << ' ' << order << ' ' << std::quoted(p) << ' '
          << direction << ' ' << bool(bounds) << ' ' << std::setprecision(17)
          << (bounds ? bounds->first : 0) << ' ' << (bounds ? bounds->second : 1) << '\n';
    auto bytes = input.str();
    DWORD written;
    if (!WriteFile(parentIn.value, bytes.data(), DWORD(bytes.size()), &written, nullptr) ||
        written != bytes.size()) {
        TerminateProcess(process.value, 2);
        throw std::runtime_error("CAS input failed");
    }
    CloseHandle(parentIn.value);
    parentIn.value = nullptr;
    std::string response;
    auto start = std::chrono::steady_clock::now();
    while (true) {
        DWORD available = 0;
        if (PeekNamedPipe(parentOut.value, nullptr, 0, nullptr, &available, nullptr) && available) {
            char buffer[4096];
            DWORD read = 0;
            if (ReadFile(parentOut.value, buffer, std::min<DWORD>(available, sizeof(buffer)), &read,
                         nullptr))
                response.append(buffer, read);
        } else if (WaitForSingleObject(process.value, 10) == WAIT_OBJECT_0)
            break;
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(15) ||
            response.size() > 2000000) {
            TerminateProcess(process.value, 3);
            throw std::runtime_error("符号计算超时或结果过于复杂");
        }
    }
    std::istringstream stream(response);
    int ok;
    if (!(stream >> ok))
        throw std::runtime_error("符号组件未返回有效结果");
    if (!ok) {
        std::string message;
        stream >> std::quoted(message);
        throw std::runtime_error(message);
    }
    CalculusResult r;
    int kind, verification;
    bool numeric;
    double value;
    if (!(stream >> kind >> verification >> std::quoted(r.formula) >> std::quoted(r.plain) >>
          std::quoted(r.detail) >> numeric >> value))
        throw std::runtime_error("符号计算结果协议错误");
    r.kind = ResultKind(kind);
    r.verification = Verification(verification);
    if (numeric)
        r.numeric = value;
    return r;
}
} // namespace
std::string symbolicBackend() {
    return "libqalculate · native C++ worker";
}
CalculusResult solveDerivative(Expr f, int order, std::optional<double> point) {
    auto r = request('D', f, order, point, 0, {});
    auto d = f;
    for (int i = 0; i < order; ++i)
        d = derivative(d);
    r.expression = d;
    return r;
}
CalculusResult solveIntegral(Expr f, std::optional<std::pair<double, double>> bounds) {
    return request('I', f, 1, {}, 0, bounds);
}
CalculusResult solveLimit(Expr f, double point, int direction) {
    return request('L', f, 1, point, direction, {});
}
CalculusResult simplifyPolynomial(Expr f) {
    return request('S', f, 1, {}, 0, {});
}
} // namespace ms
