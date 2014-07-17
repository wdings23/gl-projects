#include "ShaderManager.h"
#include "filepathutil.h"
#include "hashutil.h"
#include "tinyxml.h"

static bool loadShader( const char* szName, GLuint* piShader, GLenum type );
static bool createShaderProgram( tShaderProgram* pProgram, void (*pfnBeforeLinkFunc)( GLuint iShader ) );

/*
**
*/
CShaderManager* CShaderManager::mpInstance = NULL;
CShaderManager* CShaderManager::instance( void )
{
	if( mpInstance == NULL )
	{
		mpInstance = new CShaderManager();
	}

	return mpInstance;
}

/*
**
*/
CShaderManager::CShaderManager( void )
{
	miNumShaderProgramAlloc = NUM_SHADERS_PER_ALLOC;
	maShaderPrograms = new tShaderProgram[miNumShaderProgramAlloc];
	memset( maShaderPrograms, 0, sizeof( tShaderProgram ) * miNumShaderProgramAlloc );
	miNumShaderPrograms = 0;
}

/*
**
*/
CShaderManager::~CShaderManager( void )
{
    for( int i = 0; i < NUM_SHADERS_PER_ALLOC; i++ )
    {
        glDeleteProgram( maShaderPrograms[i].miID );
    }
    
	delete[] maShaderPrograms;
	miNumShaderPrograms = 0;
	miNumShaderProgramAlloc = 0;
}

/*
**
*/
unsigned int CShaderManager::getShader( const char* szShaderName )
{
    int iHash = hash( szShaderName );
	for( int i = 0; i < miNumShaderPrograms; i++ )
	{
		//if( !strcmp( maShaderPrograms[i].mszName, szShaderName ) )
		if( maShaderPrograms[i].miHash == iHash )
        {
			return maShaderPrograms[i].miID;
		}

	}	// for i = 0 to num shader programs

	return 0;
}

/*
**
*/
tShaderProgram const* CShaderManager::getShaderProgram( const char* szShaderName )
{
	 int iHash = hash( szShaderName );
	for( int i = 0; i < miNumShaderPrograms; i++ )
	{
		//if( !strcmp( maShaderPrograms[i].mszName, szShaderName ) )
		if( maShaderPrograms[i].miHash == iHash )
        {
			return &maShaderPrograms[i];
		}

	}	// for i = 0 to num shader programs

	return NULL;
}

/*
**
*/
unsigned int CShaderManager::getShaderSampler( int iShader, int iTexture )
{
    char szTextureName[32];
    snprintf( szTextureName, sizeof( szTextureName ), "texture%d", iTexture );
    
    int iUniformLocation = glGetUniformLocation( iShader, szTextureName );
    
    return iUniformLocation;
}

/*
**
*/
void CShaderManager::loadAllPrograms( const char* szFileName,
									  void (*pfnBeforeLinkFunc)( GLuint iShader ) )
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
            TiXmlNode* pChild = pNode->FirstChild();
            
            const char* szVertexShader = NULL;
            const char* szFragmentShader = NULL;
            
            tShaderProgram* pProgram = &maShaderPrograms[miNumShaderPrograms++];
            while( pChild )
            {
                const char* szProperty = pChild->Value();
                if( !strcmp( szProperty, "name" ) )
                {
                    const char* szName = pChild->FirstChild()->Value();
                    strncpy( pProgram->mszName, szName, sizeof( pProgram->mszName ) );
                    pProgram->miHash = hash( szName );
                }
                else if( !strcmp( szProperty, "vertex_shader" ) )
                {
                    szVertexShader = pChild->FirstChild()->Value();
                }
                else if( !strcmp( szProperty, "fragment_shader" ) )
                {
                    szFragmentShader = pChild->FirstChild()->Value();
                }
                else if( !strcmp( szProperty, "texture_0" ) )
                {
                    const char* szTexture = pChild->FirstChild()->Value();
                    if( strcmp( szTexture, "user" ) )
                    {
                        strncpy( pProgram->maszTextures[0], szTexture, sizeof( pProgram->maszTextures[0] ) );
                    }
                }
                else if( !strcmp( szProperty, "texture_1" ) )
                {
                    const char* szTexture = pChild->FirstChild()->Value();
                    if( strcmp( szTexture, "user" ) )
                    {
                        strncpy( pProgram->maszTextures[1], szTexture, sizeof( pProgram->maszTextures[1] ) );
                    }
                }
                else if( !strcmp( szProperty, "texture_2" ) )
                {
                    const char* szTexture = pChild->FirstChild()->Value();
                    if( strcmp( szTexture, "user" ) )
                    {
                        strncpy( pProgram->maszTextures[2], szTexture, sizeof( pProgram->maszTextures[2] ) );
                    }
                }
                else if( !strcmp( szProperty, "texture_3" ) )
                {
                    const char* szTexture = pChild->FirstChild()->Value();
                    if( strcmp( szTexture, "user" ) )
                    {
                        strncpy( pProgram->maszTextures[3], szTexture, sizeof( pProgram->maszTextures[3] ) );
                    }
                }
                
                pChild = pChild->NextSibling();
            }
            
            bool bLoadedVS = loadShader( szVertexShader, &pProgram->miVertexShader, GL_VERTEX_SHADER );
            bool bLoadedFS = loadShader( szFragmentShader, &pProgram->miFragmentShader, GL_FRAGMENT_SHADER );
            bool bCreatedProgram = createShaderProgram( pProgram, pfnBeforeLinkFunc );

            WTFASSERT2( bLoadedVS, "can't compile %s", szVertexShader );
            WTFASSERT2( bLoadedFS, "can't compile %s", szFragmentShader );
            WTFASSERT2( bCreatedProgram, "can't create shader program vs: %s fs: %s", szVertexShader, szFragmentShader );
            
			pProgram->miHash = hash( pProgram->mszName );

            pNode = pNode->NextSibling();
        }
    }
    else
    {
        WTFASSERT2( 0, "can't load %s error: %s", szFileName, doc.ErrorDesc() );
    }
}

