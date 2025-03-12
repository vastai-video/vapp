#ifndef VAST_DETECT_H
#define VAST_DETECT_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "vapp.h"
#include "misc_params.h"

#define KSIZE (3)
#define BLOCK_SIZE (16)
#define BLOCK_SIZE_EXTEND (32)
#define HIST_BIN_SIZE (8)
#define HIST_RANGE (256)
#define MAX_BIN_SIZE (3)
#define QP_OFFSET_STATIC (-5)
#define QP_OFFSET_TEXT (-2)
#define QP_OFFSET_STATIC_TEXT (-7)
#define HIST_PERCENTAGE_TH (0.65)
#define MORPHOLOGICAL_TH (0.5)
#define LAPLACIAN_TH (2000)
#define GRADIENT_TH (10)
#define STATIC_AND_TEXT (1)
#define SRC_BIT_DEPTH (8)

#define XV_ARRAY_FIELDS \
  void    *pBuffer;    \
  Vapp32u bufferSize;  \
  void    *pData;       \
  Vapp32s width;        \
  Vapp32s pitch;        \
  Vapp32u status;      \
  Vapp16u type;        \
  Vapp16u height;

typedef struct xvArrayStruct
{
	XV_ARRAY_FIELDS
} xvArray, * xvpArray;


typedef struct xvTileStruct
{
	XV_ARRAY_FIELDS	
	Vapp32s             x;
	Vapp32s             y;
	Vapp16u            tileEdgeLeft;
	Vapp16u            tileEdgeTop;
	Vapp16u            tileEdgeRight;
	Vapp16u            tileEdgeBottom;
	Vapp32s             dmaIndex;
	Vapp32s             reuseCount;
	struct xvTileStruct* pPrevTile;
} xvTile, * xvpTile;

typedef struct roi_map_ptr
{
	Vapp8u *roi_map_base;
	Vapp8u *roi_map_extend;
	Vapp8u *roi_map_morph;
	Vapp8u *roi_map_static;
} roi_map_ptr_t;

#define XV_ARRAY_GET_BUFF_PTR(pArray)              ((pArray)->pBuffer)
#define XV_ARRAY_SET_BUFF_PTR(pArray, pBuff)       (pArray)->pBuffer = ((void *) (pBuff))

#define XV_ARRAY_GET_BUFF_SIZE(pArray)             ((pArray)->bufferSize)
#define XV_ARRAY_SET_BUFF_SIZE(pArray, buffSize)   (pArray)->bufferSize = ((Vapp32u) (buffSize))

#define XV_ARRAY_GET_DATA_PTR(pArray)              ((pArray)->pData)
#define XV_ARRAY_SET_DATA_PTR(pArray, pArrayData)  (pArray)->pData = ((void *) (pArrayData))

#define XV_ARRAY_GET_WIDTH(pArray)                 ((pArray)->width)
#define XV_ARRAY_SET_WIDTH(pArray, value)          (pArray)->width = ((Vapp32s) (value))

#define XV_ARRAY_GET_PITCH(pArray)                 ((pArray)->pitch)
#define XV_ARRAY_SET_PITCH(pArray, value)          (pArray)->pitch = ((Vapp32s) (value))

#define XV_ARRAY_GET_HEIGHT(pArray)                ((pArray)->height)
#define XV_ARRAY_SET_HEIGHT(pArray, value)         (pArray)->height = ((Vapp16u) (value))

#define XV_ARRAY_GET_STATUS_FLAGS(pArray)          ((pArray)->status)
#define XV_ARRAY_SET_STATUS_FLAGS(pArray, value)   (pArray)->status = ((Vapp8u) (value))

#define XV_ARRAY_GET_TYPE(pArray)                  ((pArray)->type)
#define XV_ARRAY_SET_TYPE(pArray, value)           (pArray)->type = ((Vapp16u) (value))

#define XV_ARRAY_GET_CAPACITY(pArray)              XV_ARRAY_GET_PITCH(pArray)
#define XV_ARRAY_SET_CAPACITY(pArray, value)       XV_ARRAY_SET_PITCH((pArray), (value))

#define XV_ARRAY_GET_ELEMENT_TYPE(pArray)          XV_TYPE_ELEMENT_TYPE(XV_ARRAY_GET_TYPE(pArray))
#define XV_ARRAY_GET_ELEMENT_SIZE(pArray)          XV_TYPE_ELEMENT_SIZE(XV_ARRAY_GET_TYPE(pArray))
#define XV_ARRAY_IS_TILE(pArray)                   XV_TYPE_IS_TILE(XV_ARRAY_GET_TYPE(pArray) & (XV_TYPE_TILE_BIT))

#define XV_ARRAY_GET_AREA(pArray)                  (((pArray)->width) * ((Vapp32s) (pArray)->height))

/*****************************************
*    Tile Access Macros
*****************************************/

#define XV_TILE_GET_BUFF_PTR   XV_ARRAY_GET_BUFF_PTR
#define XV_TILE_SET_BUFF_PTR   XV_ARRAY_SET_BUFF_PTR

#define XV_TILE_GET_BUFF_SIZE  XV_ARRAY_GET_BUFF_SIZE
#define XV_TILE_SET_BUFF_SIZE  XV_ARRAY_SET_BUFF_SIZE

#define XV_TILE_GET_DATA_PTR   XV_ARRAY_GET_DATA_PTR
#define XV_TILE_SET_DATA_PTR   XV_ARRAY_SET_DATA_PTR

