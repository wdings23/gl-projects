//
//  font.cpp
//  CityBenchmark
//
//  Created by Dingwings on 7/27/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "font.h"
#include "filepathutil.h"
#include "texturemanager.h"
#include "matrix.h"
#include "tinyxml.h"
#include "timeutil.h"
#include "ShaderManager.h"
#include "render.h"

#define GLYPH_TILE_WIDTH 64
#define GLYPH_TILE_HEIGHT 64

/*
**
*/
CFont::CFont( void )
{
    memset( mszName, 0, sizeof( mszName ) );
    
    for( int i = 0; i < sizeof( maGlyphs ) / sizeof( *maGlyphs ); i++ )
    {
        maGlyphs[i].maKerning.clear();
    }

	mpTextureAtlasInfo = NULL;
}

/*
**
*/
CFont::~CFont( void )
{
    /*for( int i = 0; i < sizeof( maGlyphs ) / sizeof( *maGlyphs ); i++ )
    {
        tGlyph* pGlyph = &maGlyphs[i];
        for( size_t j = 0; j < pGlyph->maKerning.size(); j++ )
        {
            FREE( pGlyph->maKerning[i] );
        }
    }*/
    
}

/*					  
 **					| +y
 **					| 
 **					| 
 **		 -x ----------------- +x
 **					|  _____
 **					| |     |
 **					| |	 A  | - GLYPH
 **					| |_____|
 **					  -y
 */	