/*
**
*/
bool CShaderManager::loadProgram( const char* szName, 
								  const char* szVertexShader, 
								  const char* szFragmentShader,
								  void (*pfnBeforeLinkFunc)( GLuint iShader ) )
{
	if( miNumShaderPrograms + 1 >= miNumShaderProgramAlloc )
	{
		miNumShaderProgramAlloc += NUM_SHADERS_PER_ALLOC;
		maShaderPrograms = (tShaderProgram *)REALLOC( maShaderPrograms, sizeof( tShaderProgram ) * miNumShaderProgramAlloc );
	}

	tShaderProgram* pProgram = &maShaderPrograms[miNumShaderPrograms++];
	
	strncpy( pProgram->mszName, szName, sizeof( pProgram->mszName ) );
	bool bLoadedVS = loadShader( szVertexShader, &pProgram->miVertexShader, GL_VERTEX_SHADER );
	bool bLoadedFS = loadShader( szFragmentShader, &pProgram->miFragmentShader, GL_FRAGMENT_SHADER );
	bool bCreatedProgram = createShaderProgram( pProgram, pfnBeforeLinkFunc );

    pProgram->miHash = hash( szName );

	return ( bLoadedVS && bLoadedFS && bCreatedProgram );
}

/*
**
*/
static bool createShaderProgram( tShaderProgram* pProgram,
								 void (*pfnBeforeLinkFunc)( GLuint iShader ) )
{
	OUTPUT( "SHADER: %s\n", pProgram->mszName );

	pProgram->miID = glCreateProgram();
	if( pProgram->miID == 0 )
	{
		return false;
	}

	glAttachShader( pProgram->miID, pProgram->miVertexShader );
	glAttachShader( pProgram->miID, pProgram->miFragmentShader );

	if( pfnBeforeLinkFunc )
	{
		pfnBeforeLinkFunc( pProgram->miID );
	}

	GLint linked;
	glLinkProgram( pProgram->miID );
	glGetProgramiv( pProgram->miID, GL_LINK_STATUS, &linked );
	if( !linked )
	{
		GLint infoLength = 0;
		glGetProgramiv( pProgram->miID, GL_INFO_LOG_LENGTH, &infoLength );
		if( infoLength > 1 )
		{
			char* szInfo = (char *)MALLOC( sizeof( char ) * infoLength );
			glGetProgramInfoLog( pProgram->miID, infoLength, NULL, szInfo );
			OUTPUT( "Error linking program:\n%s\n", szInfo );
			FREE( szInfo );
		}

		glDeleteProgram( pProgram->miID );
		return false;
	}
    
	// get shader attributes
    GLint iActiveAttribs = 0;
    glGetProgramiv( pProgram->miID, GL_ACTIVE_ATTRIBUTES, &iActiveAttribs );
    
	pProgram->miNumAttributes = (int)iActiveAttribs;
	pProgram->maiShaderAttributes = (int *)MALLOC( sizeof( int ) * pProgram->miNumAttributes );
	pProgram->maiAttribTypes = (GLenum *)MALLOC( sizeof( GLenum ) * pProgram->miNumAttributes );
	pProgram->maszAttributes = (char **)MALLOC( sizeof( char* ) * pProgram->miNumAttributes );
	pProgram->maiAttributeHashes = (int *)MALLOC( sizeof( int ) * pProgram->miNumAttributes );

    for( int i = 0; i < iActiveAttribs; i++ )
    {
		pProgram->maszAttributes[i] = (char *)MALLOC( sizeof( char ) * 128 );
		memset( pProgram->maszAttributes[i], 0, sizeof( char ) * 128 );

        GLint iSize;
       
        glGetActiveAttrib( pProgram->miID,
                           i,
                           127,
                           NULL,
                           &iSize,
						   &pProgram->maiAttribTypes[i],
                           pProgram->maszAttributes[i] );
        
        int iAttrib = glGetAttribLocation( pProgram->miID, pProgram->maszAttributes[i] );
		pProgram->maiShaderAttributes[i] = iAttrib;       
		pProgram->maiAttributeHashes[i] = hash( pProgram->maszAttributes[i] );

        glEnableVertexAttribArray( iAttrib );
        OUTPUT( "shader: %d attrib %d = \"%s\" id = %d\n",
                pProgram->miID,
                i,
                pProgram->maszAttributes[i],
				iAttrib );
		
    }   // for i = 0 to num active attribs
	
	// shader uniforms
	glGetProgramiv( pProgram->miID,
					GL_ACTIVE_UNIFORMS,
					&pProgram->miNumUniforms );

	pProgram->maiShaderUniforms = (int *)MALLOC( sizeof( int ) * pProgram->miNumUniforms );
	pProgram->maszUniforms = (char **)MALLOC( sizeof( char* ) * pProgram->miNumUniforms );
	pProgram->maiUniformTypes = (GLenum *)MALLOC( sizeof( GLenum ) * pProgram->miNumUniforms );
	pProgram->maiUniformHashes = (int *)MALLOC( sizeof( int ) * pProgram->miNumUniforms );

	for( int i = 0; i < pProgram->miNumUniforms; i++ )
	{
		int iNameLength;
		int iSize = 0;

		pProgram->maszUniforms[i] = (char *)MALLOC( sizeof( char ) * 128 );
		
		memset( pProgram->maszUniforms[i], 0, 128 );
		glGetActiveUniform( pProgram->miID,
							i,
							127,
							&iNameLength,
							&iSize,
							&pProgram->maiUniformTypes[i],
							pProgram->maszUniforms[i] );
		
		pProgram->maiShaderUniforms[i] = glGetUniformLocation( pProgram->miID, pProgram->maszUniforms[i] );
		pProgram->maiUniformHashes[i] = hash( pProgram->maszUniforms[i] );

		OUTPUT( "shader: %d uniform %d = \"%s\" id = %d\n",
                pProgram->miID,
                i,
                pProgram->maszUniforms[i],
				pProgram->maiShaderUniforms[i] );
		
	}	// for i = 0 to num uniforms

	return true;
}

