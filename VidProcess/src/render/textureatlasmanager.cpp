//
//  textureatlasmanager.cpp
//  Game2
//
//  Created by Dingwings on 6/14/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "textureatlasmanager.h"
#include "hashutil.h"
#include "filepathutil.h"
#include "tinyxml.h"
#include "parseutil.h"

#define ATLAS_PER_ALLOC     100

/*
**
*/
CTextureAtlasManager* CTextureAtlasManager::mpInstance = NULL;
CTextureAtlasManager* CTextureAtlasManager::instance( void )
{
    if( mpInstance == NULL )
    {
        mpInstance = new CTextureAtlasManager();
    }
    
    return mpInstance;
}

/*
**
*/
CTextureAtlasManager::CTextureAtlasManager( void )
{
    miNumAtlasInfo = 0;
    miNumAtlasInfoAlloc = ATLAS_PER_ALLOC;
    maAtlasInfo = (tTextureAtlasInfo *)MALLOC( sizeof( tTextureAtlasInfo ) * miNumAtlasInfoAlloc );
}

/*
**
*/
CTextureAtlasManager::~CTextureAtlasManager( void )
{
    FREE( maAtlasInfo );
}

/*
**
*/
tTextureAtlasInfo* CTextureAtlasManager::getTextureAtlasInfo( const char* szTextureName ) const
{
    int iHashID = hash( szTextureName );
    for( int i = 0; i < miNumAtlasInfo; i++ )
    {
        tTextureAtlasInfo* pInfo = &maAtlasInfo[i];
        if( pInfo->miHashID == iHashID )
        {
			if( !strcmp( pInfo->mszTextureName, szTextureName ) )
			{
				return pInfo;
			}
		}
    }
    
    return NULL;
}

/*
**
*/
void CTextureAtlasManager::loadInfo( const char* szFileName, const char* szExtension )
{
    char szFullPath[256];
    getFullPath( szFullPath, szFileName );
    
    TiXmlDocument doc( szFullPath );
    bool bLoaded = doc.LoadFile();
    if( bLoaded )
    {
        TiXmlNode* pNode = doc.FirstChild()->FirstChild();
        while( pNode )
        {
            const char* szNode = pNode->Value();
            if( !strcmp( szNode, "texture" ) )
            {
                TiXmlNode* pChild = pNode->FirstChild();
                
                if( miNumAtlasInfo + 1 >= miNumAtlasInfoAlloc )
                {
                    miNumAtlasInfoAlloc += ATLAS_PER_ALLOC;
                    maAtlasInfo = (tTextureAtlasInfo *)REALLOC( maAtlasInfo, sizeof( tTextureAtlasInfo ) * miNumAtlasInfoAlloc );
                }
                
                tTextureAtlasInfo* pInfo = &maAtlasInfo[miNumAtlasInfo++];
                while( pChild )
                {
                    const char* szChildVal = pChild->Value();
                    
                    WTFASSERT2( strlen( szChildVal ) < sizeof( pInfo->mszTextureName ), "texture name too long" );
                    if( !strcmp( szChildVal, "name" ) )
                    {
                        strncpy( pInfo->mszTextureName, pChild->FirstChild()->Value(), sizeof( pInfo->mszTextureName ) );
                        pInfo->miHashID = hash( pInfo->mszTextureName );
                    }
                    else if( !strcmp( szChildVal, "atlas" ) )
                    {
                        const char* szTexture = pChild->FirstChild()->Value();
                        char szNoExtension[260];
                        char szAtlas[256];
                        
                        // get the pvr file name
                        memset( szNoExtension, 0, sizeof( szNoExtension ) );
                        const char* szEnd = strstr( szTexture, szExtension );
                        if( szEnd == NULL )
                        {
                            memcpy( szNoExtension, szTexture, sizeof( szNoExtension ) );
                        }
                        else
                        {
                            memcpy( szNoExtension, szTexture, (size_t)szEnd - (size_t)szTexture );
                            strncat( szNoExtension, ".pvr", 4 );
                        }
                        
                        // see if we have the pvr file
                        char szFullPVRPath[256];
                        getFullPath( szFullPVRPath, szNoExtension );
                        WTFASSERT2( strlen( szFullPVRPath ) < sizeof( szFullPVRPath ), "pvr name is too long" );
                        
                        FILE* pvrFile = fopen( szFullPVRPath, "rb" );
                        if( pvrFile != NULL )
                        {
                            strncpy( szAtlas, szNoExtension, sizeof( szAtlas ) );
                            fclose( pvrFile );
                        }
                        else
                        {
                            strncpy( szAtlas, szTexture, sizeof( szAtlas ) );
                        }
                        
                        pInfo->mpTexture = CTextureManager::instance()->getTexture( szAtlas, false );
                        if( pInfo->mpTexture == NULL )
                        {
                            CTextureManager::instance()->registerTexture( szAtlas );
                            pInfo->mpTexture = CTextureManager::instance()->getTexture( szAtlas, false );
                        }
                        
                        WTFASSERT2( pInfo->mpTexture, "no texture %s", szTexture );
                    }
                    else if( !strcmp( szChildVal, "topleft" ) )
                    {
                        const char* szTopLeft = pChild->FirstChild()->Value();
                        
                        tVector4 topLeft;
                        parseVector( &topLeft, szTopLeft );
                        
                        pInfo->mTopLeft.fX = topLeft.fX;
                        pInfo->mTopLeft.fY = topLeft.fY;
                    }
                    else if( !strcmp( szChildVal, "bottomright" ) )
                    {
                        const char* szBottomRight = pChild->FirstChild()->Value();
                        
                        tVector4 bottomRight;
                        parseVector( &bottomRight, szBottomRight );
                        
                        pInfo->mBottomRight.fX = bottomRight.fX;
                        pInfo->mBottomRight.fY = bottomRight.fY;
                    }
                    
                    pChild = pChild->NextSibling();
                }
            }
            
            pNode = pNode->NextSibling();
        }
    }
    else
    {
        OUTPUT( "ERROR LOADING %s: %s\n", szFileName, doc.ErrorDesc() );
    }
}

