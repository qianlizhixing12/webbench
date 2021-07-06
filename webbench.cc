/*
 * Simple forking WWW Server benchmark
 */
#include "option.h"
#include "request.h"
#include <iostream>

using std::cout;
using std::endl;

/*
 * Return codes:
 *    0 - sucess
 *    1 - benchmark failed (server is not on-line)
 *    2 - bad param
 *    3 - internal error, fork failed
 */
int main(int argc, char *argv[]) {

  WebbenchOption options;
  options.param_option(argc, argv);
  bench(options);

  return 0;
}