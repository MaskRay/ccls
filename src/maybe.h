#pragma once

#include <optional.h>

#include <utility>

template <typename T>
class Maybe {
  T storage;

public:
  constexpr Maybe() = default;
  Maybe(const Maybe&) = default;
  Maybe(std::nullopt_t) {}
  Maybe(const T& x) : storage(x) {}
  Maybe(T&& x) : storage(std::forward<T>(x)) {}

  Maybe& operator=(const Maybe&) = default;
  Maybe& operator=(const T& x) {
    storage = x;
    return *this;
  }

  const T *operator->() const { return &storage; }
  T *operator->() { return &storage; }
  const T& operator*() const { return storage; }
  T& operator*() { return storage; }

  bool has_value() const {
    return storage.HasValue();
  }
  explicit operator bool() const { return has_value(); }
  operator optional<T>() const {
    if (has_value())
      return storage;
    return nullopt;
  }

  void operator=(optional<T>&& o) {
    storage = o ? *o : T();
  }

  // Does not test if has_value()
  bool operator==(const Maybe& o) const {
    return storage == o.storage;
  }
};
