/*
 * Geometric calculations
 *
 *
 * Smallest enclosing circle algorithm is adapted from the Java implementation
 * by Project Nayuki and used under the provisions of the GPL.
 *
 * http://www.nayuki.io/page/smallest-enclosing-circle
 *
 *
 * Gift-wrapping algorithm is adapted from the Java implementation by Adam
 * Lärkeryd and used under the provisions of the GPLv2.
 *
 * https://code.google.com/p/convex-hull/
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

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
 * Returns a vector perpendicular to the given vector
 */
static struct point vector_perp(struct point p)
{
	return POINT(-p.y, p.x);
}

/*
 * Returns the dot product of two vectors
 */
static double vector_dot(struct point p, struct point q)
{
	return p.x * q.x + p.y * q.y;
}

/*
 * Handy two-dimensional "cross" product (or dot-perp)
 */
static double vector_cross(struct point p, struct point q)
{
	return vector_dot(vector_perp(p), q);
}

/*
 * Returns the sum of two vectors
 */
static struct point vector_add(struct point u, struct point v)
{
	return POINT(u.x + v.x, u.y + v.y);
}

/*
 * Returns the difference between two vectors
 */
static struct point vector_sub(struct point p, struct point q)
{
	return POINT(p.x - q.x, p.y - q.y);
}

/*
 * Multiplies a vector by a scalar
 */
static struct point vector_mul(struct point v, double s)
{
	return POINT(v.x * s, v.y * s);
}

/*
 * Divides a vector by a scalar
 */
static struct point vector_div(struct point v, double s)
{
	return POINT(v.x / s, v.y / s);
}

/*
 * Returns the square of the magnitude of a vector
 */
static double vector_norm2(struct point v)
{
	return vector_dot(v, v);
}

/*
 * Returns the magnitude of a vector (costly, use only when vector_norm2 is not
 * feasible)
 */
static double vector_norm(struct point v)
{
	return sqrt(vector_norm2(v));
}

/*
 * Returns the unit vector for a vector
 */
static struct point vector_unit(struct point v)
{
	if (v.x == 0 && v.y == 0)
		return POINT(1, 0);
	return vector_div(v, vector_norm(v));
}

/*
 * Returns the intersection point of the line passing through p in the
 * direction of r and the line passing through q in the direction of s
 *
 * Formula credited to Ronald Goldman here:
 *
 * http://stackoverflow.com/questions/563198
 */
static struct point vector_intersect(struct point p, struct point r,
		struct point q, struct point s)
{
	double t = vector_cross(vector_sub(q, p),
			vector_div(s, vector_cross(r, s)));
	return vector_add(p, vector_mul(r, t));
}

/*
 * Returns the square of the distance between two points
 */
static double point_distance2(struct point p, struct point q)
{
	return vector_norm2(vector_sub(q, p));
}

/*
 * Returns the distance between two points (costly, use only when
 * point_distance2 is not feasible)
 */
static double point_distance(struct point p, struct point q)
{
	return sqrt(point_distance2(p, q));
}

/*
 * Returns the area of the parallelogram defined by three points in
 * counterclockwise order
 */
static double points_par_area(struct point p, struct point q, struct point r)
{
	return vector_cross(p, q) + vector_cross(q, r) + vector_cross(r, p);
}

/*
 * Determines if a circle contains the given point
 */
static int circle_contains(const struct circle *c, struct point p)
{
	return point_distance2(c->c, p) <= c->r2;
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
	c->r2 = point_distance2(c->c, p);
}

/*
 * Returns the circumcircle of three points
 */
static void circle_circumscribe(struct point p, struct point q, struct point r,
		struct circle *c)
{
	double d = (vector_cross(p, q) + vector_cross(q, r) + vector_cross(r, p)) * 2;
	if (d == 0) {
		c->c.x = c->c.y = c->r2 = 0;
		return;
	}
	c->c.x = (vector_dot(p, p) * (q.y - r.y) +
			vector_dot(q, q) * (r.y - p.y) +
			vector_dot(r, r) * (p.y - q.y)) / d;
	c->c.y = (vector_dot(p, p) * (r.x - q.x) +
			vector_dot(q, q) * (p.x - r.x) +
			vector_dot(r, r) * (q.x - p.x)) / d;
	c->r2 = point_distance2(p, c->c);
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

