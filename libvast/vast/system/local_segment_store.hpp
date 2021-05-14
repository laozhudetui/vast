//    _   _____   __________
//   | | / / _ | / __/_  __/     Visibility
//   | |/ / __ |_\ \  / /          Across
//   |___/_/ |_/___/ /_/       Space and Time
//
// SPDX-FileCopyrightText: (c) 2021 The VAST Contributors
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "vast/fwd.hpp"

#include "vast/chunk.hpp"
#include "vast/system/actors.hpp"

#include <caf/typed_event_based_actor.hpp>

namespace vast::system {

struct active_store_state {
  std::filesystem::path path;
  std::unique_ptr<vast::segment_store> store;
};

struct passive_store_state {
  chunk_ptr data;
};

store_builder_actor::behavior_type
active_local_store(store_builder_actor::stateful_pointer<active_store_state>,
                   filesystem_actor filesystem,
                   const std::filesystem::path& dir);

store_actor::behavior_type
passive_local_store(store_actor::stateful_pointer<passive_store_state>,
                    filesystem_actor filesystem,
                    const std::filesystem::path& path);

} // namespace vast::system
