#ifndef __TGA_H__
#define __TGA_H__

void saveTGA( const char* szFileName,
              const char* szDirectory,
              int iWidth,
              int iHeight,
              unsigned char* acImage );

void loadTGA( const char* szFileName,
			  const char* szDirectory,
			  int* piImageWidth,
			  int* piImageHeight,
			  unsigned char* acImage );

#endif // __TGA_H__