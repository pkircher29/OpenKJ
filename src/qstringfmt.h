#ifndef OPENKJ_QSTRINGFMT_H
#define OPENKJ_QSTRINGFMT_H

#include <QString>
#include <string>
#include <spdlog/fmt/fmt.h>

// fmt 9 stopped formatting types that only provide operator<<. OpenKJ passes
// QString straight to spdlog, which older bundled fmt accepted through
// spdlog/fmt/ostr.h. This specialization keeps those call sites working on
// fmt 9+ without changing them, and is skipped on older fmt where the
// ostream fallback is still in effect.
#if FMT_VERSION >= 90000
namespace fmt {
template <>
struct formatter<QString> : formatter<std::string> {
    template <typename FormatContext>
    auto format(const QString &value, FormatContext &ctx) const -> decltype(ctx.out()) {
        return formatter<std::string>::format(value.toStdString(), ctx);
    }
};
}
#endif

#endif
