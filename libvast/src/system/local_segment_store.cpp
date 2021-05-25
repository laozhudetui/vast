//    _   _____   __________
//   | | / / _ | / __/_  __/     Visibility
//   | |/ / __ |_\ \  / /          Across
//   |___/_/ |_/___/ /_/       Space and Time
//
// SPDX-FileCopyrightText: (c) 2021 The VAST Contributors
// SPDX-License-Identifier: BSD-3-Clause

#include "vast/system/local_segment_store.hpp"

#include "vast/ids.hpp"
#include "vast/logger.hpp"
#include "vast/query.hpp"
#include "vast/segment_store.hpp"
#include "vast/table_slice.hpp"

#include <caf/settings.hpp>
#include <fmt/format.h>

#include <vector>

namespace vast::system {

std::filesystem::path store_path_for_partition(const uuid& partition_id) {
  auto store_filename = fmt::format("{}.store", partition_id);
  return std::filesystem::path{"archive"} / store_filename;
}

store_actor::behavior_type passive_local_store(
  store_builder_actor::stateful_pointer<passive_store_state> self) {
  return {
    // store
    [self](query, ids) -> atom::done {
      return atom::done_v;
    },
    [self](atom::erase, ids) -> atom::done {
      return atom::done_v;
    },
  };
}

store_builder_actor::behavior_type active_local_store(
  store_builder_actor::stateful_pointer<active_store_state> self) {
  return {
    // store
    [self](query, ids) -> atom::done {
      return atom::done_v;
    },
    [self](atom::erase, const ids& ids) -> caf::result<atom::done> {
      if (auto result = self->state.store->erase(ids); !result)
        return result;
      return atom::done_v;
    },
    // store builder
    [self](
      caf::stream<table_slice> in) -> caf::inbound_stream_slot<table_slice> {
      return self
        ->make_sink(
          in, [=](caf::unit_t&) {},
          [=](caf::unit_t&, std::vector<table_slice>& batch) {
            for (auto& slice : batch)
              if (auto error = self->state.store->put(slice))
                VAST_ERROR("{} failed to add table slice to store {}", self,
                           render(error));
          },
          [=](caf::unit_t&, const caf::error&) {})
        .inbound_slot();
    },
    // Conform to the protocol of the STATUS CLIENT actor.
    [self](atom::status,
           status_verbosity) -> caf::dictionary<caf::config_value> {
      return {};
    },
  };
}

} // namespace vast::system
