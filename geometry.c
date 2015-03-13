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
struct point points_centroid(const struct point *pts, int n)
{
	int i;
	double tx, ty;
	struct point c;

	tx = ty = 0;
	for (i = 0; i < n; i++) {
		tx += pts[i].x;
		ty += pts[i].y;
	}
	c.x = tx / n;
	c.y = ty / n;
	return c;
}

/*
 * Returns the center of the bounding box of the given points
 */
struct point points_bbox_center(const struct point *pts, int n)
{
	int i;
	struct point min, max, c;

	if (n < 1) {
		c.x = c.y = 0;
		return c;
	}

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

	c.x = (min.x + max.x) / 2;
	c.y = (min.y + max.y) / 2;
	return c;
}

/*
 * Returns the norm of a point as a vector
 */
static double point_norm(struct point p)
{
	return p.x * p.x + p.y * p.y;
}

/*
 * According to the original code, "signed area/determinant thing"; let's just
 * stick with that
 */
static double point_cross(struct point p, struct point q)
{
	return p.x * q.y - q.x * p.y;
}

/*
 * Subtracts two points
 */
static struct point point_sub(struct point p, struct point q)
{
	struct point d;
	d.x = p.x - q.x;
	d.y = p.y - q.y;
	return d;
}

/*
 * Returns the distance between two points
 */
static double point_distance(struct point p, struct point q)
{
	return sqrt(point_norm(point_sub(q, p)));
}

/*
 * Determines if a circle contains the given point
 */
static int circle_contains(const struct circle *c, struct point p)
{
	return point_distance(c->c, p) <= c->r;
}

/*
 * Determines if a circle contains all of the given points
 */
static int circle_contains_all(const struct circle *c, const struct point *pts,
		int n)
{
	int i;
	for (i = 0; i < n; i++)
		if (!circle_contains(c, pts[i]))
			return 0;
	return 1;
}

/*
 * Returns the circle defined by a given diameter
 */
static void circle_from_diameter(struct point p, struct point q,
		struct circle *c)
{
	c->c.x = (p.x + q.x) / 2;
	c->c.y = (p.y + q.y) / 2;
	c->r = point_distance(p, q) / 2;
}

/*
 * Returns the circumcircle of three points
 */
static void circle_circumscribe(struct point p, struct point q, struct point r,
		struct circle *c)
{
	//double d = (p.x * (q.y - r.y) + q.x * (r.y - p.y) + r.x * (p.y - q.y)) * 2;
	double d = (point_cross(p, q) + point_cross(q, r) + point_cross(r, p)) / 2;
	if (d == 0) {
		c->c.x = c->c.y = c->r = 0;
		return;
	}
	c->c.x = (point_norm(p) * (q.y - r.y) +
			point_norm(q) * (r.y - p.y) +
			point_norm(r) * (p.y - q.y)) / d;
	c->c.y = (point_norm(p) * (r.x - q.x) +
			point_norm(q) * (p.x - r.x) +
			point_norm(r) * (q.x - p.x)) / d;
	c->r = point_distance(p, c->c);
}

static void circle_2points(const struct point *pts, int n,
		struct point p, struct point q, struct circle *c)
{
	int i;
	struct circle temp, temp2;

	// If these two points are enough, the search is done
	circle_from_diameter(p, q, &temp);
	if (circle_contains_all(&temp, pts, n)) {
		*c = temp;
		return;
	}

	struct point pq = point_sub(q, p);;

	temp.r = temp2.r = 0;
	for (i = 0; i < n; i++) {
		struct point pr = point_sub(pts[i], p);;

		double cross = point_cross(pq, pr);
		struct circle cc;
		circle_circumscribe(p, q, pts[i], &cc);
		if (cc.r == 0)
			continue;

		struct point pc = point_sub(cc.c, p);
		struct point p1 = point_sub(temp.c, p);
		struct point p2 = point_sub(temp2.c, p);;

		if (cross > 0 && (temp.r == 0 || point_cross(pq, pc) > point_cross(pq, p1)))
			temp = cc;
		else if (cross < 0 && (temp2.r == 0 || point_cross(pq, pc) < point_cross(pq, p2)))
			temp2 = cc;
	}
	if (temp2.r == 0)
		*c = temp;
	else if (temp.r == 0)
		*c = temp2;
	else if (temp.r <= temp2.r)
		*c = temp;
	else
		*c = temp2;
}
