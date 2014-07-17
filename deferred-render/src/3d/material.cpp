#include "material.h"

#include "filepathutil.h"
#include "hashutil.h"
#include "shadermanager.h"
#include "parseutil.h"

#include "tinyxml.h"

/*
**
*/
void materialLoad( tMaterial* pMaterial, const char* szFileName )
{
	pMaterial->mColor.fX = pMaterial->mColor.fY = pMaterial->mColor.fZ = pMaterial->mColor.fW = 1.0f;

	char szFilePath[256];
    getFullPath( szFilePath, szFileName );
    
    TiXmlDocument doc( szFilePath );
    bool bLoaded = doc.LoadFile();
    
    if( bLoaded )
    {
        TiXmlNode* pNode = doc.FirstChild()->FirstChild();
        while( pNode )
        {
            const char* szValue = pNode->Value();
            if( !strcmp( szValue, "name" ) )
            {
                const char* szName = pNode->FirstChild()->Value();
                strncpy( pMaterial->mszName, szName, sizeof( pMaterial->mszName ) );
                pMaterial->miHash = hash( pMaterial->mszName );
            }
            else if( !strcmp( szValue, "texture_0" ) )
            {
                const char* szTexture = pNode->FirstChild( "name" )->FirstChild()->Value();
                const char* szType = pNode->FirstChild( "type" )->FirstChild()->Value();
                const char* szShaderName = pNode->FirstChild( "shader_name" )->FirstChild()->Value();
                
                tTexture* pTexture = CTextureManager::instance()->getTexture( szTexture );
                if( pTexture == NULL )
                {
                    CTextureManager::instance()->registerTexture( szTexture );
                    pTexture = CTextureManager::instance()->getTexture( szTexture );
                }
                
                WTFASSERT2( pTexture, "can't load texture: %s", szTexture );
                pMaterial->mapTextures[0] = pTexture;
                pMaterial->miNumTextures = 1;
            }
            else if( !strcmp( szValue, "texture_1" ) )
            {
                const char* szTexture = pNode->FirstChild( "name" )->FirstChild()->Value();
                const char* szType = pNode->FirstChild( "type" )->FirstChild()->Value();
                const char* szShaderName = pNode->FirstChild( "shader_name" )->FirstChild()->Value();
                
                tTexture* pTexture = CTextureManager::instance()->getTexture( szTexture );
                
                WTFASSERT2( pTexture, "can't load texture: %s", szTexture );
                pMaterial->mapTextures[1] = pTexture;
                pMaterial->miNumTextures = 2;
            }
            else if( !strcmp( szValue, "texture_2" ) )
            {
                const char* szTexture = pNode->FirstChild( "name" )->FirstChild()->Value();
                const char* szType = pNode->FirstChild( "type" )->FirstChild()->Value();
                const char* szShaderName = pNode->FirstChild( "shader_name" )->FirstChild()->Value();
                
                tTexture* pTexture = CTextureManager::instance()->getTexture( szTexture );
                if( pTexture == NULL )
                {
                    CTextureManager::instance()->registerTexture( szTexture );
                    pTexture = CTextureManager::instance()->getTexture( szTexture );
                }
                
                WTFASSERT2( pTexture, "can't load texture: %s", szTexture );
                pMaterial->mapTextures[2] = pTexture;
                pMaterial->miNumTextures = 3;
            }
            else if( !strcmp( szValue, "texture_3" ) )
            {
                const char* szTexture = pNode->FirstChild( "name" )->FirstChild()->Value();
                const char* szType = pNode->FirstChild( "type" )->FirstChild()->Value();
                const char* szShaderName = pNode->FirstChild( "shader_name" )->FirstChild()->Value();
                
                tTexture* pTexture = CTextureManager::instance()->getTexture( szTexture );
                if( pTexture == NULL )
                {
					CTextureManager::instance()->registerTexture( szTexture );
                    pTexture = CTextureManager::instance()->getTexture( szTexture );
                }
                
                WTFASSERT2( pTexture, "can't load texture: %s", szTexture );
                pMaterial->mapTextures[3] = pTexture;
                pMaterial->miNumTextures = 4;
            }
            else if( !strcmp( szValue, "shader" ) )
            {
                const char* szShader = pNode->FirstChild()->Value();
                pMaterial->miShaderProgram = CShaderManager::instance()->getShader( szShader );
                WTFASSERT2( pMaterial->miShaderProgram >= 0, "invalid shader: %s", szShader );
            }
			else if( !strcmp( szValue, "color" ) )
            {
                const char* szColor = pNode->FirstChild()->Value();
                parseVector( &pMaterial->mColor, szColor );
            }
                        
            pNode = pNode->NextSibling();
        
        }   // while valid node
        
    }   // if loaded
	else
	{
		OUTPUT( "can't load %s : %s\n", szFileName, doc.ErrorDesc() );
		WTFASSERT2( 0, "can't load material" );
	}
}