#!/usr/bin/env bash

# Dependencies retrievable from apt on xenial and bionic
# libfmt-dev and nlohmann-json3-dev are only retrievable from eoan
udo apt-get install -y build-essential cmake curl p7zip-full libevent-dev libfreetype6-dev libirrlicht-dev liblua5.3-dev libsqlite3-dev libgl1-mesa-dev libglu-dev libgit2-dev libasound2