void CFont::readFile( const char* szFileName )
{
    char szFilePath[512];
	getFullPath( szFilePath, szFileName );
    
    TiXmlDocument doc( szFilePath );
    bool bLoaded = doc.LoadFile();
    if( bLoaded )
    {
        // name
        strncpy( mszName, szFileName, sizeof( mszName ) );
        
        // get texture
        char szTextureName[64];
        memset( szTextureName, 0, sizeof( szTextureName ) );
        const char* szEnd = strstr( szFileName, ".fnt" );
        memcpy( szTextureName, szFileName, szEnd - szFileName );
        strcat( szTextureName, ".png" );
        
        // check for texture atlas
        tTextureAtlasInfo* pTextureAtlasInfo = CTextureAtlasManager::instance()->getTextureAtlasInfo( szTextureName );
        if( pTextureAtlasInfo )
        {
            mpTexture = pTextureAtlasInfo->mpTexture;
            mpTextureAtlasInfo = pTextureAtlasInfo;
        }
        else
        {
            mpTexture = CTextureManager::instance()->getTexture( szTextureName );
            if( mpTexture == NULL )
            {
                CTextureManager::instance()->registerTexture( szTextureName );
                mpTexture = CTextureManager::instance()->getTexture( szTextureName );
            }
        }
        
        float fTextureWidth = (float)mpTexture->miGLWidth;
        float fTextureHeight = (float)mpTexture->miGLHeight;
        
        WTFASSERT2( mpTexture, "Can't find texture %s", szTextureName );
        
        TiXmlNode* pTop = doc.FirstChild( "font" );
        TiXmlNode* pNode = pTop->FirstChild( "chars" )->FirstChild();
        TiXmlNode* pKerningNode = pTop->FirstChild( "kernings" );
        if( pKerningNode )
        {
            pKerningNode = pKerningNode->FirstChild( "kerning" );
        }
        TiXmlNode* pInfoNode = pTop->FirstChild( "info" );
        miOriginalSize = atoi( pInfoNode->ToElement()->Attribute( "size" ) );
        
        while( pNode )
        {
            const char* szValue = pNode->Value();
            
            if( !strcmp( szValue, "char" ) )
            {
                TiXmlElement* pElement = pNode->ToElement();
                const char* szData = pElement->Attribute( "id" );
                int iCharIndex = (int)atoi( szData );
                
                tGlyph* pGlyph = &maGlyphs[iCharIndex];
                
                szData = pElement->Attribute( "x" );
                pGlyph->mTopLeft.miX = atoi( szData );
                
                szData = pElement->Attribute( "y" );
                pGlyph->mTopLeft.miY = atoi( szData );
                
                szData = pElement->Attribute( "width" );
                pGlyph->miWidth = atoi( szData );
                
                szData = pElement->Attribute( "height" );
                pGlyph->miHeight = atoi( szData );
                
                szData = pElement->Attribute( "xoffset" );
                pGlyph->miXOffset = atoi( szData );
                
                szData = pElement->Attribute( "yoffset" );
                pGlyph->miYOffset = atoi( szData );
                
                szData = pElement->Attribute( "xadvance" );
                pGlyph->miXAdvance = atoi( szData );
                
                pGlyph->mBottomRight.miX = pGlyph->mTopLeft.miX + pGlyph->miWidth;
                pGlyph->mBottomRight.miY = pGlyph->mTopLeft.miY + pGlyph->miHeight;
                
                if( pTextureAtlasInfo )
                {
                    pGlyph->maUV[0].fX = ( pTextureAtlasInfo->mTopLeft.fX + (float)pGlyph->mTopLeft.miX ) / (float)pTextureAtlasInfo->mpTexture->miWidth;
                    pGlyph->maUV[0].fY = ( pTextureAtlasInfo->mTopLeft.fY + (float)pGlyph->mTopLeft.miY ) / (float)pTextureAtlasInfo->mpTexture->miHeight;
                    
                    pGlyph->maUV[1].fX = ( pTextureAtlasInfo->mTopLeft.fX + (float)pGlyph->mBottomRight.miX ) / (float)pTextureAtlasInfo->mpTexture->miWidth;
                    pGlyph->maUV[1].fY = ( pTextureAtlasInfo->mTopLeft.fY + (float)pGlyph->mTopLeft.miY ) / (float)pTextureAtlasInfo->mpTexture->miHeight;
                    
                    pGlyph->maUV[2].fX = ( pTextureAtlasInfo->mTopLeft.fX + (float)pGlyph->mTopLeft.miX ) / (float)pTextureAtlasInfo->mpTexture->miWidth;
                    pGlyph->maUV[2].fY = ( pTextureAtlasInfo->mTopLeft.fY + (float)pGlyph->mBottomRight.miY ) / (float)pTextureAtlasInfo->mpTexture->miHeight;
                    
                    pGlyph->maUV[3].fX = ( pTextureAtlasInfo->mTopLeft.fX + (float)pGlyph->mBottomRight.miX ) / (float)pTextureAtlasInfo->mpTexture->miWidth;
                    pGlyph->maUV[3].fY = ( pTextureAtlasInfo->mTopLeft.fY + (float)pGlyph->mBottomRight.miY ) / (float)pTextureAtlasInfo->mpTexture->miHeight;
                    
                }
                else
                {
                    // u = x / texture width, v = y / texture height
                    // top-left
                    pGlyph->maUV[0].fX = (float)( pGlyph->mTopLeft.miX ) / fTextureWidth;
                    pGlyph->maUV[0].fY = (float)( pGlyph->mTopLeft.miY ) / fTextureHeight;
                    
                    // top-right
                    pGlyph->maUV[1].fX = (float)( pGlyph->mBottomRight.miX  ) / fTextureWidth;
                    pGlyph->maUV[1].fY = (float)( pGlyph->mTopLeft.miY ) / fTextureHeight;
                    
                    // bottom-left
                    pGlyph->maUV[2].fX = (float)( pGlyph->mTopLeft.miX ) / fTextureWidth;
                    pGlyph->maUV[2].fY = (float)( pGlyph->mBottomRight.miY ) / fTextureHeight;
                    
                    // bottom-right
                    pGlyph->maUV[3].fX = (float)( pGlyph->mBottomRight.miX ) / fTextureWidth;
                    pGlyph->maUV[3].fY = (float)( pGlyph->mBottomRight.miY ) / fTextureHeight;
                }
                
                // top-left
                maGlyphVerts[iCharIndex*4].fX = (float)pGlyph->miXOffset;
                maGlyphVerts[iCharIndex*4].fY = (float)pGlyph->miYOffset;
            
                // top-right
                maGlyphVerts[iCharIndex*4+1].fX = (float)( pGlyph->miXOffset + pGlyph->miWidth );
                maGlyphVerts[iCharIndex*4+1].fY = (float)pGlyph->miYOffset;
                
                // bottom-left
                maGlyphVerts[iCharIndex*4+2].fX = (float)pGlyph->miXOffset;
                maGlyphVerts[iCharIndex*4+2].fY = (float)( pGlyph->miYOffset + pGlyph->miHeight );
                
                // bottom-right
                maGlyphVerts[iCharIndex*4+3].fX = (float)( pGlyph->miXOffset + pGlyph->miWidth );
                maGlyphVerts[iCharIndex*4+3].fY = (float)( pGlyph->miYOffset + pGlyph->miHeight );
            }
            
            pNode = pNode->NextSibling();
        }
        
        // kerning
        while( pKerningNode )
        {
            TiXmlElement* pElement = pKerningNode->ToElement();
            const char* szData = pElement->Attribute( "first" );
            int iCharIndex = atoi( szData );
            
            szData = pElement->Attribute( "second" );
            int iNextIndex = atoi( szData );
            
            szData = pElement->Attribute( "amount" );
            int iAmount = atoi( szData );
            
            tGlyph* pGlyph = &maGlyphs[iCharIndex];
            
            tCoord2Di* pKerning = (tCoord2Di *)MALLOC( sizeof( tCoord2Di ) );
            pKerning->miX = iNextIndex;
            pKerning->miY = iAmount;
            pGlyph->maKerning.push_back( pKerning );
            
            pKerningNode = pKerningNode->NextSibling();
        }
        
    }   // if document loaded
    else
    {
        OUTPUT( "error loading %s: %s\n", szFileName, doc.ErrorDesc() );
        WTFASSERT2( 0, "can't load %s", szFileName );
    }
}

