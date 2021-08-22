#!/bin/bash

# Read FIFO argument from CLI
FIFO=${1}
shift

if [ ! -e "${FIFO}" ]; then
  echo "Fifo file ${FIFO} does not exist, cannot run"
  exit 1
fi

# Start kdialog with all other arguments and get dbus reference
dbusRef=$(kdialog "$@" 100)

if [ $? -ne 0 ] || [ -z "${dbusRef}" ]; then
  echo "Failed to start kdialog"
  exit 1
fi

# Setup cancellation point for this script
quitfn() {
    qdbus ${dbusRef} close 2>/dev/null
    exit 0
}

trap quitfn SIGINT
trap quitfn SIGTERM

# Read Fifo for new values or a quit message
while read -t 5 line < "${FIFO}"; do
  if [ $? != 0 ]; then
    echo "Timed out, closing"
    break
  fi
  if echo "${line}" | grep -q "idle"; then
    continue
  fi
  if echo "${line}" | grep -q "quit"; then
    break
  fi
  if ! qdbus ${dbusRef} Set "" value "${line}"; then
    break
  fi
done

# Cleanup
rm -f "${FIFO}"
quitfn