#define XV_TILE_GET_WIDTH      XV_ARRAY_GET_WIDTH
#define XV_TILE_SET_WIDTH      XV_ARRAY_SET_WIDTH

#define XV_TILE_GET_PITCH      XV_ARRAY_GET_PITCH
#define XV_TILE_SET_PITCH      XV_ARRAY_SET_PITCH
#define XV_TILE_GET_PITCH_IN_BYTES(pTile)  ((pTile)->pitch * (Vapp32s) ((pTile)->pFrame->pixelRes))

#define XV_TILE_GET_HEIGHT        XV_ARRAY_GET_HEIGHT
#define XV_TILE_SET_HEIGHT        XV_ARRAY_SET_HEIGHT

#define XV_TILE_GET_STATUS_FLAGS  XV_ARRAY_GET_STATUS_FLAGS
#define XV_TILE_SET_STATUS_FLAGS  XV_ARRAY_SET_STATUS_FLAGS

#define XV_TILE_GET_TYPE          XV_ARRAY_GET_TYPE
#define XV_TILE_SET_TYPE          XV_ARRAY_SET_TYPE

#define XV_TILE_GET_ELEMENT_TYPE  XV_ARRAY_GET_ELEMENT_TYPE
#define XV_TILE_GET_ELEMENT_SIZE  XV_ARRAY_GET_ELEMENT_SIZE
#define XV_TILE_IS_TILE           XV_ARRAY_IS_TILE

#define XV_TILE_RESET_DMA_INDEX(pTile)              ((pTile)->dmaIndex = 0)
#define XV_TILE_RESET_PREVIOUS_TILE(pTile)          (pTile)->pPrevTile = ((xvTile *) (NULL))
#define XV_TILE_RESET_REUSE_COUNT(pTile)            ((pTile)->reuseCount = 0)

#define XV_TILE_GET_FRAME_PTR(pTile)                ((pTile)->pFrame)
#define XV_TILE_SET_FRAME_PTR(pTile, ptrFrame)      (pTile)->pFrame = ((xvFrame *) (ptrFrame))

#define XV_TILE_GET_X_COORD(pTile)                  ((pTile)->x)
#define XV_TILE_SET_X_COORD(pTile, xcoord)          (pTile)->x = ((Vapp32s) (xcoord))

#define XV_TILE_GET_Y_COORD(pTile)                  ((pTile)->y)
#define XV_TILE_SET_Y_COORD(pTile, ycoord)          (pTile)->y = ((Vapp32s) (ycoord))

#define XV_TILE_GET_EDGE_LEFT(pTile)                ((pTile)->tileEdgeLeft)
#define XV_TILE_SET_EDGE_LEFT(pTile, edgeWidth)     (pTile)->tileEdgeLeft = ((Vapp16u) (edgeWidth))

#define XV_TILE_GET_EDGE_RIGHT(pTile)               ((pTile)->tileEdgeRight)
#define XV_TILE_SET_EDGE_RIGHT(pTile, edgeWidth)    (pTile)->tileEdgeRight = ((Vapp16u) (edgeWidth))

#define XV_TILE_GET_EDGE_TOP(pTile)                 ((pTile)->tileEdgeTop)
#define XV_TILE_SET_EDGE_TOP(pTile, edgeHeight)     (pTile)->tileEdgeTop = ((Vapp16u) (edgeHeight))

#define XV_TILE_GET_EDGE_BOTTOM(pTile)              ((pTile)->tileEdgeBottom)
#define XV_TILE_SET_EDGE_BOTTOM(pTile, edgeHeight)  (pTile)->tileEdgeBottom = ((Vapp16u) (edgeHeight))

#define XV_TILE_GET_EDGE_WIDTH(pTile)               (((pTile)->tileEdgeLeft < (pTile)->tileEdgeRight) ? (pTile)->tileEdgeLeft : (pTile)->tileEdgeRight)
#define XV_TILE_SET_EDGE_WIDTH(pTile, edgeWidth)       \
  {                                                    \
    (pTile)->tileEdgeLeft  = ((Vapp16u) (edgeWidth)); \
    (pTile)->tileEdgeRight = ((Vapp16u) (edgeWidth)); \
  }

#define XV_TILE_GET_EDGE_HEIGHT(pTile)  (((pTile)->tileEdgeTop < (pTile)->tileEdgeBottom) ? (pTile)->tileEdgeTop : (pTile)->tileEdgeBottom)
#define XV_TILE_SET_EDGE_HEIGHT(pTile, edgeHeight)       \
  {                                                      \
    (pTile)->tileEdgeTop    = ((Vapp16u) (edgeHeight)); \
    (pTile)->tileEdgeBottom = ((Vapp16u) (edgeHeight)); \
  }

#define XV_TILE_CHECK_STATUS_FLAGS_DMA_ONGOING(pTile)          (((pTile)->status & XV_TILE_STATUS_DMA_ONGOING) > 0)
#define XV_TILE_CHECK_STATUS_FLAGS_EDGE_PADDING_NEEDED(pTile)  (((pTile)->status & XV_TILE_STATUS_EDGE_PADDING_NEEDED) > 0)


#define XV_TYPE_SIGNED_BIT         (1 << 15)
#define XV_TYPE_TILE_BIT           (1 << 14)

#define XVTM_MIN(a, b)  (((a) < (b)) ? (a) : (b))


VappStatus run_detection_test(VappiTextDetectionBuffers *buffers, dection_para_t *dect_param);

#endif