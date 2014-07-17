#include "menuanimmanager.h"
#include "filepathutil.h"

/*
**
*/
CMenuAnimManager* CMenuAnimManager::mpInstance = NULL;
CMenuAnimManager* CMenuAnimManager::instance( void )
{
	if( mpInstance == NULL )
	{
		mpInstance = new CMenuAnimManager();
	}

	return mpInstance;
}

/*
**
*/
CMenuAnimManager::CMenuAnimManager( void )
{

}

/*
**
*/
CMenuAnimManager::~CMenuAnimManager( void )
{
	for( unsigned int i = 0; i < mapAnimPlayers.size(); i++ )
	{
		CAnimPlayerDB* pAnimPlayer = mapAnimPlayers[i];
		delete pAnimPlayer;
	}
}

/*
**
*/
CAnimPlayerDB* CMenuAnimManager::getAnimPlayer( const char* szAnimPlayer )
{
	CAnimPlayerDB* pFound = NULL;
	for( unsigned int i = 0; i < mapAnimPlayers.size(); i++ )
	{
		CAnimPlayerDB* pAnimPlayer = mapAnimPlayers[i];
		if( !strcmp( pAnimPlayer->getName(), szAnimPlayer ) )
		{
			pFound = pAnimPlayer;
			break;
		}
	}

	return pFound;
}

