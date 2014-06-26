find . -depth -type f -name "*.tiff" -exec sh -c 'convert "$1" "$(dirname "$1")/$(basename "$1" .tiff).png"' _ {} \;
find . -depth -type f -name "*.tiff" -exec sh -c 'convert "$1" "$(dirname "$1")/$(basename "$1" .tiff).png"' _ {} \;
