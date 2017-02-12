#include<arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>       /* time */
#include <errno.h>

#include <iostream>
#include <sstream> // for ostringstream
#include <iomanip>
#include <algorithm>

#include <cstdio>
#include <cstdlib>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#include "trace_dogstatsd.hpp"

namespace trace {

  Dogstatsd::Dogstatsd(std::vector<std::string> tags) 
    : Dogstatsd(false, tags) { }
  Dogstatsd::Dogstatsd(
      bool buffered,
      std::vector<std::string> tags = {}
      )
    : Dogstatsd(default_dog_host, default_dog_port, buffered, tags) { }
  Dogstatsd::Dogstatsd(
      std::string host,
      uint32_t port,
      bool buffered,
      std::vector<std::string> tags = {})
    : buffered(buffered){
      memset(&_server, 0, sizeof(struct sockaddr_in));
      _sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); //IPv4(only)/UDP
      _server.sin_family = AF_INET;
      _server.sin_port = htons(port);
      _server.sin_addr.s_addr = inet_addr(host.c_str());
      global_tags = tags;

      std::ostringstream tagstream;
      std::vector<std::string>::iterator it = global_tags.begin();
      if (it != global_tags.end()) {
        tagstream << *it++;
      }
      for ( ; it != global_tags.end() ; it++) {
        tagstream << "," << *it;
      }
      global_tags_str = tagstream.str();

      //plant a random seed
      srand(time(NULL));

      //add a timer to periodically flush the cmd buffer.

    }
  Dogstatsd::~Dogstatsd(){
    if (buffered) {
      flush();
    }
    close(_sockfd);
  }

  int Dogstatsd::gauge( std::string name, double value, std::vector<std::string> tags, double rate){
    std::ostringstream statd;
    statd << std::setprecision(6) << value << "|g";
    return send(name, statd.str(), tags, rate);
  }
  int Dogstatsd::count( std::string name, int64_t value, std::vector<std::string> tags, double rate){
    std::ostringstream statd;
    statd << value << "|c";
    return send(name, statd.str(), tags, rate);
  }
  int Dogstatsd::histogram( std::string name, double value, std::vector<std::string> tags, double rate){
    std::ostringstream statd;
    statd << std::setprecision(6) << value << "|h";
    return send(name, statd.str(), tags, rate);
  }
  int Dogstatsd::incr( std::string name, std::vector<std::string> tags, double rate){
    return send(name, "1|c", tags, rate);
  }
  int Dogstatsd::decr( std::string name, std::vector<std::string> tags, double rate){
    return send(name, "-1|c", tags, rate);
  }
  int Dogstatsd::set( std::string name, std::string value, std::vector<std::string> tags, double rate){
    std::ostringstream statd;
    statd << value << "|s";
    return send(name, statd.str(), tags, rate);
  }
  int Dogstatsd::send( std::string name, std::string statd, std::vector<std::string> tags, double rate){
    int ret = 0;

    if (rate < 1 && (rate * 100) < (rand() % 100)) {
      return ret;
    }

    std::string cmd(format(name, statd, tags, rate));

    if (!buffered) {
      return sendto(_sockfd, cmd.c_str(), cmd.length(), 0, (struct sockaddr *)&_server, sizeof(_server));
    }

    std::lock_guard<std::mutex> lock(_buff_mutex);
    if (cmd_buffer.size() < max_buff_cmds-1) {
      cmd_buffer.push_back(cmd);
    } else {
      cmd_buffer.push_back(cmd);
      ret = flush();
      std::cout << "FLUSH(" << _sockfd << ") result: " << ret << " errno: " << strerror(errno) << std::endl;
    }
    return ret;
  }

  //not thread-safe, call holding lock.
  //optimize to be more optimal.
  int Dogstatsd::flush(){
    int ret = 0;
    std::ostringstream statd;
    if(!buffered) {
      return 0;
    }
    while (!cmd_buffer.empty()) {
      if (static_cast<int>(statd.tellp()) + cmd_buffer.back().length() > optimal_payload) {
        std::string buffer(statd.str());
        ret += sendto( _sockfd, buffer.c_str(), buffer.length(), 
            0, (struct sockaddr *)&_server, sizeof(_server));

        //clear the stringstream
        statd.str("");
        statd.clear();
      }
      statd << cmd_buffer.back() << std::endl;
      cmd_buffer.pop_back();
    }

    std::string buffer(statd.str());
    ret += sendto(_sockfd, buffer.c_str(), buffer.length(), 0, (struct sockaddr *)&_server, sizeof(_server));

    return ret;
  }
  std::string Dogstatsd::format( std::string name, std::string value, std::vector<std::string> tags, double rate){
    bool tagging = false;
    std::ostringstream statd;

    statd << name << ":" << value;
    if (rate < 1) {
      statd << "|@" << rate;
    }

    if (tags.size()) {
      tagging = true;
      // TODO: remove newlines from the tags.
      std::vector<std::string>::iterator it = tags.begin();

      statd << "|#" << *it++;
      for( ; it != tags.end() ; ++it) {
        statd << "," << *it;
      }
    }

    if (!global_tags_str.empty()) {
      if (!tagging) {
        statd << "|#" ;
      } else {
        statd << "," ;
      }
      statd << global_tags_str;
    }
    return statd.str();
  }

  static std::vector<std::string> get_default_tags(void) {
    char path[1024];
    uint32_t size = sizeof(path);
    std::string fullpath;
    std::vector<std::string> tags = {};
#if defined(__APPLE__)
    if (_NSGetExecutablePath(path, &size) == 0) {
      fullpath = std::string(path);
    }
#elif defined(__LINUX__)
    if (readlink("/proc/self/exe", path, size) > 0) {
      fullpath = std::string(path);
    }
#endif
    if (!fullpath.empty()) {
      std::string binary(fullpath.substr(fullpath.find_last_of('/')));
      std::replace(binary.begin(), binary.end(), ' ', '_');
      tags.push_back("binary:"+binary);
    }
    return tags;

  }

  Dogstatsd dogstatsd(true, get_default_tags());

}
