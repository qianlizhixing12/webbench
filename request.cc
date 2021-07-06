#include "request.h"
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

using std::cout;
using std::endl;
using std::string;

volatile int timerexpired = 0;

static void alarm_handler(int signal) { timerexpired = 1; }

static int Socket(const string &host, int port) {
  // sockaddr_in
  struct sockaddr_in ad;
  memset(&ad, 0, sizeof(ad));
  ad.sin_family = AF_INET;
  if (unsigned long inaddr = inet_addr(host.c_str()) != INADDR_NONE)
    memcpy(&ad.sin_addr, &inaddr, sizeof(inaddr));
  else {
    struct hostent *hp = gethostbyname(host.c_str());
    if (hp == NULL) {
      return -1;
    }
    memcpy(&ad.sin_addr, hp->h_addr, hp->h_length);
  }
  ad.sin_port = htons(port);
  // socket
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    return sock;
  }
  if (connect(sock, (struct sockaddr *)&ad, sizeof(ad)) < 0) {
    return -1;
  }
  return sock;
}

static void bench_child(const int pipeno, const string &host, const int port,
                        const int benchtime, const string &data,
                        const string &http_version, bool http_force) {
  /* setup alarm signal handler */
  struct sigaction sa;
  sa.sa_handler = alarm_handler;
  sa.sa_flags = 0;
  if (sigaction(SIGALRM, &sa, NULL)) {
    exit(3);
  }
  alarm(benchtime);
  int speed = 0;
  int failed = 0;
  int bytes = 0;
  // int len = ;
nexttry:
  while (1) {
    if (timerexpired) {
      if (failed > 0) {
        failed--;
      }
      break;
    }
    int s = Socket(host, port);
    if (s < 0) {
      failed++;
      continue;
    }
    if (data.size() != write(s, data.c_str(), data.size())) {
      failed++;
      close(s);
      continue;
    }
    if (http_version == "HTTP/0.9") {
      if (shutdown(s, 1)) {
        failed++;
        close(s);
        continue;
      }
    }
    if (!http_force) {
      /* read all available data from socket */
      while (1) {
        if (timerexpired)
          break;
        char buf[1500];
        int i = read(s, buf, 1500);
        /* fprintf(stderr,"%d\n",i); */
        if (i < 0) {
          failed++;
          close(s);
          goto nexttry;
        } else if (i == 0)
          break;
        else
          bytes += i;
      }
    }
    if (close(s)) {
      failed++;
      continue;
    }
    speed++;
  }
  /* write results to pipe */
  FILE *f = fdopen(pipeno, "w");
  if (f == NULL) {
    perror("open pipe for writing failed.");
    exit(3);
  }
  fprintf(f, "%d %d %d\n", speed, failed, bytes);
  fclose(f);
  exit(0);
}

static void bench_father(const int pipeno, int clients, const int benchtime) {
  FILE *f = fdopen(pipeno, "r");
  if (f == NULL) {
    perror("open pipe for reading failed.");
    exit(3);
  }
  setvbuf(f, NULL, _IONBF, 0);
  int speed = 0;
  int failed = 0;
  int bytes = 0;
  int i, j, k;

  while (1) {
    if (fscanf(f, "%d %d %d", &i, &j, &k) < 2) {
      cout << "Some of our childrens died." << endl;
      break;
    }
    speed += i;
    failed += j;
    bytes += k;
    /* fprintf(stderr,"*Knock* %d %d read=%d\n",speed,failed,pid); */
    if (--clients == 0)
      break;
  }
  fclose(f);

  cout << "--------------result--------------" << endl;

  cout << "total: " << speed << " susceed, " << failed << " failed, " << bytes
       << " bytes." << endl;

  cout << "speed: " << (int)((speed + failed) / (benchtime / 60.0f))
       << " pages/min, " << (int)(bytes / (float)benchtime) << " bytes/sec."
       << endl;
}

void bench(const WebbenchOption &options) {
  const string &host = options.param_http_host();
  const int port = options.param_http_port();
  const int clients = options.param_http_clients();
  const int benchtime = options.param_http_benchtime();
  const string &http_version = options.param_http_version();
  bool http_force = options.param_http_force();
  const string &data = options.build_http_headers();
  /* check avaibility of target server */
  int sock_id = Socket(host, port);
  if (sock_id < 0) {
    cout << endl << "Connect to server failed. Aborting benchmark." << endl;
    exit(1);
  }
  close(sock_id);
  /* create pipe */
  int mypipe[2];
  if (pipe(mypipe)) {
    perror("pipe failed.");
    exit(3);
  }
  /* fork childs */
  pid_t pid = 0;
  int i = 0;
  for (i = 0; i < clients; i++) {
    pid = fork();
    // 子进程不循环
    if (pid <= (pid_t)0) {
      /* child process or error*/
      sleep(1); /* make childs faster */
      break;
    }
  }
  if (pid < (pid_t)0) {
    cout << "problems forking worker no." << i << endl;
    perror("fork failed.");
    exit(3);
  } else if (pid == (pid_t)0) {
    bench_child(mypipe[1], host, port, benchtime, data, http_version,
                http_force);
  } else {
    bench_father(mypipe[0], clients, benchtime);
  }
}
