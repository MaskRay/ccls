#pragma once

#include <optional.h>

#include <utility>

// Like optional, but the stored data is responsible for containing the empty
// state. T should define a function `bool T::HasValueForMaybe_()`.
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

  const T* operator->() const { return &storage; }
  T* operator->() { return &storage; }
  const T& operator*() const { return storage; }
  T& operator*() { return storage; }

  bool HasValue() const { return storage.HasValueForMaybe_(); }
  explicit operator bool() const { return HasValue(); }
  operator optional<T>() const {
    if (HasValue())
      return storage;
    return nullopt;
  }

  void operator=(optional<T>&& o) { storage = o ? *o : T(); }

  // Does not test if has_value()
  bool operator==(const Maybe& o) const { return storage == o.storage; }
  bool operator!=(const Maybe& o) const { return !(*this == o); }
};
