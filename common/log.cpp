#include "common/log.hpp"

#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/sources/exception_handler_feature.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/exception_handler.hpp>
#include <boost/log/utility/setup/console.hpp>

// The operator is used for regular stream formatting
std::ostream& operator<<(std::ostream& strm, severity_level level)
{
  static const char* strings[] = {"debug", "info", "warning", "error", "fatal"};

  if (static_cast<std::size_t>(level) < sizeof(strings) / sizeof(*strings))
  {
    strm << strings[level];
  }
  else
  {
    strm << static_cast<int>(level);
  }

  return strm;
}

// The operator is used when putting the severity level to log
logging::formatting_ostream& operator<<(
    logging::formatting_ostream& strm,
    logging::to_log_manip<severity_level, severity_tag> const& manip)
{
  static const char* strings[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

  severity_level level = manip.get();
  if (static_cast<std::size_t>(level) < sizeof(strings) / sizeof(*strings))
    strm << strings[level];
  else
    strm << static_cast<int>(level);

  return strm;
}

namespace phemex::common
{
Log::Log(const config::Log& conf, const std::string& program_name)
  : dir_{conf.dir},
    program_name_{program_name},
    max_size_{conf.max_size},
    rotation_size_{conf.rotation_size},
    min_free_space_{conf.min_free_space},
    max_log_files_{conf.max_log_files}
{
  const auto filename = dir_ + "/" + program_name_ + "_%Y-%m-%d.%N.log";
  boost::shared_ptr<sinks::text_file_backend> backend =
      boost::make_shared<sinks::text_file_backend>(
          keywords::file_name     = filename,
          keywords::open_mode     = (std::ios::out | std::ios::app),
          keywords::rotation_size = rotation_size_,
          keywords::time_based_rotation =
              sinks::file::rotation_at_time_point(0, 0, 0));

  sink_ = boost::shared_ptr<sink_t>(new sink_t(
      backend, keywords::order = logging::make_attr_ordering(
                   "RecordID", std::less<unsigned int>())));

  SetFormater();
  SetFileCollector();
  SetGlobalAttributes();
  SetExceptionHandler();
}

void Log::AutoFlush(bool enabled)
{
  sink_->locked_backend()->auto_flush(enabled);
}

void Log::Level(const std::string& level)
{
  static const std::vector<std::string> levels = {"debug", "info", "warning",
                                                  "error", "fatal"};

  auto lc_level = boost::algorithm::to_lower_copy(level);

  for (unsigned i = 0; i < levels.size(); ++i)
  {
    if (0 == levels[i].compare(lc_level))
    {
      // disable logging for level < error.
      logging::core::get()->set_filter(
          expr::attr<severity_level>("Severity") >= i);
      return;
    }
  }

  logging::core::get()->set_filter(expr::attr<severity_level>("Severity") >= 0);
}

void Log::Flush() const
{
  sink_->flush();
}

void Log::SetFormater()
{
  sink_->set_formatter(
      expr::format("%1% %2% %3% - %4% %5%") %
      expr::format_date_time<boost::posix_time::ptime>(
          "TimeStamp", "%Y-%m-%d %H:%M:%S.%f") %
      program_name_ % expr::attr<severity_level, severity_tag>("Severity") %
      expr::smessage %
      expr::format_named_scope("Scope", keywords::format = "[%f:%l]"));
}

void Log::SetFileCollector()
{
  sink_->locked_backend()->set_file_collector(sinks::file::make_collector(
      keywords::target = dir_, keywords::max_size = max_size_,
      keywords::min_free_space = min_free_space_,
      keywords::max_files      = max_log_files_));
  // scan the directory for files matching the file_name pattern
  sink_->locked_backend()->scan_for_files();
}

void Log::SetGlobalAttributes()
{
  logging::add_console_log(
      std::cout, boost::log::keywords::format = ">> %Message%");

  logging::core::get()->add_sink(sink_);

  // Add some attributes too
  logging::core::get()->add_global_attribute("TimeStamp", attrs::local_clock());
  // BOOST_LOG_SCOPED_THREAD_TAG("ThreadID", boost::this_thread::get_id());
  logging::core::get()->add_global_attribute(
      "RecordID", attrs::counter<unsigned int>());
  logging::core::get()->add_global_attribute("Scope", attrs::named_scope());
  // surpress logging exceptions
  logging::core::get()->set_exception_handler(
      logging::make_exception_suppressor());
}

void Log::SetExceptionHandler()
{
  // Setup a global exception handler that will call
  // ExceptionHandler::operator() for the specified exception types
  logging::core::get()->set_exception_handler(
      logging::make_exception_handler<std::runtime_error, std::logic_error>(
          ExceptionHandler{}));
}

Log::~Log()
{
  // Flush all buffered records
  sink_->stop();
  sink_->flush();
}
} // namespace phemex::common
