if [[ "$(realpath /proc/self/fd/0)" == *"ttyS"* ]]; then
    export TMOUT=__TMOUT__
fi
