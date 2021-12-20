
#include <syslog.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include <iomanip>
#include <ctime>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <random>
#include <memory>
#include <type_traits>
#include <functional>
#include <utility>
#include <algorithm>
#include <string>
#include <map>
#include <vector>
#include <deque>
#include <cstring>
#include <optional>
#include <atomic>

#include <boost/program_options.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <absl/container/flat_hash_map.h>
//#include <server_ws.hpp>