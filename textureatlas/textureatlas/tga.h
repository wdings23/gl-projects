#ifndef __TGA_H__
#define __TGA_H__

void saveTGA( const char* szFileName,
              int iWidth,
              int iHeight,
              unsigned char* acImage );

void loadTGA( unsigned char** acImage,
			  const char* szFileName,
			  unsigned int* piImageWidth,
			  unsigned int* piImageHeight );

#endif // __TGA_H__