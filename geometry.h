#ifndef GEOMETRY_H_
#define GEOMETRY_H_

struct point {
	double x, y;
};

struct circle {
	struct point c;
	double r;
};

struct point points_centroid(const struct point *pts, int n);
struct point points_bbox_center(const struct point *pts, int n);
struct point points_enclosing_center(const struct point *pts, int n);

int points_convex_hull(const struct point *pts, int n, struct point *hull);
void points_oriented_bbox(const struct point *hull, int n, struct point *rect);

#endif