/*
**
*/
void CFont::drawString( const char* szString,
                        float fX,
                        float fY,
                        float fSize,
                        float fScreenWidth,
                        float fScreenHeight,
                        float fRed,
                        float fGreen,
                        float fBlue,
                        float fAlpha,
                        int iAlignment,
                        bool bUseBatch )
{
	if( !bUseBatch )
	{
		CTextureManager::instance()->loadTextureInMem( mpTexture, getCurrTime() );
		glBindTexture( GL_TEXTURE_2D, mpTexture->miID );
	}

    float fFontScale = fSize / (float)miOriginalSize;
    
	float fHalfWidth = fScreenWidth * 0.5f;
	float fHalfHeight = fScreenHeight * 0.5f;
	
    tVector4 currPos = 
    {
        fX, fY, 0.0f, 1.0f
    };
    
	int iCurrLine = 1;
	int iTextWidth = getLineWidth( szString, fSize, iCurrLine, fScreenWidth );
	
	if( iAlignment == ALIGN_CENTER ) // center
	{
		currPos.fX -= (float)( iTextWidth >> 1 );
	}
	else if( iAlignment == ALIGN_RIGHT ) // right
	{
		currPos.fX = ( fX - (float)iTextWidth ); 
	}
	
    tMatrix44 orientationMatZ;
    Matrix44RotateZ( &orientationMatZ, 0.0f );
    glUniformMatrix4fv( miOrientationMatHandle, 1, GL_FALSE, orientationMatZ.afEntries );
    
    tVector4 aTextColor[] =
    {
        { fRed, fGreen, fBlue, fAlpha },
        { fRed, fGreen, fBlue, fAlpha },
        { fRed, fGreen, fBlue, fAlpha },
        { fRed, fGreen, fBlue, fAlpha },
    };
    
	const char* pPtr = szString;
	do
	{
		if( *pPtr == '\n' )
		{
			++iCurrLine;
			iTextWidth = getLineWidth( szString, fSize, iCurrLine, fScreenWidth ); 
			if( iAlignment == ALIGN_LEFT ) // left
			{
				currPos.fX = fX;
			}
			else if( iAlignment == ALIGN_CENTER ) // center
			{
                currPos.fX = fX - (float)iTextWidth * 0.5f;
			}
			else if( iAlignment == ALIGN_RIGHT ) // right
			{
				currPos.fX = fX - (float)iTextWidth;
			}
			
			currPos.fY += ( fSize );
			++pPtr;
			continue;
		}
		
		char cChar = *pPtr;
		int iGlyphIdx = cChar;
        
        if( cChar < 0 )
        {
            ++pPtr;
            continue;
        }
        
        tGlyph* pGlyph = &maGlyphs[iGlyphIdx];
        
		tVector4 aTransformedV[4];
		
		// convert glyph's local coordinate to screen coordinate
		for( int i = 0; i < 4; i++ )
		{
			aTransformedV[i].fX = maGlyphVerts[iGlyphIdx*4+i].fX * fFontScale + currPos.fX;
			aTransformedV[i].fY = maGlyphVerts[iGlyphIdx*4+i].fY * fFontScale + currPos.fY;
			aTransformedV[i].fZ = 0.0f;
			aTransformedV[i].fW = 1.0f;
		
            aTransformedV[i].fX = ( aTransformedV[i].fX - fHalfWidth ) / fHalfWidth;
            aTransformedV[i].fY = ( fHalfHeight - aTransformedV[i].fY ) / fHalfHeight; 
        }
        
		// uv
        tVector2 aUV[] =
        {
            { pGlyph->maUV[0].fX, pGlyph->maUV[0].fY }, 
            { pGlyph->maUV[1].fX, pGlyph->maUV[1].fY }, 
            { pGlyph->maUV[2].fX, pGlyph->maUV[2].fY }, 
            { pGlyph->maUV[3].fX, pGlyph->maUV[3].fY },
        };
        
		if( mpTextureAtlasInfo )
		{
			float fOneOverWidth = 1.0f / (float)mpTextureAtlasInfo->mpTexture->miGLWidth;
			float fOneOverHeight = 1.0f / (float)mpTextureAtlasInfo->mpTexture->miGLHeight;

			float fLeft = mpTextureAtlasInfo->mTopLeft.fX;
			float fTop = mpTextureAtlasInfo->mTopLeft.fY;
		
			aUV[0].fX = ( (float)pGlyph->mTopLeft.miX + fLeft ) * fOneOverWidth;
			aUV[0].fY = ( (float)pGlyph->mTopLeft.miY + fTop ) * fOneOverHeight;

			aUV[1].fX = ( (float)pGlyph->mBottomRight.miX + fLeft ) * fOneOverWidth;
			aUV[1].fY = ( (float)pGlyph->mTopLeft.miY + fTop ) * fOneOverHeight;

			aUV[2].fX = ( (float)pGlyph->mTopLeft.miX + fLeft ) * fOneOverWidth;
			aUV[2].fY = ( (float)pGlyph->mBottomRight.miY + fTop ) * fOneOverHeight;

			aUV[3].fX = ( (float)pGlyph->mBottomRight.miX + fLeft ) * fOneOverWidth;
			aUV[3].fY = ( (float)pGlyph->mBottomRight.miY + fTop ) * fOneOverHeight;
			
		}

        if( bUseBatch )
        {
            unsigned int iShader = CShaderManager::instance()->getShader( "ui" );
            renderQuad( mpTexture->mszName, aTransformedV, aTextColor, aUV, iShader );
        }
        else
        {
            glVertexAttribPointer( miPositionHandle, 4, GL_FLOAT, GL_FALSE, 0, &aTransformedV );
            glVertexAttribPointer( miTexCoordHandle, 2, GL_FLOAT, GL_FALSE, 0, aUV );
            glVertexAttribPointer( miColorHandle, 4, GL_FLOAT, GL_FALSE, 0, aTextColor );
            
            glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 ); 
        }
		
        // get kerning
        float fKerning = 0.0f;
        char cNextChar = *( pPtr + 1 );
        
        for( unsigned int i = 0; i < pGlyph->maKerning.size(); i++ )
        {
            tCoord2Di* pKerning = pGlyph->maKerning[i];
            if( pKerning->miX == (int)cNextChar )
            {
                fKerning = (float)pKerning->miX;
                break;
            }
        }
        
        // increment current position by the width of the glyph 
		currPos.fX += ( (float)pGlyph->miXAdvance * fFontScale );
        
		++pPtr;
	} while( *pPtr );
}

