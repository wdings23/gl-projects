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
tTextureAtlasInfo* CTextureAtlasManager::getTextureAtlasInfo( const char* szTextureName )
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