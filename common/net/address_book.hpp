#pragma once

#include "common/config/host_address.hpp"

namespace phemex::common::net
{
class AddressBook
{
  using HostAddress    = config::HostAddress;
  using HostAddressPtr = typename config::HostAddress::Ptr;

 public:
  template <class Book>
  AddressBook(Book&& book) : index_{0}, book_{std::forward<Book>(book)}
  {
    Validate();
  }

  AddressBook(const HostAddress& addr) : index_{0}
  {
    book_.push_back(addr);
  }

  AddressBook(const HostAddressPtr& addr) : index_{0}
  {
    book_.push_back(*addr);
  }

  AddressBook(const std::string& host, unsigned short port)
    : index_{0}, book_{{{host, port}}}, url_{host + ":" + std::to_string(port)}
  {
  }

  AddressBook(const AddressBook&) = delete;
  AddressBook& operator=(const AddressBook&) = delete;

  inline void Validate() const
  {
    if (0 == book_.size())
    {
      throw std::runtime_error{"empty address book"};
    }
  }

  inline auto UseSSL() const
  {
    if ("wss" == book_[index_].protocol || "https" == book_[index_].protocol ||
        "tcps" == book_[index_].protocol || "udps" == book_[index_].protocol ||
        "ssl" == book_[index_].protocol)
    {
      return true;
    }
    return false;
  }

  inline const auto& Protocol() const
  {
    return book_[index_].protocol;
  }

  inline const auto& Host() const
  {
    return book_[index_].host;
  }

  inline auto Port() const
  {
    return book_[index_].port;
  }

  inline const auto& Path() const
  {
    return book_[index_].path;
  }

  inline const auto& Url() const
  {
    return book_[index_].url;
  }

  inline void Path(int32_t i, const std::string& path)
  {
    book_[i].Path(path);
  }

  inline void Path(const std::string& path)
  {
    for (auto& addr : book_)
    {
      addr.Path(path);
    }
  }

  inline void UseNext()
  {
    if (0 == book_.size())
    {
      return;
    }
    index_    = (index_ + 1) % book_.size();
    protocol_ = book_[index_].protocol;
    host_     = book_[index_].host;
    port_     = book_[index_].port;
    path_     = book_[index_].path;
    url_      = book_[index_].url;
  }

 protected:
  virtual ~AddressBook()
  {
  }

 private:
  uint32_t index_;
  std::vector<HostAddress> book_;
  std::string protocol_;
  std::string host_;
  unsigned short port_;
  std::string path_;
  std::string url_;
};
} // namespace phemex::common::net
