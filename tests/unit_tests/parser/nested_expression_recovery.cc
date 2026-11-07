// RUN: not %cxx -fsyntax-only %s

template <class T>
concept arithmetic = requires(T a, T b) {
  a (((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((* b;
};
