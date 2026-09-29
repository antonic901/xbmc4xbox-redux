#!/bin/sh
set -eu

# Resolve the repository root so this script can run from any directory.
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$script_dir/../.."

output_dir=xbmc/interfaces/python/generated
mkdir -p "$output_dir"

for input in xbmc/interfaces/swig/*.i; do
  module=${input##*/}
  echo "Generating $module.cpp"

  swig -w401 -c++ -o "$output_dir/$module.xml" -xml -Ixbmc "$input"
  java -cp "tools/codegenerator/groovy/*:tools/codegenerator:xbmc/interfaces/python" \
    groovy.ui.GroovyMain tools/codegenerator/Generator.groovy \
    "$output_dir/$module.xml" xbmc/interfaces/python/PythonSwig.cpp.template \
    "$output_dir/$module.cpp"
done
