#ifndef __SHADERMANAGER_H__
#define __SHADERMANAGER_H__

#define NUM_SHADERS_PER_ALLOC 10
#define MAX_SHADER_STRING 64

struct ShaderProgram
{
	char			mszName[MAX_SHADER_STRING];
    char            mszText[256];
    
	unsigned int	miID;
	unsigned int	miVertexShader;
	unsigned int	miFragmentShader;
    
    char            maszTextures[4][MAX_SHADER_STRING];
    
	int				miNumAttributes;
	int*			maiShaderAttributes;
	int*			maiAttributeHashes;
	char**			maszAttributes;
	GLenum*			maiAttribTypes;

	int				miNumUniforms;
	int*			maiShaderUniforms;
	int*			maiUniformHashes;
	char**			maszUniforms;
	GLenum*			maiUniformTypes;	

    int             miHash;
};

typedef struct ShaderProgram tShaderProgram;

class CShaderManager
{
public:
	CShaderManager( void );
	~CShaderManager( void );
	
	unsigned int getShader( const char* szShaderName );
	bool loadProgram( const char* szName, 
					  const char* szVertexShader, 
					  const char* szFragmentShader,
					  void (*pfnBeforeLinkFunc)( GLuint iShader ) = NULL );

    void loadAllPrograms( const char* szFileName,
						  void (*pfnBeforeLinkFunc)( GLuint iShader ) = NULL );
    
    unsigned int getShaderSampler( int iShader, int iTexture );
    tShaderProgram const* getShaderProgram( const char* szShaderName );

    inline int getNumShaderPrograms( void ) { return miNumShaderPrograms; }
    inline tShaderProgram const* getShaderFromIndex( int iIndex ) { return &maShaderPrograms[iIndex]; }
    
    tShaderProgram const* getShaderProgramWithShaderID( GLuint iShaderID );
    
public:
	static CShaderManager* instance( void );

	static GLint getUniformLocation( tShaderProgram const* pShader, const char* szName );
	static GLint getAttributeLocation( tShaderProgram const* pShader, const char* szName );

protected:
	static CShaderManager* mpInstance;

protected:
	tShaderProgram*		maShaderPrograms;
	int					miNumShaderPrograms;
	int					miNumShaderProgramAlloc;

};

#endif // __SHADERMANAGER_H__