/*
**
*/
CAnimPlayerDB* CMenuAnimManager::load( const char* szFileName, std::vector<CAnimPlayerDB *> *pAnimList )
{
    CAnimPlayerDB* pAnimPlayer = NULL;
    
	char szFullPath[256];
	getFullPath( szFullPath, szFileName );
	TiXmlDocument doc( szFullPath );
	bool bLoaded = doc.LoadFile();
    WTFASSERT2( bLoaded, "can't load %s", szFileName );
	
	if( bLoaded )
	{
		TiXmlElement* pElement =
        doc.FirstChildElement();
		TiXmlNode* pNode = pElement->FirstChild();
		
		float fWidth = 512.0f;
		float fHeight = 384.0f;
		
		while( pNode )
		{
			const char* szValue = pNode->Value();
			if( !strcmp( szValue, "width" ) )
			{
				TiXmlNode* pValueNode = pNode->FirstChild();
				const char* szWidth = pValueNode->Value();
				fWidth = atof( szWidth );
			}
			else if( !strcmp( szValue, "height" ) )
			{
				TiXmlNode* pValueNode = pNode->FirstChild();
				const char* szHeight = pValueNode->Value();
				fHeight = atof( szHeight );
			}
			else if( !strcmp( szValue, "layer" ) )
			{
				// animation frames
                TiXmlNode* pChild = pNode->FirstChild();
				
                TiXmlNode* pNameNode = pNode->FirstChild( "name" )->FirstChild();
                const char* szName = pNameNode->Value();
                
                // filename to concat to the actual name of the animation for ID
                char szNoExtension[64];
                const char* szEnd = strstr( szFileName, "." );
                if( szEnd )
                {
                    memset( szNoExtension, 0, sizeof( szNoExtension ) );
                    memcpy( szNoExtension, szFileName, (size_t)szEnd - (size_t)szFileName );
                }
                else
                {
                    strcpy( szNoExtension, szFileName );
                }
                
                char szFullName[256];
                sprintf( szFullName, "%s/%s", szNoExtension, szName );
                
                // look for same player
                bool bFound = false;
                for( unsigned int i = 0; i < mapAnimPlayers.size(); i++ )
                {
                    CAnimPlayerDB* pAnimDB = mapAnimPlayers[i];
                    if( !strcmp( pAnimDB->getName(), szFullName ) )
                    {
                        pAnimPlayer = pAnimDB;
                        if( pAnimList )
                        {
                            pAnimList->push_back( pAnimPlayer );
                        }
                        
                        bFound = true;
                        break;
                    }
                }
                
                // only load if not found
                if( !bFound )
                {
                    pAnimPlayer = new CAnimPlayerDB();
                        
                    pAnimPlayer->setStageWidth( fWidth );
                    pAnimPlayer->setStageHeight( fHeight );

                    pAnimPlayer->load( pChild, szFileName );
                    mapAnimPlayers.push_back( pAnimPlayer );
                    if( pAnimList )
                    {
                        pAnimList->push_back( pAnimPlayer );
                    }
                }
                
                pChild = pChild->NextSibling();
			}
            else if( !strcmp( szValue, "particle" ) )
            {
                TiXmlNode* pChild = pNode->FirstChild();
                
                // name of the particle
                TiXmlNode* pNameNode = pNode->FirstChild( "name" )->FirstChild();
                const char* szName = pNameNode->Value();
                
                // filename to concat to the actual name of the animation for ID
                char szNoExtension[64];
                const char* szEnd = strstr( szFileName, "." );
                if( szEnd )
                {
                    memset( szNoExtension, 0, sizeof( szNoExtension ) );
                    memcpy( szNoExtension, szFileName, (size_t)szEnd - (size_t)szFileName );
                }
                else
                {
                    strcpy( szNoExtension, szFileName );
                }
                
                char szFullName[256];
                sprintf( szFullName, "%s/%s", szNoExtension, szName );
            
                // position of the particle
                TiXmlNode* pPositionNode = pNode->FirstChild( "positions" )->FirstChild( "keyframe" )->FirstChild( "position" )->FirstChild();
                const char* szPosition = pPositionNode->Value();
                
                // parse out coordinate
                tVector2 position;
                char szNum[32];
                memset( szNum, 0, sizeof( szNum ) );
                szEnd = strstr( szPosition, "," );
                memcpy( szNum, szPosition, (size_t)szEnd - (size_t)szPosition );
                position.fX = atof( szNum );
                memset( szNum, 0, sizeof( szNum ) );
                strncpy( szNum, (const char *)( (size_t)szEnd + 1 ), sizeof( szNum ) );
                position.fY = atof( szNum );
                
                // look for same player
                bool bFound = false;
                for( unsigned int i = 0; i < mapAnimPlayers.size(); i++ )
                {
                    CAnimPlayerDB* pAnimDB = mapAnimPlayers[i];
                    if( !strcmp( pAnimDB->getName(), szFullName ) )
                    {
                        tVectorAnimFrame animKeyFrame;
                        pAnimDB->getFrameAtTime( &animKeyFrame, CAnimPlayerDB::FRAME_POSITION, 0.0f );
                        
                        // same coordinate?
                        if( fabs( animKeyFrame.mValue.fX - position.fX ) < 0.01f &&
                            fabs( animKeyFrame.mValue.fY - position.fY ) < 0.01f )
                        {
                            pAnimPlayer = pAnimDB;
                            if( pAnimList )
                            {
                                pAnimList->push_back( pAnimPlayer );
                            }
                            
                            bFound = true;
                            break;

                        }
                    }
                }
                
                // new animation player for this particle
                if( !bFound )
                {
                    pAnimPlayer = new CAnimPlayerDB();
                    
                    TiXmlNode* pName = pNode->FirstChild( "name" );
                    szName = pName->FirstChild()->Value();
                    
                    pAnimPlayer->setParticle( szName ); 
                    
                    pAnimPlayer->setStageWidth( fWidth );
                    pAnimPlayer->setStageHeight( fHeight );
                    
                    pAnimPlayer->load( pChild, szFileName );
                    mapAnimPlayers.push_back( pAnimPlayer );
                    if( pAnimList )
                    {
                        pAnimList->push_back( pAnimPlayer );
                    }
                }
                
                pChild = pChild->NextSibling();
            }
            else if( !strcmp( szValue, "sound" ) )
            {
                TiXmlNode* pChild = pNode->FirstChild();
                
                // name of the sound file
                TiXmlNode* pNameNode = pNode->FirstChild( "name" )->FirstChild();
                const char* szName = pNameNode->Value();
                
                // filename to concat to the actual name of the animation for ID
                char szNoExtension[64];
                const char* szEnd = strstr( szFileName, "." );
                if( szEnd )
                {
                    memset( szNoExtension, 0, sizeof( szNoExtension ) );
                    memcpy( szNoExtension, szFileName, (size_t)szEnd - (size_t)szFileName );
                }
                else
                {
                    strcpy( szNoExtension, szFileName );
                }
                
                char szFullName[256];
                sprintf( szFullName, "%s/%s", szNoExtension, szName );
                
                // look for same player
                bool bFound = false;
                for( unsigned int i = 0; i < mapAnimPlayers.size(); i++ )
                {
                    CAnimPlayerDB* pAnimDB = mapAnimPlayers[i];
                    if( !strcmp( pAnimDB->getName(), szFullName ) )
                    {
                        pAnimPlayer = pAnimDB;
                        if( pAnimList )
                        {
                            pAnimList->push_back( pAnimPlayer );
                        }
                        
                        bFound = true;
                        break;
                    }
                }
                
                // load if not found
                if( !bFound )
                {
                    pAnimPlayer = new CAnimPlayerDB();
                    
                    TiXmlNode* pName = pNode->FirstChild( "name" );
                    const char* szName = pName->FirstChild()->Value();
                    
                    pAnimPlayer->setSound( szName );                    
                    
                    pAnimPlayer->load( pChild, szFileName );
                    mapAnimPlayers.push_back( pAnimPlayer );
                    if( pAnimList )
                    {
                        pAnimList->push_back( pAnimPlayer );
                    }
                }
                
                pChild = pChild->NextSibling();
            }

			pNode = pNode->NextSibling();
		}
	}
    else
    {
        WTFASSERT2( 0, "Error loading file %s : %s", szFileName, doc.ErrorDesc() );
    }
    
    return pAnimPlayer;
}
