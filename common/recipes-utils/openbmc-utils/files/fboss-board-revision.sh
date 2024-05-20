#!/bin/sh

#shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

if [ -z "$1" ]; then
  set -- --help
fi

while [ "$1" ]; do
  case "$1" in
    --help)
      echo "Prints board revision information."
      echo "Usage: fboss-board-revision.sh [-r] [-s]"
      echo "Multiple options can be given, each will print the result in order"
      echo "Options:"
      echo "  -r: Prints 1 if respin, 0 if not, 'unknown' if unknown"
      echo "  -s: Prints board type/revision as a human-readable string"
      ;;
    -r)
      if type board_rev_is_respin >/dev/null; then
        board_rev_is_respin && echo "1" || echo "0"
      else
        echo "unknown"
      fi
      ;;
    -s)
      if type wedge_board_type_rev >/dev/null; then
        wedge_board_type_rev
      else
        echo "unknown"
      fi
      ;;
    *)
      echo "UNKNOWN OPTION"
      ;;
  esac
  shift
done

exit 0
