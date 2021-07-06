#ifndef WEBBENCH_OPTION_H_
#define WEBBENCH_OPTION_H_

#include <string>

using std::string;

class WebbenchOption {
public:
  WebbenchOption();
  ~WebbenchOption();
  static void usage(void);
  void print_option() const;
  void param_option(const int argc, char *argv[]);
  string param_http_method() const;
  string param_http_version() const;
  int param_http_clients() const;
  string param_http_path() const;
  string param_http_host() const;
  int param_http_port() const;
  int param_http_benchtime() const;
  bool param_http_force() const;
  string build_http_headers() const;
  static const string shortopts;

private:
  void fix_option();
  bool force;
  bool force_reload;
  int benchtime;
  string proxyhost;
  int proxyport;
  int clients;
  int http10;
  int method;
  string url;
};

#endif