/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#pragma once

#include <optional>

#include <utility>

// Like std::optional, but the stored data is responsible for containing the
// empty state. T should define a function `bool T::Valid()`.
template <typename T> class Maybe {
  T storage;

public:
  constexpr Maybe() = default;
  Maybe(const Maybe &) = default;
  Maybe(std::nullopt_t) {}
  Maybe(const T &x) : storage(x) {}
  Maybe(T &&x) : storage(std::forward<T>(x)) {}

  Maybe &operator=(const Maybe &) = default;
  Maybe &operator=(const T &x) {
    storage = x;
    return *this;
  }

  const T *operator->() const { return &storage; }
  T *operator->() { return &storage; }
  const T &operator*() const { return storage; }
  T &operator*() { return storage; }

  bool Valid() const { return storage.Valid(); }
  explicit operator bool() const { return Valid(); }
  operator std::optional<T>() const {
    if (Valid())
      return storage;
    return std::nullopt;
  }

  void operator=(std::optional<T> &&o) { storage = o ? *o : T(); }

  // Does not test if has_value()
  bool operator==(const Maybe &o) const { return storage == o.storage; }
  bool operator!=(const Maybe &o) const { return !(*this == o); }
};