/*
**
*/
void CTextureAtlasManager::addTexture( const char* szTexture )
{
    if( miNumAtlasInfo + 1 >= miNumAtlasInfoAlloc )
    {
        miNumAtlasInfoAlloc += ATLAS_PER_ALLOC;
        maAtlasInfo = (tTextureAtlasInfo *)REALLOC( maAtlasInfo, sizeof( tTextureAtlasInfo ) * miNumAtlasInfoAlloc );
    }
    
    tTextureAtlasInfo* pLastInfo = &maAtlasInfo[miNumAtlasInfo-1];
    tTextureAtlasInfo* pInfo = &maAtlasInfo[miNumAtlasInfo++];
    
    char szFullPath[256];
    getFullPath( szFullPath, szTexture );
    FILE* fp = fopen( szFullPath, "rb" );
    if( fp )
    {
        fseek( fp, 12, SEEK_SET );
        int iLow = (int)fgetc( fp );
        int iHigh = (int)fgetc( fp );
        int iImageWidth = iLow | ( iHigh << 8 );
        
        iLow = (int)getc( fp );
        iHigh = (int)getc( fp );
        int iImageHeight = iLow | ( iHigh << 8 );
        
        float fX = pLastInfo->mBottomRight.fX + 2.0f;
        float fY = pLastInfo->mTopLeft.fY;
        if( fX + (float)iImageWidth >= 2048 )
        {
            // need the range of the textures in this row for the bottom most coordinate
            fX = 2.0f;
            int iStartRow = 0;
            for( int i = miNumAtlasInfo - 2; i >= 0; i-- )
            {
                tTextureAtlasInfo* pCheckInfo = &maAtlasInfo[i];
                if( pCheckInfo->mTopLeft.fY < pLastInfo->mTopLeft.fY )
                {
                    iStartRow = i;
                    break;
                }
            }
            
            // get the bottom y
            float fBottomMostY = 0.0f;
            for( int i = iStartRow; i < miNumAtlasInfo - 1; i++ )
            {
                if( maAtlasInfo[i].mBottomRight.fY > fBottomMostY )
                {
                    fBottomMostY = maAtlasInfo[i].mBottomRight.fY;
                }
            }
            
            fY = (int)( fBottomMostY + 2.0f );
        }
        
        pInfo->mTopLeft.fX = fX;
        pInfo->mTopLeft.fY = fY;
        pInfo->miHashID = hash( szTexture );
        pInfo->miTextureAtlasWidth = 2048;
        pInfo->miTextureAtlasHeight = 2048;
        pInfo->mTopLeft.fX = fX;
        pInfo->mTopLeft.fY = fY;
        pInfo->mBottomRight.fX = fX + (float)iImageWidth;
        pInfo->mBottomRight.fY = fY + (float)iImageHeight;
        
        strncpy( pInfo->mszTextureName, szTexture, sizeof( pInfo->mszTextureName ) );
        
        
    }
    
}
