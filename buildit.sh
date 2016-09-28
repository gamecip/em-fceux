#!/bin/bash

rm -f src/drivers/em/site/fceux.{data,js,js.mem} && ./embuild.sh && cp src/drivers/em/site/fceux.{data,js,js.mem} ../cite-state/emulators