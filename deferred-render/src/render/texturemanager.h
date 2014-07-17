#ifndef __CTEXTUREMANAGER_H__
#define __CTEXTUREMANAGER_H__

#define MAX_TEXTURE_STRING 256
#define MAX_TEXTURES_IN_MANAGER 500
#define MAX_TEXTURE_SLOTS 200

struct Texture
{
	int			miWidth;
	int			miHeight;
    
    int         miGLWidth;
    int         miGLHeight;
    
    int         miBPP;
    
	int			miID;
	char		mszName[MAX_TEXTURE_STRING];
	double		mfLastAccessed;
	bool		mbInMem;
    
    bool        mbCompressed;
    int         miHash;
    
    bool        mbCubeMap;
    
    GLint       miTextureEdgeTypeU;
    GLint       miTextureEdgeTypeV;
};

typedef struct Texture tTexture;

struct TextureSlot
{
	unsigned int*		mpiGLTexID;
	tTexture*			mpTexture;
};

typedef struct TextureSlot tTextureSlot;

class CTextureManager
{
public:
	CTextureManager( void );
	~CTextureManager( void );

	void init( void );
	void release( void );
	
	tTexture* getTexture( const char* szTextureName, bool bLoadInMem = true );
	void registerTexture( const char* szTextureName, bool bCubeMap = false, GLint iEdgeTypeU = GL_CLAMP_TO_EDGE, GLint iEdgeTypeV = GL_CLAMP_TO_EDGE );
	void registerTextureWithData( const char* szTextureName, 
                                  unsigned char* acImage,
                                  int iWidth,
                                  int iHeight, 
                                  GLint iEdgeTypeU = GL_CLAMP_TO_EDGE, 
                                  GLint iEdgeTypeV = GL_CLAMP_TO_EDGE );
    
    void loadTextureInMem( tTexture* pTexture, double fCurrTime );
	void swapInTexture( const char* szTextureName, tTexture* pTexture, unsigned int iSwapTex );
	void reloadTexture( tTexture* pTexture );
    
    void purgeTextures( void );
    inline void setPurgeTexturesFlag( bool bPurge ) { mbPurgeTextures = bPurge; }
    inline bool getPurgeTexturesFlag( void ) { return mbPurgeTextures; }
    
    bool getSemaphore( void ) { return mbSemaphore; }
    void setSemaphore( bool bSemaphore ) { mbSemaphore = bSemaphore; }

	void reloadTextureData( const char* szTextureName, unsigned char const* acData );
    
	void releaseTexture( tTexture* pTexture );

public:
	static CTextureManager* instance( void );

protected:
	static CTextureManager* mpInstance;
	

protected:
	tTexture			maTextures[MAX_TEXTURES_IN_MANAGER];
	unsigned int		maiTextureIDs[MAX_TEXTURES_IN_MANAGER];
	tTextureSlot		maTextureSlots[MAX_TEXTURE_SLOTS];
	int					miNumTextures;
    
    bool                mbSemaphore;
    bool                mbPurgeTextures;
    
public:
    static void readTarga( const char* pszFileName,
                           int* iWidth,
                           int* iHeight,
                          unsigned char** aImage );
};

#endif // __CTEXTUREMANAGER_H__