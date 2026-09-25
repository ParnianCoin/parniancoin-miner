/**
 * Various utility functions
 *
 * Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com> - https://parniancoin.com
 * Author: Amir Reza Zamani <amirrezazamani@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation. It is distributed WITHOUT ANY WARRANTY.
 * See the LICENSE file or <https://www.gnu.org/licenses/gpl-3.0.html>.
 */

///
/// @file
/// Copyright (c) 2003-2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
/// @copyright Amir Reza Zamani <amirrezazamani@gmail.com>
/// @author Amir Reza Zamani <amirrezazamani@gmail.com>

#pragma once

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Boost ext
#include <boost/asio.hpp>

namespace boost {
namespace asio {

template <typename SyncReadStream, typename Allocator>
std::size_t RH_ReadLine(SyncReadStream& s,
    boost::asio::basic_streambuf<Allocator>& b, const std::string& delim,
    boost::system::error_code& ec)
{
  std::size_t search_position = 0;
  for (;;)
  {
    // Determine the range of the data to be searched.
    typedef typename boost::asio::basic_streambuf<
      Allocator>::const_buffers_type const_buffers_type;
    typedef boost::asio::buffers_iterator<const_buffers_type> iterator;
    const_buffers_type buffers = b.data();
    iterator begin = iterator::begin(buffers);
    iterator start_pos = begin + search_position;
    iterator end = iterator::end(buffers);

    // Look for a match.
    std::pair<iterator, bool> result = detail::partial_search(
        start_pos, end, delim.begin(), delim.end());
    if (result.first != end)
    {
      if (result.second)
      {
        // Full match. We're done.
        ec = boost::system::error_code();
        return result.first - begin + delim.length();
      }
      else
      {
        // Partial match. Next search needs to start from beginning of match.
        search_position = result.first - begin;
      }
    }
    else
    {
      // No match. Next search can start with the new data.
      search_position = end - begin;
    }

    // Check if buffer is full.
    if (b.size() == b.max_size())
    {
      ec = error::not_found;
      return 0;
    }

    // Need more data.
    std::size_t bytes_to_read = read_size_helper(b, 65536);
    b.commit(s.read_some(b.prepare(bytes_to_read), ec));
    if (ec)
      return 0;
  }
}


template <typename SyncReadStream, typename Allocator>
inline std::size_t RH_ReadLine(SyncReadStream& s,
    boost::asio::basic_streambuf<Allocator>& b, const std::string& delim)
{
  boost::system::error_code ec;
  std::size_t bytes_transferred = RH_ReadLine(s, b, delim, ec);
  boost::asio::detail::throw_error(ec, "RH_ReadLine");
  return bytes_transferred;
}

}
}
