#!/bin/bash
astyle --style=kr --indent=force-tab=4 -n --lineend=linux --exclude=src/third_party -r "src/*.h"
astyle --style=kr --indent=force-tab=4 -n --lineend=linux --exclude=src/third_party -r "src/*.cpp"
