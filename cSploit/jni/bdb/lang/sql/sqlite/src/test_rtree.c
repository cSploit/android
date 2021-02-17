/*
** 2010 August 28
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** Code for testing all sorts of SQLite interfaces. This code
** is not included in the SQLite library. 
*/

#include <sqlite3.h>

/* Solely for the UNUSED_PARAMETER() macro. */
#include "sqliteInt.h"

#ifdef SQLITE_ENABLE_RTREE
/* 
** Type used to cache parameter information for the "circle" r-tree geometry
** callback.
*/
typedef struct Circle Circle;
struct Circle {
  struct Box {
    double xmin;
    double xmax;
    double ymin;
    double ymax;
  } aBox[2];
  double centerx;
  double centery;
  double radius;
};

/*
** Destructor function for Circle objects allocated by circle_geom().
*/
static void circle_del(void *p){
  sqlite3_free(p);
}

/*
** Implementation of "circle" r-tree geometry callback.
*/
static int circle_geom(
  sqlite3_rtree_geometry *p,
  int nCoord, 
#ifdef SQLITE_RTREE_INT_ONLY
  sqlite3_int64 *aCoord,
#else
  double *aCoord, 
#endif
  int *pRes
){
  int i;                          /* Iterator variable */
  Circle *pCircle;                /* Structure defining circular region */
  double xmin, xmax;              /* X dimensions of box being tested */
  double ymin, ymax;              /* X dimensions of box being tested */

  if( p->pUser==0 ){
    /* If pUser is still 0, then the parameter values have not been tested
    ** for correctness or stored into a Circle structure yet. Do this now. */

    /* This geometry callback is for use with a 2-dimensional r-tree table.
    ** Return an error if the table does not have exactly 2 dimensions. */
    if( nCoord!=4 ) return SQLITE_ERROR;

    /* Test that the correct number of parameters (3) have been supplied,
    ** and that the parameters are in range (that the radius of the circle 
    ** radius is greater than zero). */
    if( p->nParam!=3 || p->aParam[2]<0.0 ) return SQLITE_ERROR;

    /* Allocate a structure to cache parameter data in. Return SQLITE_NOMEM
    ** if the allocation fails. */
    pCircle = (Circle *)(p->pUser = sqlite3_malloc(sizeof(Circle)));
    if( !pCircle ) return SQLITE_NOMEM;
    p->xDelUser = circle_del;

    /* Record the center and radius of the circular region. One way that
    ** tested bounding boxes that intersect the circular region are detected
    ** is by testing if each corner of the bounding box lies within radius
    ** units of the center of the circle. */
    pCircle->centerx = p->aParam[0];
    pCircle->centery = p->aParam[1];
    pCircle->radius = p->aParam[2];

    /* Define two bounding box regions. The first, aBox[0], extends to
    ** infinity in the X dimension. It covers the same range of the Y dimension
    ** as the circular region. The second, aBox[1], extends to infinity in
    ** the Y dimension and is constrained to the range of the circle in the
    ** X dimension.
    **
    ** Then imagine each box is split in half along its short axis by a line
    ** that intersects the center of the circular region. A bounding box
    ** being tested can be said to intersect the circular region if it contains
    ** points from each half of either of the two infinite bounding boxes.
    */
    pCircle->aBox[0].xmin = pCircle->centerx;
    pCircle->aBox[0].xmax = pCircle->centerx;
    pCircle->aBox[0].ymin = pCircle->centery + pCircle->radius;
    pCircle->aBox[0].ymax = pCircle->centery - pCircle->radius;
    pCircle->aBox[1].xmin = pCircle->centerx + pCircle->radius;
    pCircle->aBox[1].xmax = pCircle->centerx - pCircle->radius;
    pCircle->aBox[1].ymin = pCircle->centery;
    pCircle->aBox[1].ymax = pCircle->centery;
  }

  pCircle = (Circle *)p->pUser;
  xmin = aCoord[0];
  xmax = aCoord[1];
  ymin = aCoord[2];
  ymax = aCoord[3];

  /* Check if any of the 4 corners of the bounding-box being tested lie 
  ** inside the circular region. If they do, then the bounding-box does
  ** intersect the region of interest. Set the output variable to true and
  ** return SQLITE_OK in this case. */
  for(i=0; i<4; i++){
    double x = (i&0x01) ? xmax : xmin;
    double y = (i&0x02) ? ymax : ymin;
    double d2;
    
    d2  = (x-pCircle->centerx)*(x-pCircle->centerx);
    d2 += (y-pCircle->centery)*(y-pCircle->centery);
    if( d2<(pCircle->radius*pCircle->radius) ){
      *pRes = 1;
      return SQLITE_OK;
    }
  }

  /* Check if the bounding box covers any other part of the circular region.
  ** See comments above for a description of how this test works. If it does
  ** cover part of the circular region, set the output variable to true
  ** and return SQLITE_OK. */
  for(i=0; i<2; i++){
    if( xmin<=pCircle->aBox[i].xmin 
     && xmax>=pCircle->aBox[i].xmax 
     && ymin<=pCircle->aBox[i].ymin 
     && ymax>=pCircle->aBox[i].ymax 
    ){
      *pRes = 1;
      return SQLITE_OK;
    }
  }

  /* The specified bounding box does not intersect the circular region. Set
  ** the output variable to zero and return SQLITE_OK. */
  *pRes = 0;
  return SQLITE_OK;
}

