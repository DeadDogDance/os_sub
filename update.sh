#!/bin/bash


case "$1" in

  "pull")
    echo "Pulling..."
    git pull -v origin main
    ;;

  "fetch")
    echo "Fetching..."
    git fetch -v
    ;;
  *)
    echo "No choice provided. Fetching and pulling..."
    git fetch -v
    git pull -v origin main
    ;;
esac
