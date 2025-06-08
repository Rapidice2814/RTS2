# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Project root is the parent of the scripts folder
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Create and move to build directory inside project root
mkdir -p "$PROJECT_ROOT/build"
cd "$PROJECT_ROOT/build"

# Run cmake to configure the build system
cmake ..

if [ $? -ne 0 ]; then
    echo "CMake configuration failed."
    exit 1
fi

# Run make to build
make

if [ $? -ne 0 ]; then
    echo "Make failed."
    exit 1
fi

echo "Executing:"
echo "-------------------"
./RTS2