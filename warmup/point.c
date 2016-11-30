#include <assert.h>
#include <math.h>
#include "common.h"
#include "point.h"

void
point_translate(struct point *p, double x, double y)
{
  p->x = p->x + x;
  p->y = p->y + y;

  //TBD();
}

double
point_distance(const struct point *p1, const struct point *p2)
{
  //TBD();
  double x_distance = p1->x - p2->x;
  double y_distance = p1->y - p2->y;

  double total_distance = x_distance * x_distance + y_distance * y_distance;
  
  return sqrt(total_distance);
  //return -1.0;
}

int
point_compare(const struct point *p1, const struct point *p2)
{
  double p1_distance = sqrt(p1->x * p1->x + p1->y * p1->y);
  double p2_distance = sqrt(p2->x * p2->x + p2->y * p2->y);

  if (p2_distance == p1_distance){
    return 0;
  }
  else if (p2_distance > p1_distance){
    return -1;
  }
  else {
    return 1;
  }
  return 0;
}
