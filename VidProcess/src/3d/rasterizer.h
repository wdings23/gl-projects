#ifndef __RASTERIZER_H__
#define __RASTERIZER_H__

#include "model.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

bool checkZBuffer( tVector4 const* pTopLeftFront, 
				   tVector4 const* pBottomRightBack,
				   tMatrix44 const* pViewProjMatrix,
                   float* afDepthBuffer,
				   unsigned char* acBuffer, 
				   int iBufferWidth, 
				   int iBufferHeight );

void rasterizeTri( float* afDepthBuffer,
                   unsigned char* acBuffer, 
                   int iBufferWidth, 
                   int iBufferHeight,
                   tVector2i* pScreenCoord0,
                   tVector2i* pScreenCoord1,
                   tVector2i* pScreenCoord2,
                   float fZCoord0,
                   float fZCoord1,
                   float fZCoord2,
                   tVector2* aUV = NULL,
                   unsigned char const* acTexture = NULL,
                   int iTextureWidth = 0,
                   int iTextureHeight = 0 );
    
void rasterizeClipTri( float* afDepthBuffer,
                      unsigned char* acBuffer, 
                      int iBufferWidth, 
                      int iBufferHeight,
                      tVector4* aVerts,
                      float fNearZ,
                      tVector2* aUV = NULL,
                      unsigned char const* acTexture = NULL,
                      int iTextureWidth = 0,
                      int iTextureHeight = 0 );
    
#if defined( PROFILE )
    void clearProfileTimes( void );    
#endif // PROFILE
    
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __RASTERIZER_H__