	struct point pq = vector_sub(q, p);;

	temp.r2 = temp2.r2 = 0;
	for (i = 0; i < n; i++) {
		struct point pr = vector_sub(pts[i], p);;

		double cross = vector_cross(pq, pr);
		struct circle cc;
		circle_circumscribe(p, q, pts[i], &cc);
		if (cc.r2 == 0)
			continue;

		struct point pc = vector_sub(cc.c, p);

		if (cross > 0 && (temp.r2 == 0 || vector_cross(pq, pc) > vector_cross(pq, vector_sub(temp.c, p))))
			temp = cc;
		else if (cross < 0 && (temp2.r2 == 0 || vector_cross(pq, pc) < vector_cross(pq, vector_sub(temp2.c, p))))
			temp2 = cc;
	}
	if (temp2.r2 == 0)
		*c = temp;
	else if (temp.r2 == 0)
		*c = temp2;
	else if (temp.r2 <= temp2.r2)
		*c = temp;
	else
		*c = temp2;
}

static void circle_1point(const struct point *pts, int n, struct point p,
		struct circle *c)
{
	int i;
	c->c = p;
	c->r2 = 0;

	for (i = 0; i < n; i++) {
		if (circle_contains(c, pts[i]))
			continue;
		if (c->r2 == 0)
			circle_from_diameter(p, pts[i], c);
		else
			circle_2points(pts, i, p, pts[i], c);
	}
}

struct point points_enclosing_center(const struct point *pts, int n)
{
	assert(n >= 1);

	int i;
	struct circle c;

	// Skip the shuffle, assume it's random/small enough

	circle_1point(pts, 1, pts[0], &c);
	for (i = 1; i < n; i++) {
		if (!circle_contains(&c, pts[i]))
			circle_1point(pts, i, pts[i], &c);
	}

	return c.c;
}

static int points_compare_x(const void *v1, const void *v2)
{
	const struct point *p1 = v1, *p2 = v2;
	if (p1->x < p2->x)
		return -1;
	if (p1->x > p2->x)
		return 1;
	return 0;
}

static int left_turn(struct point p, struct point q, struct point r)
{
	return points_par_area(p, q, r) > 0;
}

/*
 * Calculates the convex hull of the given points, storing the points of the
 * hull in the hull array and returning the length of that array
 */
int points_convex_hull(const struct point *pts, int n, struct point *hull)
{
	if (n < 0)
		return -1;
	if (n == 0)
		return 0;
	if (n == 1) {
		hull[0] = pts[0];
		return 1;
	}

	struct point *xsorted = malloc(n * sizeof(xsorted[0]));
	assert(xsorted); //sloppy
	memcpy(xsorted, pts, n * sizeof(xsorted[0]));
	qsort(xsorted, n, sizeof(xsorted[0]), points_compare_x);

	struct point *llower = malloc(n * sizeof(llower[0]));
	assert(llower);
	llower[0] = xsorted[0];
	llower[1] = xsorted[1];

	int ui = 2;
	int i;
	for (i = 2; i < n; i++) {
		llower[ui++] = xsorted[i];
		while (ui > 2 && !left_turn(llower[ui - 3], llower[ui - 2], llower[ui - 1])) {
			// Remove the middle point of the three last
			llower[ui - 2] = llower[ui - 1];
			ui--;
		}
	}

	struct point *lupper = malloc(n * sizeof(lupper[0]));
	assert(lupper);
	lupper[0] = xsorted[n - 1];
	lupper[1] = xsorted[n - 2];

	int li = 2;

	for (i = n - 3; i >= 0; i--) {
		lupper[li++] = xsorted[i];
		while(li > 2 && !left_turn(lupper[li - 3], lupper[li - 2], lupper[li - 1])) {
			// Remove the middle point of the three last
			lupper[li - 2] = lupper[li - 1];
			li--;
		}
	}

	for (i = 0; i < ui; i++)
		hull[i] = llower[i];
	for (i = 0; i < li - 2; i++)
		hull[ui + i] = lupper[i + 1];

	free(lupper);
	free(llower);
	free(xsorted);

	return ui + li - 2;
}

