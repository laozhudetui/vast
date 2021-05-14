//    _   _____   __________
//   | | / / _ | / __/_  __/     Visibility
//   | |/ / __ |_\ \  / /          Across
//   |___/_/ |_/___/ /_/       Space and Time
//
// SPDX-FileCopyrightText: (c) 2021 The VAST Contributors
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "vast/fwd.hpp"

#include <filesystem>

namespace vast {

/// @relates segment_store
using partition_local_store_ptr = std::unique_ptr<segment_store>;

/// A store that stores the table slices related to a single partition.
class partition_local_store {
public:
  static partition_local_store_ptr make(std::filesystem::path dir);
};

} // namespace vast
