if [[ "$(realpath /proc/self/fd/0)" == *"/pts/"* ]]; then
    export TMOUT=__SSH_TMOUT__
fi