/*
 * Calculates the minimum oriented bounding box of the given convex hull,
 * storing the points of the box in the rect array (length 4)
 *
 * The hull points must be provided in counterclockwise order.
 */
void points_oriented_bbox(const struct point *hull, int n, struct point *rect)
{
	int i;
	double min_area = -1;
	struct point temp[4];
	double area;

	if (n == 2) {
		rect[0] = rect[1] = hull[0];
		rect[2] = rect[3] = hull[1];
		return;
	}

	// Yay VLA, livin' dangerously
	struct point hullcal[n];

	// Start calipers on standard basis
	struct point caliper[4] = {POINT(1, 0), POINT(0, 1), POINT(-1, 0),
		POINT(0, -1)};

	// Find points on the hull which the initial calipers touch.  While
	// we're at it, calculate caliper vectors for each edge of the hull.
	int point[4];
	point[0] = point[1] = point[2] = point[3] = 0;
	for (i = 0; i < n; i++) {
		int last = (i + n - 1) % n;
		int next = (i + 1) % n;
		if (hull[i].y < hull[last].y && hull[i].y <= hull[next].y)
			point[0] = i;
		if (hull[i].x > hull[last].x && hull[i].x >= hull[next].x)
			point[1] = i;
		if (hull[i].y > hull[last].y && hull[i].y >= hull[next].y)
			point[2] = i;
		if (hull[i].x < hull[last].x && hull[i].x <= hull[next].x)
			point[3] = i;
		hullcal[i] = vector_unit(vector_sub(hull[next], hull[i]));
	}

	// With a rectangular set of calipers, we have to rotate through pi/2
	// radians at most, which is done when the X component of the caliper
	// that started as (1, 0) hits zero.
	while (caliper[0].x > 0) {
		// Find caliper with minimum angle to next hull edge - since the
		// angles are all between 0 and pi, this is the one with the
		// maximum cosine
		int cal = 0;
		double max_cos = -2;
		for (i = 0; i < 4; i++) {
			double cos_theta = vector_dot(caliper[i], hullcal[point[i]]);
			if (cos_theta > max_cos) {
				max_cos = cos_theta;
				cal = i;
			}
		}

		// Move current caliper to next point
		caliper[cal] = hullcal[point[cal]];
		point[cal]++;
		point[cal] %= n;

		// "Rotate" (actually just recalculate) remaining calipers
		for (i = 1; i < 4; i++)
			caliper[(cal + i) % 4] = vector_perp(caliper[(cal + i - 1) % 4]);

		// Calculate rectangle corners (intersections of calipers)
		for (i = 0; i < 4; i++) {
			int next = (i + 1) % 4;
			temp[i] = vector_intersect(hull[point[i]], caliper[i],
					hull[point[next]], caliper[next]);
		}

		// If it's the best so far, save it
		area = points_par_area(temp[0], temp[1], temp[2]);
		if (area < min_area || min_area < 0) {
			min_area = area;
			for (i = 0; i < 4; i++)
				rect[i] = temp[i];
		}
	}
}

/*
 * Calculates the area of the polygon defined by a list of points (given in
 * counterclockwise order)
 */
double polygon_area(const struct point *poly, int n)
{
	double area = 0.0;
	int i, last;

	for (i = 0, last = n - 1; i < n; last = i++)
		area += (poly[i].x + poly[last].x) * (poly[i].y - poly[last].y);
	return area / 2;
}
