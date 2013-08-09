#include "vast/ingestor.h"

#include "vast/logger.h"
#include "vast/segment.h"
#include "vast/source/file.h"

#ifdef VAST_HAVE_BROCCOLI
#include "vast/source/broccoli.h"
#endif

namespace vast {

using namespace cppa;

ingestor::ingestor(actor_ptr receiver,
                   size_t max_events_per_chunk,
                   size_t max_segment_size,
                   size_t batch_size)
  : receiver_(receiver),
    max_events_per_chunk_(max_events_per_chunk),
    max_segment_size_(max_segment_size),
    batch_size_(batch_size)
{
  VAST_LOG_VERBOSE("spawning ingestor @" << id());
  chaining(false);
}

void ingestor::init()
{
  become(
      on(atom("DOWN"), arg_match) >> [=](uint32_t /* reason */)
      {
        auto i = sinks_.find(last_sender());
        assert(i != sinks_.end());
        sinks_.erase(i);

        if (sinks_.empty())
          quit();
      },
      on(atom("kill")) >> [=]
      {
        if (sinks_.empty())
          quit();
        for (auto& pair : sinks_)
          pair.first << last_dequeued();
      },
#ifdef VAST_HAVE_BROCCOLI
      on(atom("ingest"), atom("broccoli"), arg_match) >>
        [=](std::string const& host, unsigned port,
            std::vector<std::string> const& events)
      {
        auto src = make_source<source::broccoli>(host, port);
        send(src, atom("subscribe"), events);
        send(src, atom("run"));
      },
#endif
      on(atom("ingest"), "bro15conn", arg_match) >> [=](std::string const& file)
      {
        auto src = make_source<source::bro15conn>(file);
        send(src, atom("run"));
      },
      on(atom("ingest"), "bro2", arg_match) >> [=](std::string const& file)
      {
        auto src = make_source<source::bro2>(file);
        send(src, atom("run"));
      },
      on(atom("ingest"), val<std::string>, arg_match) >> [=](std::string const&)
      {
        VAST_LOG_ERROR("invalid ingestion file type");
      },
      on(atom("run")) >> [=]
      {
        delayed_send(
            self,
            std::chrono::seconds(2),
            atom("statistics"), atom("print"), size_t(0));
      },
      on(atom("statistics"), arg_match) >> [=](size_t rate)
      {
        assert(sinks_.find(last_sender()) != sinks_.end());
        sinks_[last_sender()] = rate;
      },
      on(atom("statistics"), atom("print"), arg_match) >> [=](size_t last)
      {
        size_t sum = 0;
        for (auto& pair : sinks_)
          sum += pair.second;

        if (sum != last)
          VAST_LOG_INFO("ingestor @" << id() <<
                        " ingests at rate " << sum << " events/sec");

        if (! sinks_.empty())
          delayed_send(
              self,
              std::chrono::seconds(1),
              atom("statistics"), atom("print"), sum);
      },
      on_arg_match >> [=](segment& s)
      {
        VAST_LOG_DEBUG("ingestor @" << id() <<
                       " relays segment " << s.id() <<
                       " to receiver @" << receiver_->id());

        sync_send(receiver_, s).then(
            on(atom("ack"), arg_match) >> [=](uuid const& segment_id)
            {
              VAST_LOG_DEBUG("ingestor @" << id() <<
                             " received ack for " << segment_id);
            },
            after(std::chrono::seconds(30)) >> [=]
            {
              VAST_LOG_ERROR("ingestor @" << id() <<
                             " did not receive ack from receiver @" <<
                             receiver_->id());

              // TODO: Handle the failed segment, e.g., by sending it again or
              // saving it to the file system.
            });
      });
}

void ingestor::on_exit()
{
  VAST_LOG_VERBOSE("ingestor @" << id() << " terminated");
}

} // namespace vast