/*
**
*/
static bool loadShader( const char* szName, GLuint* piShader, GLenum type )
{
    char szFullPath[512];
    getFullPath( szFullPath, szName );
    
	FILE* fp = fopen( szFullPath, "rb" );
    //OUTPUT( "%s : %d filename = %s\n", __FILE__, __LINE__, szName );
	assert( fp );
	
	fseek( fp, 0, SEEK_END );
	size_t iPos = ftell( fp );
	fseek( fp, 0, SEEK_SET );
	
	char* szShader = (char *)malloc( iPos + 1 );
	memset( szShader, 0, iPos + 1 );
	fread( szShader, sizeof( char ), iPos, fp );
	fclose( fp );

	*piShader = glCreateShader( type );
	if( *piShader == 0 )
	{
		FREE( szShader );
		return false;
	}
	
    const char* pszShader = szShader;
    
#if defined( MACOS )
    for( ;; )
    {
        const char* szPrecisionStart = strstr( pszShader, "precision" );
        if( szPrecisionStart )
        {
            const char* szPrecisionEnd = strstr( szPrecisionStart, ";" );
            pszShader = szPrecisionEnd + 1;
        }
        else
        {
            break;
        }
    }
#endif // MACOS
    
	glShaderSource( *piShader, 1, (const char**)&pszShader, NULL );
	glCompileShader( *piShader );

	GLint compiled;
	glGetShaderiv( *piShader, GL_COMPILE_STATUS, &compiled); 
	if( !compiled )
	{
		GLint infoLen = 0; 
		glGetShaderiv( *piShader, GL_INFO_LOG_LENGTH, &infoLen ); 
		if( infoLen > 1 ) 
		{ 
			char* infoLog = (char *)MALLOC( sizeof(char) * infoLen ); 
			glGetShaderInfoLog( *piShader, infoLen, NULL, infoLog ); 
			OUTPUT( "Error compiling shader %s:\n%s\n", szName, infoLog );
			FREE( infoLog ); 
	    } 
	    
		glDeleteShader( *piShader ); 
	}
    
	FREE( szShader );
	if( !compiled )
	{
		return false;
	}
	
	return true;
}

/*
**
*/
GLint CShaderManager::getUniformLocation( tShaderProgram const* pShader, const char* szName )
{
	int iNameHash = hash( szName );

	GLint iRet = -1;
	for( int i = 0; i < pShader->miNumUniforms; i++ )
	{
		if( pShader->maiUniformHashes[i] == iNameHash )
		{
			iRet = pShader->maiShaderUniforms[i];
			break;
		}
	};

	return iRet;
}

/*
**
*/
GLint CShaderManager::getAttributeLocation( tShaderProgram const* pShader, const char* szName )
{
	int iNameHash = hash( szName );

	GLint iRet = -1;
	for( int i = 0; i < pShader->miNumAttributes; i++ )
	{
		if( pShader->maiAttributeHashes[i] == iNameHash )
		{
			iRet = pShader->maiShaderAttributes[i];
			break;
		}
	};

	return iRet;
}