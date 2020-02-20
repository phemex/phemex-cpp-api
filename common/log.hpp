#pragma once

#include <boost/filesystem.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/keywords/severity.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/utility/record_ordering.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

#include "common/config/log.hpp"

namespace sinks    = boost::log::sinks;
namespace logging  = boost::log;
namespace expr     = boost::log::expressions;
namespace keywords = boost::log::keywords;
namespace attrs    = boost::log::attributes;
namespace src      = boost::log::sources;

enum severity_level
{
  debug,
  info,
  warning,
  error,
  fatal
};

static src::severity_logger_mt<severity_level> client_lg =
    src::severity_logger_mt<severity_level>(keywords::severity = info);

// The operator is used for regular stream formatting
std::ostream& operator<<(std::ostream&, severity_level);
// Attribute value tag type
struct severity_tag;

// The operator is used when putting the severity level to log
logging::formatting_ostream& operator<<(
    logging::formatting_ostream&,
    logging::to_log_manip<severity_level, severity_tag> const&);

namespace phemex::common
{
class Log
{
 public:
  struct ExceptionHandler
  {
    using result_type = void;

    void operator()(std::runtime_error const& e) const
    {
      // do nothing
    }

    void operator()(std::logic_error const& e) const
    {
      // rethrow
      throw;
    }
  };

  using backend_t = sinks::text_file_backend;
  using sink_t    = sinks::asynchronous_sink<
      backend_t, sinks::bounded_ordering_queue<
                     logging::attribute_value_ordering<
                         unsigned int, std::less<unsigned int>>,
                     1000000, sinks::block_on_overflow>>;

 public:
  ~Log();

  static Log& Get(const config::Log& conf, const std::string& program_path)
  {
    const auto program_name = boost::filesystem::basename(program_path);
    static Log logger(conf, program_name);
    logger.Level(conf.log_level);
    logger.AutoFlush(conf.flush_flag);

    return logger;
  }

  void AutoFlush(bool);
  void Level(const std::string&);
  void Flush() const;

 private:
  Log(const config::Log&, const std::string&);
  inline void SetFormater();
  inline void SetFileCollector();
  inline void SetGlobalAttributes();
  inline void SetExceptionHandler();

 private:
  boost::shared_ptr<sink_t> sink_;
  const std::string dir_;
  const std::string program_name_;
  const uint64_t max_size_;
  const uint64_t rotation_size_;
  const uint64_t min_free_space_;
  const uint64_t max_log_files_;
};
} // namespace phemex::common
