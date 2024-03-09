#!/bin/bash

cur_dir=$(pwd)

base_dir="$cur_dir/../.."
groovy_dir="$base_dir/lib/groovy"
generator_dir="$base_dir/tools/codegenerator"
bin_dir="$cur_dir/../BuildDependencies/bin"

# go into xbmc/interfaces/python
cd "$base_dir/xbmc/interfaces/python" || exit

python_dir=$(pwd)
python_generated_dir="$python_dir/generated"
doxygen_dir="$python_generated_dir/doxygenxml"
swig_dir="$python_dir/../swig"

# make sure all necessary directories exist and delete any old generated files
rm -rf "$python_generated_dir"
mkdir -p "$python_generated_dir"
mkdir -p "$doxygen_dir"

# run doxygen
doxygen > /dev/null 2>&1

for file in "$swig_dir"/*.i; do
  # Get the file name without the extension
  filename=$(basename "$file" .i)

  # run swig to generate the XML used by groovy to generate the python bindings
  swig -w401 -c++ -outdir "$python_generated_dir" -o "$python_generated_dir/$filename.xml" -xml -I"$base_dir/xbmc" -xmllang python "$swig_dir/$filename.i"

  # run groovy to generate the python bindings
  java -cp "$groovy_dir/groovy-all-2.1.7.jar:$groovy_dir/commons-lang-2.6.jar:$generator_dir:$python_dir" groovy.ui.GroovyMain "$generator_dir/Generator.groovy" "$python_generated_dir/$filename.xml" "$python_dir/PythonSwig.cpp.template"  "$python_generated_dir/$filename.cpp" "$doxygen_dir"

  # go back to the initial directory
  cd "$cur_dir" || exit
done