/*
**
*/
int CFont::getLineWidth( const char* szString, 
                         const float fTextSize, 
                         const int iCurrLine,
                         float fScreenWidth )
{
	char szLine[512];
	
	size_t iCharCount = 0;
	for( int iLine = 0; iLine < iCurrLine; iLine++ )
	{
		memset( szLine, 0, sizeof( szLine ) );
		for( int i = 0; iCharCount < strlen( szString ); i++ )
		{
			if( szString[iCharCount] == '\n' )
			{
				++iCharCount;
				break;
			}
			
			szLine[i] = szString[iCharCount++];
		}
	}
	
	int iTextWidth = getStringWidth( szLine, fTextSize, fScreenWidth );
	return iTextWidth;
}

/*
**
*/
int CFont::getStringWidth( const char* szString, float fSize, float fScreenWidth )
{
    if( szString == NULL )
    {
        return 0;
    }
    
	float fWidth = 0.0f;
	float fFontScale = fSize / (float)miOriginalSize;
	
	do
	{
		char cChar = *szString;
		int iGlyphIdx = cChar;
        
        if( iGlyphIdx >= 0 && iGlyphIdx <= 255 )
        {
            float fGlyphWidth = (float)maGlyphs[iGlyphIdx].miXAdvance;
            fWidth += fGlyphWidth;
        }
		
		++szString;
	} while( *szString );
    
	return (int)( fWidth * fFontScale );
}

