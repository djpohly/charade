#ifndef GEOMETRY_H_
#define GEOMETRY_H_

struct point {
	double x, y;
};

struct circle {
	struct point c;
	double r;
};

void points_centroid(const struct point *pts, int n, struct point *c);
void points_bbox_center(const struct point *pts, int n, struct point *c);

#endif
