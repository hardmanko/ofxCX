

/* Example use of getSepDist
std::vector<ofPoint> getObjectLocations(int locCount, double minDistance) {
auto dist = [](ofPoint a, ofPoint b) {
return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
};

auto rand = []() {
ofPoint rval;
rval.x = randomInt(100, 500);
rval.y = randomInt(100, 500);
return rval;
};

return getSepDist<ofPoint>(locCount, minDistance, dist, rand, 1000);
}
*/