/*
**
*/
void CFont::wrapText( char* szResult, 
                       const char* szOrig, 
                       int iAlign, 
                       float fWidth,
                       float fSize,
                       int iResultSize,
                       float fScreenWidth )
{
    int iLine = 0;
    int iLast = (int)strlen( szOrig );
    int iCount = 0;
    
    float fSpaceWidth = getStringWidth( " ", fSize, fScreenWidth );
    
    //if( iAlign == ALIGN_CENTER )
    //if( strstr( szOrig, "NOT ENOUGH" ) )    
    {
        char szLine[256];
        memset( szLine, 0, sizeof( szLine ) );
        
        float fCurrWidth = 0.0f;
        while( iCount < iLast )
        {
            char szWord[128];
            memset( szWord, 0, sizeof( szWord ) );
            
            int iChar = 0;
            while( szOrig[iCount] != '\0' && szOrig[iCount] != ' ' && iCount < iLast )
            {
                szWord[iChar++] = szOrig[iCount++];
            }   // while char != delimiter
            
            ++iCount;
            
            float fWordWidth = getStringWidth( szWord, fSize, fScreenWidth );
            if( fWordWidth + fCurrWidth < fWidth )
            {
                if( strlen( szResult ) > 0 )
                {
                    strncat( szResult, " ", iResultSize );
                }
                
                strncat( szResult, szWord, iResultSize );
                
                fCurrWidth += ( fWordWidth + fSpaceWidth );
            }
            else
            {
                if( strlen( szResult ) > 0 )
                {
                    strncat( szResult, "\n", iResultSize );
                }
                    
                strncat( szResult, szWord, iResultSize );
                fCurrWidth = 0.0f;
                ++iLine;
            }
        }   // while still has characters
    }
}

/*
**
*/
float CFont::getHeight( const char* szText, float fSize )
{
    float fCurrSize = fSize;
    const char* pszText = szText;
    while( *pszText )
    {
        if( *pszText == '\n' )
        {
            fCurrSize += fSize;
        }
        
        ++pszText;
    }
    
    return fCurrSize;
}