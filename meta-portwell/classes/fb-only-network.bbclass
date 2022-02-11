# Prevent bitbake from downloading anything over the network.
# This is done via the builtin BB_ALLOWED_NETWORKS setting.
BB_ALLOWED_NETWORKS := "*.facebook.com *.facebook.net"
