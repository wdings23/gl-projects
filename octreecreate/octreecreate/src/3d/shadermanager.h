#ifndef __SHADERMANAGER_H__
#define __SHADERMANAGER_H__

#define NUM_SHADERS_PER_ALLOC 10
#define MAX_SHADER_STRING 64

struct ShaderProgram
{
	char			mszName[MAX_SHADER_STRING];

	unsigned int	miShaderProgram;
	unsigned int	miVertexShader;
	unsigned int	miFragmentShader;
    
    char            maszTextures[4][MAX_SHADER_STRING];
    
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
					  const char* szFragmentShader );

    void loadAllPrograms( const char* szFileName );
    
    unsigned int getShaderSampler( int iShader, int iTexture );
    
public:
	static CShaderManager* instance( void );

protected:
	static CShaderManager* mpInstance;

protected:
	tShaderProgram*		maShaderPrograms;
	int					miNumShaderPrograms;
	int					miNumShaderProgramAlloc;

};

#endif // __SHADERMANAGER_H__