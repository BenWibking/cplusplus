// RUN: %cxx -verify -fsyntax-only %s
// expected-no-diagnostics

namespace foreign {
inline namespace abi {
using Size = long;
using Imported = char;
namespace detail { using Foreign = short; }
}
}

namespace api {
using namespace foreign::abi;
using Size = int;
inline namespace v1 {
inline namespace inner {
namespace detail { using Local = Size; }
}
}
using namespace v1;
}

namespace api::detail { using Reopened = Local; }
static_assert(__is_same(api::Size, int));
static_assert(__is_same(api::Imported, char));
static_assert(__is_same(api::v1::inner::detail::Reopened, int));
static_assert(__is_same(foreign::detail::Foreign, short));

namespace api { struct Arg {}; }
namespace foreign::inline abi { void foreign_only(api::Arg); }
template <class T> concept HasForeign = requires(T arg) { foreign_only(arg); };
static_assert(!HasForeign<api::Arg>);

namespace separate {
namespace { namespace detail { using Local = long; } }
namespace detail { using Local = int; }
}
static_assert(__is_same(separate::detail::Local, int));
