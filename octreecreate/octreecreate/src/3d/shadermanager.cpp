#include "ShaderManager.h"
#include "fileutil.h"
#include "hashutil.h"
#include "tinyxml.h"



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

static bool loadShader( const char* szName, GLuint* piShader, GLenum type );
static bool createShaderProgram( GLuint iVertexShader, GLuint iPixelShader, GLuint* piProgram );

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
        glDeleteProgram( maShaderPrograms[i].miShaderProgram );
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
			return maShaderPrograms[i].miShaderProgram;
		}

	}	// for i = 0 to num shader programs

	return 0;
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
void CShaderManager::loadAllPrograms( const char* szFileName )
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
            bool bCreatedProgram = createShaderProgram( pProgram->miVertexShader, pProgram->miFragmentShader, &pProgram->miShaderProgram );

            WTFASSERT2( bLoadedVS, "can't compile %s", szVertexShader );
            WTFASSERT2( bLoadedFS, "can't compile %s", szFragmentShader );
            WTFASSERT2( bCreatedProgram, "can't create shader program vs: %s fs: %s", szVertexShader, szFragmentShader );
            
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
								  const char* szFragmentShader )
{
	if( miNumShaderPrograms + 1 >= miNumShaderProgramAlloc )
	{
		miNumShaderProgramAlloc += NUM_SHADERS_PER_ALLOC;
		maShaderPrograms = (tShaderProgram *)realloc( maShaderPrograms, sizeof( tShaderProgram ) * miNumShaderProgramAlloc );
	}

	tShaderProgram* pProgram = &maShaderPrograms[miNumShaderPrograms++];
	
	strncpy( pProgram->mszName, szName, sizeof( pProgram->mszName ) );
	bool bLoadedVS = loadShader( szVertexShader, &pProgram->miVertexShader, GL_VERTEX_SHADER );
	bool bLoadedFS = loadShader( szFragmentShader, &pProgram->miFragmentShader, GL_FRAGMENT_SHADER );
	bool bCreatedProgram = createShaderProgram( pProgram->miVertexShader, pProgram->miFragmentShader, &pProgram->miShaderProgram );

    pProgram->miHash = hash( szName );

	return ( bLoadedVS && bLoadedFS && bCreatedProgram );
}

/*
**
*/
static bool createShaderProgram( GLuint iVertexShader, GLuint iFragmentShader, GLuint* piProgram )
{
	*piProgram = glCreateProgram();
	if( *piProgram == 0 )
	{
		return false;
	}

	glAttachShader( *piProgram, iVertexShader );
	glAttachShader( *piProgram, iFragmentShader );

	GLint linked;
	glLinkProgram( *piProgram );
	glGetProgramiv( *piProgram, GL_LINK_STATUS, &linked );
	if( !linked )
	{
		GLint infoLength = 0;
		glGetProgramiv( *piProgram, GL_INFO_LOG_LENGTH, &infoLength );
		if( infoLength > 1 )
		{
			char* szInfo = (char *)malloc( sizeof( char ) * infoLength );
			glGetProgramInfoLog( *piProgram, infoLength, NULL, szInfo );
			OUTPUT( "Error linking program:\n%s\n", szInfo );
			free( szInfo );
		}

		glDeleteProgram( *piProgram );
		return false;
	}
    
    GLint iActiveAttribs = 0;
    glGetProgramiv( *piProgram, GL_ACTIVE_ATTRIBUTES, &iActiveAttribs );
    
    char szAttribName[256];
    for( unsigned int i = 0; i < iActiveAttribs; i++ )
    {
        GLint iSize;
        GLenum eType;
        
        glGetActiveAttrib( *piProgram,
                          i,
                          sizeof( szAttribName ),
                          NULL,
                          &iSize,
                          &eType,
                          szAttribName );
        
        int iAttrib = glGetAttribLocation( *piProgram, szAttribName );
        
        glEnableVertexAttribArray( iAttrib );
        OUTPUT( "%s : %d shader: %d attrib %d = %s\n",
                __PRETTY_FUNCTION__,
                __LINE__,
                *piProgram,
                i,
                szAttribName );
        
    }   // for i = 0 to num active attribs

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
    //OUTPUT( "%s : %d filename = %s fp = 0x%08X\n", __FILE__, __LINE__, szName, (unsigned int)fp );
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
		free( szShader );
		return false;
	}
	
	glShaderSource( *piShader, 1, (const char**)&szShader, NULL );
	glCompileShader( *piShader );

	GLint compiled;
	glGetShaderiv( *piShader, GL_COMPILE_STATUS, &compiled); 
	if( !compiled )
	{
		GLint infoLen = 0; 
		glGetShaderiv( *piShader, GL_INFO_LOG_LENGTH, &infoLen ); 
		if( infoLen > 1 ) 
		{ 
			char* infoLog = (char *)malloc( sizeof(char) * infoLen ); 
			glGetShaderInfoLog( *piShader, infoLen, NULL, infoLog ); 
			OUTPUT( "Error compiling shader:\n%s\n", infoLog ); 
			free( infoLog ); 
	    } 
	    
		glDeleteShader( *piShader ); 
	}
    
	free( szShader );
	if( !compiled )
	{
		return false;
	}
	
	return true;
}