#pragma once

#include <netinet/in.h>
#include <sys/types.h>

#include <string>
#include <vector>
#include <mutex>

namespace trace {

  const std::string default_dog_host = "127.0.0.1";
  const uint32_t default_dog_port = 8125;
  const uint32_t optimal_payload = 65467;
  const uint32_t max_udp_payload = 65467;
  const uint32_t max_buff_cmds = 50;

  class Dogstatsd {
    public:
      Dogstatsd(std::vector<std::string>);
      Dogstatsd(bool, std::vector<std::string>);
      Dogstatsd(std::string, uint32_t, bool, std::vector<std::string>);
      ~Dogstatsd();

      int gauge( std::string, double, std::vector<std::string>, double );
      int count( std::string, int64_t, std::vector<std::string>, double );
      int histogram( std::string, double, std::vector<std::string>, double );
      int incr( std::string, std::vector<std::string>, double );
      int decr( std::string, std::vector<std::string>, double );
      int set( std::string, std::string, std::vector<std::string>, double );
    protected:
      std::string host;
      uint32_t port;
      bool buffered;
      std::vector<std::string> cmd_buffer;
      std::string global_tags_str;
      std::vector<std::string> global_tags;
      int send( std::string, std::string, std::vector<std::string>, double );
      int flush();
    private:
      int _sockfd;
      std::mutex _buff_mutex;
      struct sockaddr_in _server;

      std::string format( std::string, std::string, std::vector<std::string>, double );

  };

  /**
   * Singleton.
   */
  extern Dogstatsd dogstatsd;

}
