#include "objects/Path.h"
#include "objects/Ground.h"

#include <fmtlog/fmtlog.h>

Path::Path(std::string file) {
	// Path masks are data, not GL textures: keep the historical QImage
	// top-left origin (no vertical flip).
	image = Image(file, false);
	// init the size of the loaded picture
	width = image.getWidth();
	height = image.getHeight();
}

// compute if the position x, z is contained in the path.
bool Path::validate(double x, double z) const {
	float range = Ground::MAP_MAX - Ground::MAP_MIN;
	const float offset = -Ground::MAP_MIN;
	float xr = (x + offset) / range;
	float yr = (z + offset) / range;
	// Pixel indices: truncating a non-negative ratio is exact flooring;
	// negatives fall through to the bounds check below.
	int xp = static_cast<int>(xr * width);
	int yp = static_cast<int>(yr * height);

	if (xp < 0 || xp >= width || yp < 0 || yp >= height) {
		return false;
	}
	else if (image.getPixel(xp, yp).r > 0) {
		return false;
	}
	else {
		return true;
	}
}
