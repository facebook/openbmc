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

  GDBusProxy* proxy = g_dbus_proxy_new_for_bus_sync(
                        G_BUS_TYPE_SESSION,
                        G_DBUS_PROXY_FLAGS_NONE,
                        NULL,
                        "org.openbmc.TestServer",
                        "/org/openbmc/test",
                        "org.openbmc.TestInterface",
                        NULL,
                        &error);

  if (proxy == NULL) {
    printf("Cannot register the dbus proxy: %s", error->message);
    return 1;
  }

  int iteration = 1000;
  double avgtime = 0;
  double var = 0;
  int i = 0;
  for (i = 0; i < iteration; i++) {
    struct timeval tv1, tv2;
    GVariant* param = g_variant_new("(s)", "greetings");

    gettimeofday(&tv1, NULL);

    g_dbus_proxy_call_sync(
        proxy,
        "org.openbmc.TestInterface.HelloWorld",
        param,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);

    gettimeofday(&tv2, NULL);

    double diff = (double) (1000000*(tv2.tv_sec - tv1.tv_sec)) +
                  tv2.tv_usec - tv1.tv_usec;
    avgtime += diff;
    var += diff*diff;

    if (error != NULL) {
      printf("Cannot send message to the proxy: %s", error->message);
      return 1;
    }
  }
  avgtime /= iteration;
  double std = var/iteration - avgtime*avgtime;
  std = sqrt(std);
  printf("average latency = %f\nlatency std = %f\n", avgtime, std);
  return 0;
}