/* END of implementation of "circle" geometry callback.
**************************************************************************
*************************************************************************/

#include <assert.h>
#include "tcl.h"

typedef struct Cube Cube;
struct Cube {
  double x;
  double y;
  double z;
  double width;
  double height;
  double depth;
};

static void cube_context_free(void *p){
  sqlite3_free(p);
}

/*
** The context pointer registered along with the 'cube' callback is
** always ((void *)&gHere). This is just to facilitate testing, it is not
** actually used for anything.
*/
static int gHere = 42;

/*
** Implementation of a simple r-tree geom callback to test for intersection
** of r-tree rows with a "cube" shape. Cubes are defined by six scalar
** coordinates as follows:
**
**   cube(x, y, z, width, height, depth)
**
** The width, height and depth parameters must all be greater than zero.
*/
static int cube_geom(
  sqlite3_rtree_geometry *p,
  int nCoord,
#ifdef SQLITE_RTREE_INT_ONLY
  sqlite3_int64 *aCoord, 
#else
  double *aCoord, 
#endif
  int *piRes
){
  Cube *pCube = (Cube *)p->pUser;

  assert( p->pContext==(void *)&gHere );

  if( pCube==0 ){
    if( p->nParam!=6 || nCoord!=6
     || p->aParam[3]<=0.0 || p->aParam[4]<=0.0 || p->aParam[5]<=0.0
    ){
      return SQLITE_ERROR;
    }
    pCube = (Cube *)sqlite3_malloc(sizeof(Cube));
    if( !pCube ){
      return SQLITE_NOMEM;
    }
    pCube->x = p->aParam[0];
    pCube->y = p->aParam[1];
    pCube->z = p->aParam[2];
    pCube->width = p->aParam[3];
    pCube->height = p->aParam[4];
    pCube->depth = p->aParam[5];

    p->pUser = (void *)pCube;
    p->xDelUser = cube_context_free;
  }

  assert( nCoord==6 );
  *piRes = 0;
  if( aCoord[0]<=(pCube->x+pCube->width)
   && aCoord[1]>=pCube->x
   && aCoord[2]<=(pCube->y+pCube->height)
   && aCoord[3]>=pCube->y
   && aCoord[4]<=(pCube->z+pCube->depth)
   && aCoord[5]>=pCube->z
  ){
    *piRes = 1;
  }

  return SQLITE_OK;
}
#endif /* SQLITE_ENABLE_RTREE */

static int register_cube_geom(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
#ifndef SQLITE_ENABLE_RTREE
  UNUSED_PARAMETER(clientData);
  UNUSED_PARAMETER(interp);
  UNUSED_PARAMETER(objc);
  UNUSED_PARAMETER(objv);
#else
  extern int getDbPointer(Tcl_Interp*, const char*, sqlite3**);
  extern const char *sqlite3TestErrorName(int);
  sqlite3 *db;
  int rc;

  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "DB");
    return TCL_ERROR;
  }
  if( getDbPointer(interp, Tcl_GetString(objv[1]), &db) ) return TCL_ERROR;
  rc = sqlite3_rtree_geometry_callback(db, "cube", cube_geom, (void *)&gHere);
  Tcl_SetResult(interp, (char *)sqlite3TestErrorName(rc), TCL_STATIC);
#endif
  return TCL_OK;
}

static int register_circle_geom(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
#ifndef SQLITE_ENABLE_RTREE
  UNUSED_PARAMETER(clientData);
  UNUSED_PARAMETER(interp);
  UNUSED_PARAMETER(objc);
  UNUSED_PARAMETER(objv);
#else
  extern int getDbPointer(Tcl_Interp*, const char*, sqlite3**);
  extern const char *sqlite3TestErrorName(int);
  sqlite3 *db;
  int rc;

  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "DB");
    return TCL_ERROR;
  }
  if( getDbPointer(interp, Tcl_GetString(objv[1]), &db) ) return TCL_ERROR;
  rc = sqlite3_rtree_geometry_callback(db, "circle", circle_geom, 0);
  Tcl_SetResult(interp, (char *)sqlite3TestErrorName(rc), TCL_STATIC);
#endif
  return TCL_OK;
}

int Sqlitetestrtree_Init(Tcl_Interp *interp){
  Tcl_CreateObjCommand(interp, "register_cube_geom", register_cube_geom, 0, 0);
  Tcl_CreateObjCommand(interp, "register_circle_geom",register_circle_geom,0,0);
  return TCL_OK;
}
