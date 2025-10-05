setup:
	mkdir -p build && cd build
	conan profile detect --force
	conan install . -s build_type=Release --build=missing

release:
	cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE="$(shell pwd)/build/Release/generators/conan_toolchain.cmake" \
  -DCMAKE_PREFIX_PATH="$(shell pwd)/build/Release/generators" \
  -DCMAKE_BUILD_TYPE=Release
	cd build && make

deploy: release
	cp build/rawproc .
