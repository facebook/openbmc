#include <gio/gio.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

// measure the latency of dbus
int main (int argc, char *argv[]) {
  GSubprocess *subprocess = NULL;
  GError *error = NULL;
  gchar *response;

  int iteration = 1000;
  double avgtime = 0;
  double var = 0;
  int i = 0;
  for (i = 0; i < iteration; i++) {
    struct timeval tv1, tv2;
    gettimeofday(&tv1, NULL);
    // fork a subprocess to execute the shell command
    subprocess = g_subprocess_new(G_SUBPROCESS_FLAGS_STDOUT_PIPE,
                                  &error,
                                  "dbus-send",
                                  "--session",
                                  "--type=method_call",
                                  "--dest=org.openbmc.TestServer",
                                  "/org/openbmc/test",
                                  "org.openbmc.TestInterface.HelloWorld",
                                  "string:'greeting'",
                                  NULL);

    g_subprocess_communicate(subprocess,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             &error);

    g_subprocess_wait(subprocess, NULL, &error);
    gettimeofday(&tv2, NULL);
    double diff = (double) (1000000*(tv2.tv_sec - tv1.tv_sec)) + tv2.tv_usec - tv1.tv_usec;
    avgtime += diff;
    var += diff*diff;
  }
  avgtime /= iteration;
  double std = var/iteration - avgtime*avgtime;
  std = sqrt(std);
  printf("average latency = %f\nlatency std = %f\n", avgtime, std);
  return 0;
}
