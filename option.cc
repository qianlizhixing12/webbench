#include "option.h"
#include <iostream>
#include <getopt.h>

using std::cout;
using std::endl;
using std::ios_base;
// using std::npos;

#define METHOD_GET 0
#define METHOD_HEAD 1
#define METHOD_OPTIONS 2
#define METHOD_TRACE 3

#define HTTP_09 0
#define HTTP_10 1
#define HTTP_11 2

const string WebbenchOption::shortopts = "frt:p:c:h";

WebbenchOption::WebbenchOption() {
  force = false;
  force_reload = false;
  benchtime = 30;
  proxyhost = "";
  proxyport = 80;
  clients = 1;
  http10 = HTTP_09;
  method = METHOD_GET;
  url = "";
}

WebbenchOption::~WebbenchOption() {}

void WebbenchOption::usage(void) {
  // 原来的9，1，2代表http09，http10，http11短格式和长格式差异大，强制使用长格式
  cout << "webbench [option]... URL" << endl;
  cout << "  -f|--force               Don't wait for reply from server."
       << endl;
  cout << "  -r|--reload              Send reload request - Pragma: no-cache."
       << endl;
  cout << "  -t|--time <sec>          Run benchmark for <sec> seconds. Default "
          "30."
       << endl;
  cout << "  -p|--proxy <server:port> Use proxy server for request." << endl;
  cout << "  -c|--clients <n>         Run <n> HTTP clients at once. Default "
          "one."
       << endl;
  cout << "  --http09                 Use HTTP/0.9 style requests." << endl;
  cout << "  --http10                 Use HTTP/1.0 protocol." << endl;
  cout << "  --http11                 Use HTTP/1.1 protocol." << endl;
  cout << "  --get                    Use GET request method." << endl;
  cout << "  --head                   Use HEAD request method." << endl;
  cout << "  --options                Use OPTIONS request method." << endl;
  cout << "  --trace                  Use TRACE request method." << endl;
  cout << "  -h|--help                This information." << endl;
}

void WebbenchOption::print_option() const {
  cout.setf(ios_base::boolalpha);
  cout << "webbench option:" << endl;
  // proxy
  cout << "\t--proxy      ";
  if (proxyhost != "") {
    cout << proxyhost << ":" << proxyport;
  } else {
    cout << "no";
  }
  cout << endl;
  // force
  cout << "\t--force      early socket close " << force << endl;
  // reload
  cout << "\t--reload     forcing reload " << force_reload << endl;
  // http version
  cout << "\t--http       using " << param_http_version() << endl;
  // method
  cout << "\t--method     " << param_http_method() << endl;
  // timeout
  cout << "\t--time       " << benchtime << endl;
  // clients
  cout << "\t--clients    " << clients << endl;
  // url
  cout << "\t--url        " << url << endl;
}

void WebbenchOption::param_option(const int argc, char *argv[]) {
  if (argc == 1) {
    usage();
    exit(2);
  }

  // 长命令选项
  int opt = 0;
  string tmp;
  int i;
  const struct option longopts[] = {
      {"help", no_argument, NULL, 'h'},
      {"force", no_argument, NULL, 'f'},
      {"reload", no_argument, NULL, 'r'},
      {"time", required_argument, NULL, 't'},
      {"proxy", required_argument, NULL, 'p'},
      {"clients", required_argument, NULL, 'c'},
      {"http09", no_argument, &http10, HTTP_09},
      {"http10", no_argument, &http10, HTTP_10},
      {"http11", no_argument, &http10, HTTP_11},
      {"get", no_argument, &method, METHOD_GET},
      {"head", no_argument, &method, METHOD_HEAD},
      {"options", no_argument, &method, METHOD_OPTIONS},
      {"trace", no_argument, &method, METHOD_TRACE},
      {NULL, 0, NULL, 0}};
  while ((opt = getopt_long(argc, argv, WebbenchOption::shortopts.c_str(),
                            longopts, NULL)) != EOF) {
    switch (opt) {
    case 0:
      break;
    case '?':
      break;
    case 'f':
      force = true;
      break;
    case 'r':
      force_reload = true;
      break;
    case 't':
      benchtime = atoi(optarg);
      break;
    case 'p':
      // server:port
      tmp = optarg;
      // tmp = strrchr(optarg, ':');
      if (tmp == "") {
        break;
      }
      i = tmp.rfind(":");
      if (i < 0) {
        cout << "Error in option --proxy " << tmp << ": Not valid proxy."
             << endl;
        exit(2);
      }
      if (i == 0) {
        cout << "Error in option --proxy " << tmp << ": Missing hostname."
             << endl;
        exit(2);
      }
      if (i == tmp.size() - 1) {
        cout << "Error in option --proxy " << tmp << " Port number is missing."
             << endl;
        exit(2);
      }
      proxyhost = tmp.substr(0, i);
      proxyport = atoi(tmp.substr(i + 1).c_str());
      break;
    case 'c':
      clients = atoi(optarg);
      break;
    case 'h':
      WebbenchOption::usage();
      exit(0);
      break;
    default:
      break;
    }
  }

  // url
  if (optind == argc) {
    cout << "webbench: Missing URL!" << endl;
    usage();
    exit(2);
  }
  url = argv[optind];
  i = url.find("://");
  if (i < 0) {
    cout << url << ": is not a valid URL." << endl;
    exit(2);
  }
  i = url.find("/", url.find("://") + 3);
  if (i < 0) {
    cout << "Invalid URL syntax - hostname don't ends with '/'." << endl;
    exit(2);
  }
  if (url.size() > 1500) {
    cout << "URL is too long." << endl;
    exit(2);
  }
  if (proxyhost == "") {
    // 只有http协议可以直连，其他协议要代理支持
    if (0 != url.find_first_of("http://")) {
      cout << "Only HTTP protocol is directly supported, set --proxy for "
              "others."
           << endl;
      exit(2);
    }
  }

  // option
  // cout << "----------cmdline option----------" << endl;
  // print_option();

  fix_option();

  // fix option
  // cout << "--------fix cmdline option--------" << endl;
  // print_option();
}

