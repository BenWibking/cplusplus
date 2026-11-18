// RUN: %cxx -verify -fsyntax-only %s

template <class> concept Visible = true;
namespace api {
inline namespace v1 {
template <class T, class U = int> concept Fits = sizeof(T) <= sizeof(U);
template <class T> struct Box { using value = T; };
}
}
namespace alias = api;

api::Fits<> auto count = 1;
alias::Fits<long> decltype(auto) value = 1;
template <api::Fits<> T> struct Tagged {};
template <class T> concept Returns = requires(T t) { { t } -> alias::Fits<>; };
static_assert(Returns<int>);
static_assert(__is_same(api::Box<short>::value, short));

namespace shadow {
using Visible = int;
Visible named = 1;
}
shadow::Visible auto wrong = 1; // expected-error {{'Visible' does not name a concept}}
missing::Visible auto bad = 1; // expected-error {{nested name specifier must be a class or namespace}}
template <missing::Visible T> struct Bad; // expected-error {{nested name specifier must be a class or namespace}}
template <class T> concept Missing = requires(T t) {
  { t } -> missing::Visible; // expected-error {{nested name specifier must be a class or namespace}}
};

namespace first { template <class> concept Match = true; }
namespace second { template <class> concept Match = true; }
namespace both { using namespace first; using namespace second; }
both::Match auto ambiguous = 1; // expected-error {{reference to 'Match' is ambiguous}}
