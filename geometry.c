/*
 * Geometric calculations
 *
 * Smallest enclosing circle algorithm is adapted from the Java implementation
 * by Project Nayuki and used under the provisions of the GPL.
 *
 * http://www.nayuki.io/page/smallest-enclosing-circle
 */

#include <math.h>

#include "geometry.h"

/*
 * Returns the centroid of the given points
 */
void points_centroid(const struct point *pts, int n, struct point *c)
{
	int i;
	double tx, ty;

	tx = ty = 0;
	for (i = 0; i < n; i++) {
		tx += pts[i].x;
		ty += pts[i].y;
	}
	c->x = tx / n;
	c->y = ty / n;
}

/*
 * Returns the center of the bounding box of the given points
 */
void points_bbox_center(const struct point *pts, int n, struct point *c)
{
	int i;
	struct point min, max;

	if (n < 1)
		return;

	min = max = pts[0];
	for (i = 1; i < n; i++) {
		if (pts[i].x < min.x)
			min.x = pts[i].x;
		if (pts[i].x > max.x)
			max.x = pts[i].x;
		if (pts[i].y < min.y)
			min.y = pts[i].y;
		if (pts[i].y > max.y)
			max.y = pts[i].y;
	}

	c->x = (min.x + max.x) / 2;
	c->y = (min.y + max.y) / 2;
}

/*
 * Returns the norm of a point as a vector
 */
static double point_norm(const struct point *p)
{
	return p->x*p->x + p->y*p->y;
}

/*
 * Returns the distance between two points
 */
static double point_distance(const struct point *p, const struct point *q)
{
	struct point d;
	d.x = p->x - q->x;
	d.y = p->y - q->y;
	return sqrt(point_norm(&d));
}

/*
 * Determines if a circle contains the given point
 */
static int circle_contains(const struct circle *c, const struct point *p)
{
	return point_distance(&c->c, p) <= c->r;
}

/*
 * Returns the circle defined by a given diameter
 */
static void circle_from_diameter(const struct point *p, const struct point *q,
		struct circle *c)
{
	c->c.x = (p->x + q->x) / 2;
	c->c.y = (p->y + q->y) / 2;
	c->r = point_distance(p, q) / 2;
}

/*
 * Returns the circumcircle of three points
 */
static void circle_circumscribe(const struct point *p, const struct point *q,
		const struct point *r, struct circle *c)
{
	double d = (p->x * (q->y - r->y) + q->x * (r->y - p->y) + r->x * (p->y - q->y)) * 2;
	if (d == 0) {
		c->c.x = c->c.y = c->r = 0;
		return;
	}
	c->c.x = (point_norm(p) * (q->y - r->y) +
			point_norm(q) * (r->y - p->y) +
			point_norm(r) * (p->y - q->y)) / d;
	c->c.y = (point_norm(p) * (r->x - q->x) +
			point_norm(q) * (p->x - r->x) +
			point_norm(r) * (q->x - p->x)) / d;
	c->r = point_distance(p, &c->c);
}