void WebbenchOption::fix_option() {
  if (clients == 0) {
    clients = 1;
  }

  if (benchtime == 0) {
    benchtime = 30;
  }

  // http/0.9不支持缓冲控制
  if (force_reload && proxyhost != "" && http10 < HTTP_10) {
    http10 = HTTP_10;
  }
  // http/0.9只支持get请求
  if (method == METHOD_HEAD && http10 < HTTP_10) {
    http10 = HTTP_10;
  }
  if (method == METHOD_OPTIONS && http10 < HTTP_11) {
    http10 = HTTP_11;
  }
  if (method == METHOD_TRACE && http10 < HTTP_11) {
    http10 = HTTP_11;
  }
  if (proxyhost != "") {
    http10 = HTTP_11;
  }
}

string WebbenchOption::param_http_method() const {
  if (method == METHOD_GET) {
    return "GET";
  } else if (method == METHOD_HEAD) {
    return "HEAD";
  } else if (method == METHOD_OPTIONS) {
    return "OPTIONS";
  } else if (method == METHOD_TRACE) {
    return "TRACE";
  }
}

string WebbenchOption::param_http_version() const {
  if (http10 == HTTP_09) {
    return "HTTP/0.9";
  } else if (http10 == HTTP_10) {
    return "HTTP/1.0";
  } else if (http10 == HTTP_11) {
    return "HTTP/1.1";
  }
}

int WebbenchOption::param_http_clients() const { return clients; }

string WebbenchOption::param_http_path() const {
  if (proxyhost == "") {
    return url.substr(url.find("/", url.find("://") + 3));
  } else {
    return url;
  }
}

string WebbenchOption::param_http_host() const {
  /* protocol/host delimiter */
  if (proxyhost == "") {
    // 是否包含端口号
    int i = url.find("://") + 3;
    int j = url.find(":", i);
    if (j >= 0) {
      return url.substr(i, j - i);
    } else {
      int k = url.find("/", i);
      return url.substr(i, k - i);
    }
  } else {
    return proxyhost;
  }
}

int WebbenchOption::param_http_port() const {
  if (proxyhost == "") {
    // 是否包含端口号
    int i = url.find("://") + 3;
    int j = url.find(":", i);
    if (j >= 0) {
      int k = url.find("/", j);
      return atoi(url.substr(j, k - j).c_str());
    } else {
      return 80;
    }
  } else {
    return proxyport;
  }
}

int WebbenchOption::param_http_benchtime() const { return benchtime; }

bool WebbenchOption::param_http_force() const { return force; }

// http_headers与options强耦合
string WebbenchOption::build_http_headers() const {
  string request;

  // http start-line
  request += param_http_method() + " " + param_http_path() + " " +
             param_http_version() + "\r\n";
  // http headers
  if (http10 > 0) {
    request += "User-Agent: WebBench\r\n";
  }
  if (proxyhost == "" && http10 > 0) {
    request += "Host: " + param_http_host() + "\r\n";
  }
  if (force_reload && proxyhost != "") {
    request += "Pragma: no-cache\r\n";
  }
  if (http10 > 1) {
    request += "Connection: close\r\n";
  }
  /* add empty line at end */
  if (http10 > 0) {
    request += "\r\n";
  }

  return request;
}