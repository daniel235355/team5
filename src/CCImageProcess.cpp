#include "stdafx.h"
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "CCImageProcess.h"
#include "CCImageIO.h"

#ifdef _MSC_VER // Windows
#else // Linux
#define _I64_MAX      9223372036854775807
#endif /*_MSC_VER*/
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))


#define PIX_SORT( a, b) { if ((a)>(b)) PIX_SWAP((a),(b)); }
#define PIX_SWAP( a, b) { short temp=(a);(a)=(b);(b)=temp; }

void CCSobel( unsigned char *a_pucDstImageBuf, unsigned char *a_pucSrcImageBuf, unsigned short a_uwWidth, unsigned short a_uwHeight)
{
    int x, y;

    unsigned short uwMirrorWidth = a_uwWidth+2;
    unsigned short uwMirrorHeight = a_uwHeight+2;
    unsigned char *pucMirrorBuf = new unsigned char[uwMirrorWidth*uwMirrorHeight];
    CCMirror( pucMirrorBuf, a_pucSrcImageBuf, uwMirrorWidth, uwMirrorHeight, a_uwWidth, a_uwHeight, 1, 1, 1, 1);

    unsigned char *pucSrcBuf = pucMirrorBuf;
    unsigned char *pucDstBuf = a_pucDstImageBuf;
    for ( y=0; y<a_uwHeight; y++)
    {
        for ( x=0; x<a_uwWidth; x++)
        {
            unsigned short uwSum = abs((pucSrcBuf[2]+2*pucSrcBuf[uwMirrorWidth+2]+pucSrcBuf[uwMirrorWidth*2+2])-(pucSrcBuf[0]+2*pucSrcBuf[uwMirrorWidth]+pucSrcBuf[uwMirrorWidth*2]))
                +abs((pucSrcBuf[uwMirrorWidth*2]+2*pucSrcBuf[uwMirrorWidth*2+1]+pucSrcBuf[uwMirrorWidth*2+2])-(pucSrcBuf[0]+2*pucSrcBuf[1]+pucSrcBuf[2]));
            *pucDstBuf = uwSum*255/1530;
            pucSrcBuf++;
            pucDstBuf++;
        }
        pucSrcBuf+=2;
    }

    delete [] pucMirrorBuf;
}

void CCLoG( unsigned char *a_pucDstImageBuf, unsigned char *a_pucSrcImageBuf, unsigned short a_uwWidth, unsigned short a_uwHeight, float a_fSigma)
{
    //calculate mask size
    float ftemp, fTotal;
    unsigned char ucSize = 0;
    int x, y;
    do 
    {
        fTotal = 0;
        for ( y=-ucSize; y<=ucSize; y++)
        {
            for ( x=-ucSize; x<=ucSize; x++)
            {
                ftemp = (x*x+y*y)/(2*a_fSigma*a_fSigma);
                ftemp = (float)((-1/(3.1415926f*pow( a_fSigma, 4)))*(1-ftemp)*exp(-ftemp));
                fTotal+=ftemp;
            }
        }

        //weighting sum ~= 0
        if ( fabs(fTotal*255)<0.5f )
            break;

        ucSize++;
    } while (true);

    float *pfMask = new float[(ucSize*2+1)*(ucSize*2+1)];
    for ( y=-ucSize; y<=ucSize; y++)
    {
        for ( x=-ucSize; x<=ucSize; x++)
        {
            ftemp = (x*x+y*y)/(2*a_fSigma*a_fSigma);
            ftemp = (float)((-1/(3.1415926f*pow( a_fSigma, 4)))*(1-ftemp)*exp(-ftemp));
            pfMask[(y+ucSize)*(ucSize*2+1)+(x+ucSize)] = ftemp;
        }
    }

    unsigned char ucFullSize = ucSize*2+1;
    unsigned char ucHalfSize = ucSize;
    unsigned short uwMirrorWidth = a_uwWidth+ucHalfSize*2;
    unsigned short uwMirrorHeight = a_uwHeight+ucHalfSize*2;
    unsigned char *pucMirrorBuf = new unsigned char[uwMirrorWidth*uwMirrorHeight];
    CCMirror( pucMirrorBuf, a_pucSrcImageBuf, uwMirrorWidth, uwMirrorHeight, a_uwWidth, a_uwHeight, ucHalfSize, ucHalfSize, ucHalfSize, ucHalfSize);
    
    short *pwLoGBuf = new short[a_uwWidth*a_uwHeight];
    unsigned char *pucSrcBuf = pucMirrorBuf;
    short *pwDstBuf = pwLoGBuf;
    
    int m, n;
    
    for ( y=0; y<a_uwHeight; y++)
    {
        for ( x=0; x<a_uwWidth; x++)
        {
            float *ppfMask = pfMask;
            float fSum = 0;
            for ( m=0; m<ucFullSize; m++)
            {
                for ( n=0; n<ucFullSize; n++)
                {
                    fSum+=ppfMask[0]*pucSrcBuf[0];
                    ppfMask++;
                    pucSrcBuf++;
                }
                pucSrcBuf+=(uwMirrorWidth-ucFullSize);
            }
            
            if ( fSum>=0 )
                pwDstBuf[0] = (short)(fSum+0.5f);
            else
                pwDstBuf[0] = (short)(fSum-0.5f);
            
            pucSrcBuf-=(uwMirrorWidth*ucFullSize-1);
            pwDstBuf++;
        }
        pucSrcBuf+=ucHalfSize*2;
    }

    for ( y=0; y<a_uwHeight; y++)
    {
        for ( x=0; x<a_uwWidth; x++)
        {
            a_pucDstImageBuf[y*a_uwWidth+x] = pwLoGBuf[y*a_uwWidth+x]+128;
        }
    }

    delete [] pfMask;
    delete [] pucMirrorBuf;
    delete [] pwLoGBuf;
}

void CCMaskCalculation( short *a_pwDstImageBuf, unsigned char *a_pucSrcImageBuf, unsigned short a_uwWidth, unsigned short a_uwHeight, short *a_pwMask, unsigned char a_ucSize)
{
    unsigned char ucHalfSize = a_ucSize/2;
    unsigned short uwMirrorWidth = a_uwWidth+ucHalfSize*2;
    unsigned short uwMirrorHeight = a_uwHeight+ucHalfSize*2;
    unsigned char *pucMirrorBuf = new unsigned char[uwMirrorWidth*uwMirrorHeight];
    CCMirror( pucMirrorBuf, a_pucSrcImageBuf, uwMirrorWidth, uwMirrorHeight, a_uwWidth, a_uwHeight, ucHalfSize, ucHalfSize, ucHalfSize, ucHalfSize);

    unsigned char *pucSrcBuf = pucMirrorBuf;
    short *pwDstBuf = a_pwDstImageBuf;
    short *pwMask = a_pwMask;
    
    int x, y, m, n;
    
    for ( y=0; y<a_uwHeight; y++)
    {
        for ( x=0; x<a_uwWidth; x++)
        {
            short *pwMask = a_pwMask;
            int dSum = 0;
            for ( m=0; m<a_ucSize; m++)
            {
                for ( n=0; n<a_ucSize; n++)
                {
                    dSum+=pwMask[0]*pucSrcBuf[0];
                    pwMask++;
                    pucSrcBuf++;
                }
                pucSrcBuf+=(uwMirrorWidth-a_ucSize);
            }
            
            pwDstBuf[0] = (short)dSum;

            pucSrcBuf-=(uwMirrorWidth*a_ucSize-1);
            pwDstBuf++;
        }
        pucSrcBuf+=ucHalfSize*2;
    }

    delete [] pucMirrorBuf;
}

void CCGaussianPyramid( unsigned char *a_pucDstImageBuf, unsigned char *a_pucSrcImageBuf, unsigned short a_uwWidth, unsigned short a_uwHeight)
{
    unsigned short uwDstWidth = a_uwWidth/2;
    unsigned short uwDstHeight = a_uwHeight/2;

    unsigned short uwMirrorWidth = a_uwWidth+4;
    unsigned short uwMirrorHeight = a_uwHeight+4;
    unsigned char *pucMirrorBuf = new unsigned char[uwMirrorWidth*uwMirrorHeight];    
    CCMirror( pucMirrorBuf, a_pucSrcImageBuf, uwMirrorWidth, uwMirrorHeight, a_uwWidth, a_uwHeight, 2, 2, 2, 2);
    
    unsigned char aucWeight[5] = {1,4,6,4,1};

    unsigned char *pucHorizontalBuf = new unsigned char[uwDstWidth*uwMirrorHeight];
    unsigned char *pucSrcBuf = pucMirrorBuf;
    unsigned char *pucDstBuf = pucHorizontalBuf;
    int x, y;
    for ( y=0; y<uwMirrorHeight; y++)
    {
        for ( x=0; x<uwDstWidth; x++)
        {
            *pucDstBuf = ((unsigned short)aucWeight[0]*pucSrcBuf[2*x+0]
                +aucWeight[1]*pucSrcBuf[2*x+1]
                +aucWeight[2]*pucSrcBuf[2*x+2]
                +aucWeight[3]*pucSrcBuf[2*x+3]
                +aucWeight[4]*pucSrcBuf[2*x+4])/16;

            pucDstBuf++;
        }
        pucSrcBuf+=uwMirrorWidth;
    }
    delete [] pucMirrorBuf;

    pucSrcBuf = pucHorizontalBuf;
    pucDstBuf = a_pucDstImageBuf;
    for ( y=0; y<uwDstHeight; y++)
    {
        for ( x=0; x<uwDstWidth; x++)
        {
            *pucDstBuf = ((unsigned short)aucWeight[0]*pucSrcBuf[2*y*uwDstWidth+x]
                +aucWeight[1]*pucSrcBuf[2*y*uwDstWidth+uwDstWidth+x]
                +aucWeight[2]*pucSrcBuf[2*y*uwDstWidth+uwDstWidth*2+x]
                +aucWeight[3]*pucSrcBuf[2*y*uwDstWidth+uwDstWidth*3+x]
                +aucWeight[4]*pucSrcBuf[2*y*uwDstWidth+uwDstWidth*4+x])/16;

            pucDstBuf++;
        }
    }

    delete [] pucHorizontalBuf;
}

void CCBinning( unsigned char *a_pucDstImageBuf, unsigned char *a_pucSrcImageBuf, unsigned short a_uwSrcWidth, unsigned short a_uwSrcHeight,
               unsigned char a_ucWidthRatio, unsigned char a_ucHeightRatio)
{
    int i, j, m, n;
    unsigned short uwDstWidth = a_uwSrcWidth/a_ucWidthRatio;
    unsigned short uwDstHeight = a_uwSrcHeight/a_ucHeightRatio;
    unsigned short uwNum = a_ucWidthRatio*a_ucHeightRatio;
    unsigned short uwNum_2 = uwNum/2;
    
    unsigned short uwJumpWidth = a_uwSrcWidth*a_ucHeightRatio-a_ucWidthRatio;
    
    unsigned char *pucSrcImageBuf = a_pucSrcImageBuf;
    unsigned char *pucDstImageBuf = a_pucDstImageBuf;
    
    for ( i=0; i<uwDstHeight; i++)
    {
        pucSrcImageBuf = a_pucSrcImageBuf+a_uwSrcWidth*a_ucHeightRatio*i;
        
        for ( j=0; j<uwDstWidth; j++)
        {
            unsigned int udSum = 0;
            for ( m=0; m<a_ucHeightRatio; m++)
            {
                for ( n=0; n<a_ucWidthRatio; n++)
                {
                    udSum+=(*pucSrcImageBuf);
                    pucSrcImageBuf++;
                }
                pucSrcImageBuf+=(a_uwSrcWidth-a_ucWidthRatio);
            }
            
            *pucDstImageBuf = (unsigned char)((udSum+uwNum_2)/uwNum);
            
            pucSrcImageBuf-=uwJumpWidth;
            pucDstImageBuf++;
        }
    }
}

void CCBinning16(unsigned short *a_puwDstImageBuf, unsigned short *a_puwSrcImageBuf, unsigned short a_uwSrcWidth, unsigned short a_uwSrcHeight,
    unsigned char a_ucWidthRatio, unsigned char a_ucHeightRatio)
{
    int i, j, m, n;
    unsigned short uwDstWidth = a_uwSrcWidth / a_ucWidthRatio;
    unsigned short uwDstHeight = a_uwSrcHeight / a_ucHeightRatio;
    unsigned short uwNum = a_ucWidthRatio*a_ucHeightRatio;
    unsigned short uwNum_2 = uwNum / 2;

    unsigned short uwJumpWidth = a_uwSrcWidth*a_ucHeightRatio - a_ucWidthRatio;

    unsigned short *puwSrcImageBuf = a_puwSrcImageBuf;
    unsigned short *puwDstImageBuf = a_puwDstImageBuf;

    for (i = 0; i<uwDstHeight; i++)
    {
        puwSrcImageBuf = a_puwSrcImageBuf + a_uwSrcWidth*a_ucHeightRatio*i;

        for (j = 0; j<uwDstWidth; j++)
        {
            unsigned int udSum = 0;
            for (m = 0; m<a_ucHeightRatio; m++)
            {
                for (n = 0; n<a_ucWidthRatio; n++)
                {
                    udSum += (*puwSrcImageBuf);
                    puwSrcImageBuf++;
                }
                puwSrcImageBuf += (a_uwSrcWidth - a_ucWidthRatio);
            }

            *puwDstImageBuf = (unsigned short)((udSum + uwNum_2) / uwNum);

            puwSrcImageBuf -= uwJumpWidth;
            puwDstImageBuf++;
        }
    }
}

short opt_med9( short *p);
short opt_med25( short *p);
void CCMedianFilter5x5( short *a_pwDstImageBuf, short *a_pwSrcImageBuf, unsigned short a_uwImageWidth, unsigned short a_uwImageHeight)
{
    short *pwTempBuf = new short[a_uwImageWidth*a_uwImageHeight];
    
    memcpy( pwTempBuf, a_pwSrcImageBuf, sizeof(short)*a_uwImageWidth*a_uwImageHeight);
    //create a sliding window of size 25
    short awWindow[25];
    
    short *pwSrcImageBuf = a_pwSrcImageBuf;//first pixel of window
    short *pwTemp1Buf = pwTempBuf+2*a_uwImageWidth+2;
    
    unsigned short uwWidth_x4 = a_uwImageWidth*4-1;
    
    int x, y;
    for( y=2; y<a_uwImageHeight-2; y++)
    {
        for(x=2; x<a_uwImageWidth-2; x++)
        {
            awWindow[0] = pwSrcImageBuf[0];
            awWindow[1] = pwSrcImageBuf[1];
            awWindow[2] = pwSrcImageBuf[2];
            awWindow[3] = pwSrcImageBuf[3];
            awWindow[4] = pwSrcImageBuf[4];
            pwSrcImageBuf+=a_uwImageWidth;
            awWindow[5] = pwSrcImageBuf[0];
            awWindow[6] = pwSrcImageBuf[1];
            awWindow[7] = pwSrcImageBuf[2];
            awWindow[8] = pwSrcImageBuf[3];
            awWindow[9] = pwSrcImageBuf[4];
            pwSrcImageBuf+=a_uwImageWidth;
            awWindow[10] = pwSrcImageBuf[0];
            awWindow[11] = pwSrcImageBuf[1];
            awWindow[12] = pwSrcImageBuf[2];
            awWindow[13] = pwSrcImageBuf[3];
            awWindow[14] = pwSrcImageBuf[4];
            pwSrcImageBuf+=a_uwImageWidth;
            awWindow[15] = pwSrcImageBuf[0];
            awWindow[16] = pwSrcImageBuf[1];
            awWindow[17] = pwSrcImageBuf[2];
            awWindow[18] = pwSrcImageBuf[3];
            awWindow[19] = pwSrcImageBuf[4];
            pwSrcImageBuf+=a_uwImageWidth;
            awWindow[20] = pwSrcImageBuf[0];
            awWindow[21] = pwSrcImageBuf[1];
            awWindow[22] = pwSrcImageBuf[2];
            awWindow[23] = pwSrcImageBuf[3];
            awWindow[24] = pwSrcImageBuf[4];
            
            
            // sort the window to find median
            opt_med25(awWindow);
            
            
            // assign the median to centered element of the matrix
            pwTemp1Buf[0] = awWindow[12];
            
            pwTemp1Buf++;
            pwSrcImageBuf-=uwWidth_x4;
        }
        pwTemp1Buf+=4;
        pwSrcImageBuf+=4;
    }
    
    memcpy( a_pwDstImageBuf, pwTempBuf, sizeof(short)*a_uwImageWidth*a_uwImageHeight);
    
    delete [] pwTempBuf;
}

void CCMedianFilter( short *a_pwDstImageBuf, short *a_pwSrcImageBuf, unsigned short a_uwImageWidth, unsigned short a_uwImageHeight)
{
    short *pwTempBuf = new short[a_uwImageWidth*a_uwImageHeight];
    
    memcpy( pwTempBuf, a_pwSrcImageBuf, sizeof(short)*a_uwImageWidth*a_uwImageHeight);
    //create a sliding window of size 9
    short awWindow[9];
    
    int x, y;
    for( y=1; y<a_uwImageHeight-1; y++)
    {
        for(x=1; x<a_uwImageWidth-1; x++)
        {
            // Pick up window element
            awWindow[0] = a_pwSrcImageBuf[(y-1)*a_uwImageWidth+x-1];
            awWindow[1] = a_pwSrcImageBuf[(y-1)*a_uwImageWidth+x];
            awWindow[2] = a_pwSrcImageBuf[(y-1)*a_uwImageWidth+x+1];
            awWindow[3] = a_pwSrcImageBuf[y*a_uwImageWidth+x-1];
            awWindow[4] = a_pwSrcImageBuf[y*a_uwImageWidth+x];
            awWindow[5] = a_pwSrcImageBuf[y*a_uwImageWidth+x+1];
            awWindow[6] = a_pwSrcImageBuf[(y+1)*a_uwImageWidth+x-1];
            awWindow[7] = a_pwSrcImageBuf[(y+1)*a_uwImageWidth+x];
            awWindow[8] = a_pwSrcImageBuf[(y+1)*a_uwImageWidth+x+1];
            
            // sort the window to find median
            opt_med9(awWindow);
            
            // assign the median to centered element of the matrix
            pwTempBuf[y*a_uwImageWidth+x] = awWindow[4];
        }
    }
    
    memcpy( a_pwDstImageBuf, pwTempBuf, sizeof(short)*a_uwImageWidth*a_uwImageHeight);
    delete [] pwTempBuf;
}

void CCInsertionSort( short *a_pwWindow, unsigned short a_uwNum)
{
    int i , j;
    short temp;
    for( i=0; i<a_uwNum; i++)
    {
        temp = a_pwWindow[i];
        for( j=i-1; j>=0 && temp<a_pwWindow[j]; j--)
        {
            a_pwWindow[j+1] = a_pwWindow[j];
        }
        a_pwWindow[j+1] = temp;
    }
}

void CCMirror( unsigned char *a_pucDstImageBuf, unsigned char *a_pucSrcImageBuf,
                unsigned short a_uwDstImageWidth, unsigned short a_uwDstImageHeight,
                unsigned short a_uwSrcImageWidth, unsigned short a_uwSrcImageHeight, 
                unsigned char a_ucLeft, unsigned char a_ucRight, unsigned char a_ucTop, unsigned char a_ucBottom)
{
    int i, j;
    
    unsigned char *pucSrcImageBuf;
    unsigned char *pucDstImageBuf;
    unsigned short uwTopLine, uwBottomLine, uwLeftLine, uwRightLine;
    uwTopLine = a_ucTop;
    uwBottomLine = a_ucBottom;
    uwLeftLine = a_ucLeft;
    uwRightLine = a_ucRight;
    
    pucSrcImageBuf = a_pucSrcImageBuf;
    pucDstImageBuf = a_pucDstImageBuf+uwTopLine*a_uwDstImageWidth+uwLeftLine;
    for ( i=0; i<a_uwSrcImageHeight; i++)
    {
        memcpy( pucDstImageBuf, pucSrcImageBuf, a_uwSrcImageWidth);
        pucSrcImageBuf+=a_uwSrcImageWidth;
        pucDstImageBuf+=a_uwDstImageWidth;
    }
    
    //mirror left
    pucSrcImageBuf = a_pucSrcImageBuf+1;
    pucDstImageBuf = a_pucDstImageBuf+uwTopLine*a_uwDstImageWidth+uwLeftLine-1;
    for ( i=0; i<a_uwSrcImageHeight; i++)
    {
        for ( j=0; j<uwLeftLine; j++)
        {
            *pucDstImageBuf = *pucSrcImageBuf;
            pucSrcImageBuf++;
            pucDstImageBuf--;
        }
        pucSrcImageBuf+=(a_uwSrcImageWidth-uwLeftLine);
        pucDstImageBuf+=(a_uwDstImageWidth+uwLeftLine);
    }
    
    //mirror right
    pucSrcImageBuf = a_pucSrcImageBuf+a_uwSrcImageWidth-2;
    pucDstImageBuf = a_pucDstImageBuf+uwTopLine*a_uwDstImageWidth+uwLeftLine+a_uwSrcImageWidth;
    for ( i=0; i<a_uwSrcImageHeight; i++)
    {
        for ( j=0; j<uwRightLine; j++)
        {
            *pucDstImageBuf = *pucSrcImageBuf;
            pucSrcImageBuf--;
            pucDstImageBuf++;
        }
        pucSrcImageBuf+=(a_uwSrcImageWidth+uwRightLine);
        pucDstImageBuf+=(a_uwDstImageWidth-uwRightLine);
    }
    
    //mirror top
    pucSrcImageBuf = a_pucDstImageBuf+(uwTopLine+1)*a_uwDstImageWidth;
    pucDstImageBuf = a_pucDstImageBuf+(uwTopLine-1)*a_uwDstImageWidth;
    for ( i=0; i<uwTopLine; i++)
    {
        memcpy( pucDstImageBuf, pucSrcImageBuf, a_uwDstImageWidth);
        pucSrcImageBuf+=a_uwDstImageWidth;
        pucDstImageBuf-=a_uwDstImageWidth;
    }
    
    //mirror bottom
    pucSrcImageBuf = a_pucDstImageBuf+(uwTopLine+a_uwSrcImageHeight-2)*a_uwDstImageWidth;
    pucDstImageBuf = a_pucDstImageBuf+(uwTopLine+a_uwSrcImageHeight)*a_uwDstImageWidth;
    for ( i=0; i<uwBottomLine; i++)
    {
        memcpy( pucDstImageBuf, pucSrcImageBuf, a_uwDstImageWidth);
        pucSrcImageBuf-=a_uwDstImageWidth;
        pucDstImageBuf+=a_uwDstImageWidth;
    }
}

void CCMirror16(unsigned short *a_puwDstImageBuf, unsigned short *a_puwSrcImageBuf,
    unsigned short a_uwDstImageWidth, unsigned short a_uwDstImageHeight,
    unsigned short a_uwSrcImageWidth, unsigned short a_uwSrcImageHeight,
    unsigned char a_ucLeft, unsigned char a_ucRight, unsigned char a_ucTop, unsigned char a_ucBottom)
{
    int i, j;

    unsigned short *puwSrcImageBuf;
    unsigned short *puwDstImageBuf;
    unsigned short uwTopLine, uwBottomLine, uwLeftLine, uwRightLine;
    uwTopLine = a_ucTop;
    uwBottomLine = a_ucBottom;
    uwLeftLine = a_ucLeft;
    uwRightLine = a_ucRight;

    puwSrcImageBuf = a_puwSrcImageBuf;
    puwDstImageBuf = a_puwDstImageBuf + uwTopLine*a_uwDstImageWidth + uwLeftLine;
    for (i = 0; i<a_uwSrcImageHeight; i++)
    {
        memcpy(puwDstImageBuf, puwSrcImageBuf, sizeof(unsigned short)*a_uwSrcImageWidth);
        puwSrcImageBuf += a_uwSrcImageWidth;
        puwDstImageBuf += a_uwDstImageWidth;
    }

    //mirror left
    puwSrcImageBuf = a_puwSrcImageBuf + 1;
    puwDstImageBuf = a_puwDstImageBuf + uwTopLine*a_uwDstImageWidth + uwLeftLine - 1;
    for (i = 0; i<a_uwSrcImageHeight; i++)
    {
        for (j = 0; j<uwLeftLine; j++)
        {
            *puwDstImageBuf = *puwSrcImageBuf;
            puwSrcImageBuf++;
            puwDstImageBuf--;
        }
        puwSrcImageBuf += (a_uwSrcImageWidth - uwLeftLine);
        puwDstImageBuf += (a_uwDstImageWidth + uwLeftLine);
    }

    //mirror right
    puwSrcImageBuf = a_puwSrcImageBuf + a_uwSrcImageWidth - 2;
    puwDstImageBuf = a_puwDstImageBuf + uwTopLine*a_uwDstImageWidth + uwLeftLine + a_uwSrcImageWidth;
    for (i = 0; i<a_uwSrcImageHeight; i++)
    {
        for (j = 0; j<uwRightLine; j++)
        {
            *puwDstImageBuf = *puwSrcImageBuf;
            puwSrcImageBuf--;
            puwDstImageBuf++;
        }
        puwSrcImageBuf += (a_uwSrcImageWidth + uwRightLine);
        puwDstImageBuf += (a_uwDstImageWidth - uwRightLine);
    }

    //mirror top
    puwSrcImageBuf = a_puwDstImageBuf + (uwTopLine + 1)*a_uwDstImageWidth;
    puwDstImageBuf = a_puwDstImageBuf + (uwTopLine - 1)*a_uwDstImageWidth;
    for (i = 0; i<uwTopLine; i++)
    {
        memcpy(puwDstImageBuf, puwSrcImageBuf, sizeof(unsigned short)*a_uwDstImageWidth);
        puwSrcImageBuf += a_uwDstImageWidth;
        puwDstImageBuf -= a_uwDstImageWidth;
    }

    //mirror bottom
    puwSrcImageBuf = a_puwDstImageBuf + (uwTopLine + a_uwSrcImageHeight - 2)*a_uwDstImageWidth;
    puwDstImageBuf = a_puwDstImageBuf + (uwTopLine + a_uwSrcImageHeight)*a_uwDstImageWidth;
    for (i = 0; i<uwBottomLine; i++)
    {
        memcpy(puwDstImageBuf, puwSrcImageBuf, sizeof(unsigned short)*a_uwDstImageWidth);
        puwSrcImageBuf -= a_uwDstImageWidth;
        puwDstImageBuf += a_uwDstImageWidth;
    }
}

void CCDuplicate( unsigned char *a_pucDstImageBuf, unsigned char *a_pucSrcImageBuf,
              unsigned short a_uwDstImageWidth, unsigned short a_uwDstImageHeight,
              unsigned short a_uwSrcImageWidth, unsigned short a_uwSrcImageHeight, 
              unsigned char a_ucLeft, unsigned char a_ucRight, unsigned char a_ucTop, unsigned char a_ucBottom)
{
    int i, j;
    
    unsigned char *pucSrcImageBuf;
    unsigned char *pucDstImageBuf;
    unsigned short uwTopLine, uwBottomLine, uwLeftLine, uwRightLine;
    uwTopLine = a_ucTop;
    uwBottomLine = a_ucBottom;
    uwLeftLine = a_ucLeft;
    uwRightLine = a_ucRight;
    
    pucSrcImageBuf = a_pucSrcImageBuf;
    pucDstImageBuf = a_pucDstImageBuf+uwTopLine*a_uwDstImageWidth+uwLeftLine;
    for ( i=0; i<a_uwSrcImageHeight; i++)
    {
        memcpy( pucDstImageBuf, pucSrcImageBuf, a_uwSrcImageWidth);
        pucSrcImageBuf+=a_uwSrcImageWidth;
        pucDstImageBuf+=a_uwDstImageWidth;
    }
    
    //left
    pucSrcImageBuf = a_pucSrcImageBuf;
    pucDstImageBuf = a_pucDstImageBuf+uwTopLine*a_uwDstImageWidth+uwLeftLine-1;
    for ( i=0; i<a_uwSrcImageHeight; i++)
    {
        for ( j=0; j<uwLeftLine; j++)
        {
            *pucDstImageBuf = *pucSrcImageBuf;
            pucDstImageBuf--;
        }
        pucSrcImageBuf+=a_uwSrcImageWidth;
        pucDstImageBuf+=(a_uwDstImageWidth+uwLeftLine);
    }
    
    //right
    pucSrcImageBuf = a_pucSrcImageBuf+a_uwSrcImageWidth-1;
    pucDstImageBuf = a_pucDstImageBuf+uwTopLine*a_uwDstImageWidth+uwLeftLine+a_uwSrcImageWidth;
    for ( i=0; i<a_uwSrcImageHeight; i++)
    {
        for ( j=0; j<uwRightLine; j++)
        {
            *pucDstImageBuf = *pucSrcImageBuf;
            pucDstImageBuf++;
        }
        pucSrcImageBuf+=a_uwSrcImageWidth;
        pucDstImageBuf+=(a_uwDstImageWidth-uwRightLine);
    }
    
    //mirror top
    pucSrcImageBuf = a_pucDstImageBuf+uwTopLine*a_uwDstImageWidth;
    pucDstImageBuf = a_pucDstImageBuf+(uwTopLine-1)*a_uwDstImageWidth;
    for ( i=0; i<uwTopLine; i++)
    {
        memcpy( pucDstImageBuf, pucSrcImageBuf, a_uwDstImageWidth);
        pucDstImageBuf-=a_uwDstImageWidth;
    }
    
    //mirror bottom
    pucSrcImageBuf = a_pucDstImageBuf+(uwTopLine+a_uwSrcImageHeight-1)*a_uwDstImageWidth;
    pucDstImageBuf = a_pucDstImageBuf+(uwTopLine+a_uwSrcImageHeight)*a_uwDstImageWidth;
    for ( i=0; i<uwBottomLine; i++)
    {
        memcpy( pucDstImageBuf, pucSrcImageBuf, a_uwDstImageWidth);
        pucDstImageBuf+=a_uwDstImageWidth;
    }
}

//0:clockwise, 1:counterclockwise, 2:top-down
void CCRotate( unsigned char *a_pucDstImageBuf, unsigned char *a_pucSrcImageBuf, unsigned short a_uwImageWidth, unsigned short a_uwImageHeight,
              unsigned char a_ucDirection)
{
    unsigned int i, j;
    unsigned char *pucSrcImageBuf;
    unsigned char *pucDstImageBuf;
    if ( a_ucDirection==0 )
    {
        pucSrcImageBuf = a_pucSrcImageBuf;
        for ( i=0; i<a_uwImageHeight; i++)
        {
            pucDstImageBuf = a_pucDstImageBuf+a_uwImageHeight-1-i;
            for ( j=0; j<a_uwImageWidth; j++)
            {
                *pucDstImageBuf = (*pucSrcImageBuf);
                pucSrcImageBuf++;
                pucDstImageBuf+=a_uwImageHeight;
            }
        }
    }
    else if ( a_ucDirection==1 )
    {
        pucSrcImageBuf = a_pucSrcImageBuf;
        for ( i=0; i<a_uwImageHeight; i++)
        {
            pucDstImageBuf = a_pucDstImageBuf+(a_uwImageWidth-1)*a_uwImageHeight+i;
            for ( j=0; j<a_uwImageWidth; j++)
            {
                *pucDstImageBuf = (*pucSrcImageBuf);
                pucSrcImageBuf++;
                pucDstImageBuf-=a_uwImageHeight;
            }
        }
    }
    else if ( a_ucDirection==2 )
    {
        pucSrcImageBuf = a_pucSrcImageBuf;
        pucDstImageBuf = a_pucDstImageBuf+a_uwImageWidth*a_uwImageHeight-1;
        for ( i=0; i<a_uwImageHeight; i++)
        {
            for ( j=0; j<a_uwImageWidth; j++)
            {
                *pucDstImageBuf = (*pucSrcImageBuf);
                pucSrcImageBuf++;
                pucDstImageBuf--;
            }
        }
    }
}

//0:clockwise, 1:counterclockwise, 2:top-down
void CCRotate16( unsigned short *a_puwDstImageBuf, unsigned short *a_puwSrcImageBuf, unsigned short a_uwImageWidth, unsigned short a_uwImageHeight,
              unsigned char a_ucDirection)
{
    unsigned int i, j;
    unsigned short *puwSrcImageBuf;
    unsigned short *puwDstImageBuf;
    if ( a_ucDirection==0 )
    {
        puwSrcImageBuf = a_puwSrcImageBuf;
        for ( i=0; i<a_uwImageHeight; i++)
        {
            puwDstImageBuf = a_puwDstImageBuf+a_uwImageHeight-1-i;
            for ( j=0; j<a_uwImageWidth; j++)
            {
                *puwDstImageBuf = (*puwSrcImageBuf);
                puwSrcImageBuf++;
                puwDstImageBuf+=a_uwImageHeight;
            }
        }
    }
    else if ( a_ucDirection==1 )
    {
        puwSrcImageBuf = a_puwSrcImageBuf;
        for ( i=0; i<a_uwImageHeight; i++)
        {
            puwDstImageBuf = a_puwDstImageBuf+(a_uwImageWidth-1)*a_uwImageHeight+i;
            for ( j=0; j<a_uwImageWidth; j++)
            {
                *puwDstImageBuf = (*puwSrcImageBuf);
                puwSrcImageBuf++;
                puwDstImageBuf-=a_uwImageHeight;
            }
        }
    }
    else if ( a_ucDirection==2 )
    {
        puwSrcImageBuf = a_puwSrcImageBuf;
        puwDstImageBuf = a_puwDstImageBuf+a_uwImageWidth*a_uwImageHeight-1;
        for ( i=0; i<a_uwImageHeight; i++)
        {
            for ( j=0; j<a_uwImageWidth; j++)
            {
                *puwDstImageBuf = (*puwSrcImageBuf);
                puwSrcImageBuf++;
                puwDstImageBuf--;
            }
        }
    }
}

//0:horizontal, 1:vertical
void CCFlip( unsigned char *a_pucDstImageBuf, unsigned char *a_pucSrcImageBuf, unsigned short a_uwImageWidth, unsigned short a_uwImageHeight,
              unsigned char a_ucDirection)
{
    unsigned int i, j;

    if ( a_ucDirection==0 )
    {
        unsigned char *pucSrcImageBuf = a_pucSrcImageBuf;
        unsigned char *pucDstImageBuf = a_pucDstImageBuf+a_uwImageWidth-1;
        for ( i=0; i<a_uwImageHeight; i++)
        {
            for ( j=0; j<a_uwImageWidth; j++)
            {
                pucDstImageBuf[0] = pucSrcImageBuf[0];
                
                pucSrcImageBuf++;
                pucDstImageBuf--;
            }
            pucDstImageBuf+=(a_uwImageWidth*2);
        }
    }
    else if ( a_ucDirection==1 )
    {
        unsigned char *pucSrcImageBuf = a_pucSrcImageBuf;
        unsigned char *pucDstImageBuf = a_pucDstImageBuf+a_uwImageWidth*a_uwImageHeight-a_uwImageWidth;
        for ( i=0; i<a_uwImageHeight; i++)
        {
            for ( j=0; j<a_uwImageWidth; j++)
            {
                pucDstImageBuf[0] = pucSrcImageBuf[0];
                
                pucSrcImageBuf++;
                pucDstImageBuf++;
            }
            pucDstImageBuf-=(a_uwImageWidth*2);
        }
    }
}

//0:horizontal, 1:vertical
void CCFlip16( unsigned short *a_puwDstImageBuf, unsigned short *a_puwSrcImageBuf, unsigned short a_uwImageWidth, unsigned short a_uwImageHeight,
            unsigned char a_ucDirection)
{
    unsigned int i, j;
    
    if ( a_ucDirection==0 )
    {
        unsigned short *puwSrcImageBuf = a_puwSrcImageBuf;
        unsigned short *puwDstImageBuf = a_puwDstImageBuf+a_uwImageWidth-1;
        for ( i=0; i<a_uwImageHeight; i++)
        {
            for ( j=0; j<a_uwImageWidth; j++)
            {
                puwDstImageBuf[0] = puwSrcImageBuf[0];
                
                puwSrcImageBuf++;
                puwDstImageBuf--;
            }
            puwDstImageBuf+=(a_uwImageWidth*2);
        }
    }
    else if ( a_ucDirection==1 )
    {
        unsigned short *puwSrcImageBuf = a_puwSrcImageBuf;
        unsigned short *puwDstImageBuf = a_puwDstImageBuf+a_uwImageWidth*a_uwImageHeight-a_uwImageWidth;
        for ( i=0; i<a_uwImageHeight; i++)
        {
            for ( j=0; j<a_uwImageWidth; j++)
            {
                puwDstImageBuf[0] = puwSrcImageBuf[0];
                
                puwSrcImageBuf++;
                puwDstImageBuf++;
            }
            puwDstImageBuf-=(a_uwImageWidth*2);
        }
    }
}

short opt_med9( short *p)
{
    PIX_SORT(p[1], p[2]) ; PIX_SORT(p[4], p[5]) ; PIX_SORT(p[7], p[8]) ;
    PIX_SORT(p[0], p[1]) ; PIX_SORT(p[3], p[4]) ; PIX_SORT(p[6], p[7]) ;
    PIX_SORT(p[1], p[2]) ; PIX_SORT(p[4], p[5]) ; PIX_SORT(p[7], p[8]) ;
    PIX_SORT(p[0], p[3]) ; PIX_SORT(p[5], p[8]) ; PIX_SORT(p[4], p[7]) ;
    PIX_SORT(p[3], p[6]) ; PIX_SORT(p[1], p[4]) ; PIX_SORT(p[2], p[5]) ;
    PIX_SORT(p[4], p[7]) ; PIX_SORT(p[4], p[2]) ; PIX_SORT(p[6], p[4]) ;
    PIX_SORT(p[4], p[2]) ; return(p[4]) ;
}

short opt_med25( short *p)
{
    PIX_SORT(p[0], p[1]) ; PIX_SORT(p[3], p[4]) ; PIX_SORT(p[2], p[4]) ;
    PIX_SORT(p[2], p[3]) ; PIX_SORT(p[6], p[7]) ; PIX_SORT(p[5], p[7]) ;
    PIX_SORT(p[5], p[6]) ; PIX_SORT(p[9], p[10]) ; PIX_SORT(p[8], p[10]) ;
    PIX_SORT(p[8], p[9]) ; PIX_SORT(p[12], p[13]) ; PIX_SORT(p[11], p[13]) ;
    PIX_SORT(p[11], p[12]) ; PIX_SORT(p[15], p[16]) ; PIX_SORT(p[14], p[16]) ;
    PIX_SORT(p[14], p[15]) ; PIX_SORT(p[18], p[19]) ; PIX_SORT(p[17], p[19]) ;
    PIX_SORT(p[17], p[18]) ; PIX_SORT(p[21], p[22]) ; PIX_SORT(p[20], p[22]) ;
    PIX_SORT(p[20], p[21]) ; PIX_SORT(p[23], p[24]) ; PIX_SORT(p[2], p[5]) ;
    PIX_SORT(p[3], p[6]) ; PIX_SORT(p[0], p[6]) ; PIX_SORT(p[0], p[3]) ;
    PIX_SORT(p[4], p[7]) ; PIX_SORT(p[1], p[7]) ; PIX_SORT(p[1], p[4]) ;
    PIX_SORT(p[11], p[14]) ; PIX_SORT(p[8], p[14]) ; PIX_SORT(p[8], p[11]) ;
    PIX_SORT(p[12], p[15]) ; PIX_SORT(p[9], p[15]) ; PIX_SORT(p[9], p[12]) ;
    PIX_SORT(p[13], p[16]) ; PIX_SORT(p[10], p[16]) ; PIX_SORT(p[10], p[13]) ;
    PIX_SORT(p[20], p[23]) ; PIX_SORT(p[17], p[23]) ; PIX_SORT(p[17], p[20]) ;
    PIX_SORT(p[21], p[24]) ; PIX_SORT(p[18], p[24]) ; PIX_SORT(p[18], p[21]) ;
    PIX_SORT(p[19], p[22]) ; PIX_SORT(p[8], p[17]) ; PIX_SORT(p[9], p[18]) ;
    PIX_SORT(p[0], p[18]) ; PIX_SORT(p[0], p[9]) ; PIX_SORT(p[10], p[19]) ;
    PIX_SORT(p[1], p[19]) ; PIX_SORT(p[1], p[10]) ; PIX_SORT(p[11], p[20]) ;
    PIX_SORT(p[2], p[20]) ; PIX_SORT(p[2], p[11]) ; PIX_SORT(p[12], p[21]) ;
    PIX_SORT(p[3], p[21]) ; PIX_SORT(p[3], p[12]) ; PIX_SORT(p[13], p[22]) ;
    PIX_SORT(p[4], p[22]) ; PIX_SORT(p[4], p[13]) ; PIX_SORT(p[14], p[23]) ;
    PIX_SORT(p[5], p[23]) ; PIX_SORT(p[5], p[14]) ; PIX_SORT(p[15], p[24]) ;
    PIX_SORT(p[6], p[24]) ; PIX_SORT(p[6], p[15]) ; PIX_SORT(p[7], p[16]) ;
    PIX_SORT(p[7], p[19]) ; PIX_SORT(p[13], p[21]) ; PIX_SORT(p[15], p[23]) ;
    PIX_SORT(p[7], p[13]) ; PIX_SORT(p[7], p[15]) ; PIX_SORT(p[1], p[9]) ;
    PIX_SORT(p[3], p[11]) ; PIX_SORT(p[5], p[17]) ; PIX_SORT(p[11], p[17]) ;
    PIX_SORT(p[9], p[17]) ; PIX_SORT(p[4], p[10]) ; PIX_SORT(p[6], p[12]) ;
    PIX_SORT(p[7], p[14]) ; PIX_SORT(p[4], p[6]) ; PIX_SORT(p[4], p[7]) ;
    PIX_SORT(p[12], p[14]) ; PIX_SORT(p[10], p[14]) ; PIX_SORT(p[6], p[7]) ;
    PIX_SORT(p[10], p[12]) ; PIX_SORT(p[6], p[10]) ; PIX_SORT(p[6], p[17]) ;
    PIX_SORT(p[12], p[17]) ; PIX_SORT(p[7], p[17]) ; PIX_SORT(p[7], p[10]) ;
    PIX_SORT(p[12], p[18]) ; PIX_SORT(p[7], p[12]) ; PIX_SORT(p[10], p[18]) ;
    PIX_SORT(p[12], p[20]) ; PIX_SORT(p[10], p[20]) ; PIX_SORT(p[10], p[12]) ;
    return (p[12]);
}

void CCCropWOI( unsigned char *a_pucDstImageBuf, unsigned char *a_pucSrcImageBuf, unsigned short a_uwDstImageWidth, unsigned short a_uwDstImageHeight, unsigned short a_uwSrcImageWidth, unsigned short a_uwSrcImageHeight,
             unsigned short a_uwWOIOffsetX, unsigned short a_uwWOIOffsetY)
{
    int i;
    unsigned char *pucSrcImageBuf = a_pucSrcImageBuf+a_uwWOIOffsetY*a_uwSrcImageWidth+a_uwWOIOffsetX;
    unsigned char *pucDstImageBuf = a_pucDstImageBuf;
    for ( i=0; i<a_uwDstImageHeight; i++)
    {
        memcpy( pucDstImageBuf, pucSrcImageBuf, sizeof(unsigned char)*a_uwDstImageWidth);
        pucSrcImageBuf+=a_uwSrcImageWidth;
        pucDstImageBuf+=a_uwDstImageWidth;
    }
}

void CCCropWOI16( unsigned short *a_puwDstImageBuf, unsigned short *a_puwSrcImageBuf, unsigned short a_uwDstImageWidth, unsigned short a_uwDstImageHeight, unsigned short a_uwSrcImageWidth, unsigned short a_uwSrcImageHeight,
               unsigned short a_uwWOIOffsetX, unsigned short a_uwWOIOffsetY)
{
    int i;
    unsigned short *puwSrcImageBuf = a_puwSrcImageBuf+a_uwWOIOffsetY*a_uwSrcImageWidth+a_uwWOIOffsetX;
    unsigned short *puwDstImageBuf = a_puwDstImageBuf;
    for ( i=0; i<a_uwDstImageHeight; i++)
    {
        memcpy( puwDstImageBuf, puwSrcImageBuf, sizeof(unsigned short)*a_uwDstImageWidth);
        puwSrcImageBuf+=a_uwSrcImageWidth;
        puwDstImageBuf+=a_uwDstImageWidth;
    }
}

bool CCIntegralImage( unsigned int *a_pudIntegralBuf, unsigned char *a_pucSrcImageBuf, unsigned short a_uwWidth, unsigned short a_uwHeight)
{
    //integral buffer should be (a_uwWidth+1)*(a_uwHeight+1)
    if ( (a_uwWidth*a_uwHeight*255)>0xFFFFFFFF )
        return false;

    unsigned int udSum, i, j;
    unsigned int *pudDstBuf = new unsigned int[a_uwWidth*a_uwHeight];

    pudDstBuf[0] = a_pucSrcImageBuf[0];
    for ( i=1; i<a_uwWidth; i++)
        pudDstBuf[i] = pudDstBuf[i-1]+a_pucSrcImageBuf[i];

    unsigned char *pucSrcBuf = a_pucSrcImageBuf+a_uwWidth;
    unsigned int *pudTempBuf = pudDstBuf+a_uwWidth;
    for ( i=1; i<a_uwHeight; i++)
    {
        udSum = 0;
        for ( j=0; j<a_uwWidth; j++)
        {
            udSum+=(*pucSrcBuf);
            *pudTempBuf = pudTempBuf[-a_uwWidth]+udSum;

            pucSrcBuf++;
            pudTempBuf++;
        }
    }

    memset( a_pudIntegralBuf, 0, sizeof(unsigned int)*(a_uwWidth+1)*(a_uwHeight+1));
    for ( i=0; i<a_uwHeight; i++)
        memcpy( a_pudIntegralBuf+(a_uwWidth+1)*(i+1)+1, pudDstBuf+i*a_uwWidth, sizeof(unsigned int)*a_uwWidth);

    delete [] pudDstBuf;

    return true;
}

bool CCIntegralImage64( __int64 *a_pddIntegralBuf, unsigned short *a_puwSrcImageBuf, unsigned short a_uwWidth, unsigned short a_uwHeight)
{
    //integral buffer should be (a_uwWidth+1)*(a_uwHeight+1)
    if ((((_I64_MAX / (double)a_uwWidth) / (double)a_uwHeight) / (double)65535)<1.0)
            return false;
    
    unsigned int udSum, i, j;
    __int64 *pddTempBuf = new __int64[a_uwWidth*a_uwHeight];
    
    pddTempBuf[0] = a_puwSrcImageBuf[0];
    for ( i=1; i<a_uwWidth; i++)
        pddTempBuf[i] = pddTempBuf[i-1]+a_puwSrcImageBuf[i];
    
    unsigned short *puwSrcBuf = a_puwSrcImageBuf + a_uwWidth;
    __int64 *ppddTempBuf = pddTempBuf + a_uwWidth;
    for ( i=1; i<a_uwHeight; i++)
    {
        udSum = 0;
        for ( j=0; j<a_uwWidth; j++)
        {
            udSum += puwSrcBuf[0];
            ppddTempBuf[0] = ppddTempBuf[-a_uwWidth]+udSum;

            puwSrcBuf++;
            ppddTempBuf++;
        }
    }
    
    memset(a_pddIntegralBuf, 0, sizeof(__int64)*(a_uwWidth + 1)*(a_uwHeight + 1));
    for ( i=0; i<a_uwHeight; i++)
        memcpy(a_pddIntegralBuf + (a_uwWidth + 1)*(i + 1) + 1, pddTempBuf + i*a_uwWidth, sizeof(__int64)*a_uwWidth);
    
    delete [] pddTempBuf;
    
    return true;
}

__int64 CCGetIntegralValue(__int64 *a_pddIntegralBuf, unsigned short a_uwImageWidth, unsigned short a_uwImageHeigt, unsigned short a_uwStart_x, unsigned short a_uwStart_y, unsigned short a_uwWOIWidth, unsigned short a_uwWOIHeight)
{
    //integral image should be (a_uwImageWidth+1)*(a_uwImageHeigt+1)
    //coordinate at integral image
    unsigned short uwIntegralWidth = a_uwImageWidth + 1;
    unsigned short uwStart_x = a_uwStart_x + 1;
    unsigned short uwStart_y = a_uwStart_y + 1;

    __int64 *pddIntegralBuf = a_pddIntegralBuf + (uwStart_y - 1)*uwIntegralWidth + (uwStart_x - 1);
    unsigned int udOffset = a_uwWOIHeight*uwIntegralWidth;

    //int int ddValue = a_pddIntegralBuf[(uwStart_y + a_uwWOIHeight - 1)*uwIntegralWidth + (uwStart_x + a_uwWOIWidth - 1)]
    //    + a_pddIntegralBuf[(uwStart_y - 1)*uwIntegralWidth + (uwStart_x - 1)]
    //    - a_pddIntegralBuf[(uwStart_y - 1)*uwIntegralWidth + (uwStart_x + a_uwWOIWidth - 1)]
    //    - a_pddIntegralBuf[(uwStart_y + a_uwWOIHeight - 1)*uwIntegralWidth + (uwStart_x - 1)];
    __int64 ddValue = pddIntegralBuf[udOffset + a_uwWOIWidth]
        + pddIntegralBuf[0]
        - pddIntegralBuf[a_uwWOIWidth]
        - pddIntegralBuf[udOffset];

    return ddValue;
}

bool CCIntegralSquareImage(__int64 *a_pddIntegralBuf, unsigned short *a_puwSrcImageBuf, unsigned short a_uwWidth, unsigned short a_uwHeight)
{
    //integral buffer should be (a_uwWidth+1)*(a_uwHeight+1)
    if (((((_I64_MAX / (float)a_uwWidth) / (float)a_uwHeight) / 65535.0) / 65535.0) < 1.0f)
    {
        return false;
    }

    unsigned int udSum, i, j;
    __int64 *pddTempBuf = new __int64[a_uwWidth*a_uwHeight];

    pddTempBuf[0] = a_puwSrcImageBuf[0] * a_puwSrcImageBuf[0];
    for (i = 1; i<a_uwWidth; i++)
        pddTempBuf[i] = pddTempBuf[i - 1] + a_puwSrcImageBuf[i] * a_puwSrcImageBuf[i];

    for (i = 1; i<a_uwHeight; i++)
    {
        udSum = 0;
        for (j = 0; j<a_uwWidth; j++)
        {
            udSum += a_puwSrcImageBuf[i*a_uwWidth + j] * a_puwSrcImageBuf[i*a_uwWidth + j];
            pddTempBuf[i*a_uwWidth + j] = pddTempBuf[(i - 1)*a_uwWidth + j] + udSum;
        }
    }

    memset(a_pddIntegralBuf, 0, sizeof(__int64)*(a_uwWidth + 1)*(a_uwHeight + 1));
    for (i = 0; i<a_uwHeight; i++)
        memcpy(a_pddIntegralBuf + (a_uwWidth + 1)*(i + 1) + 1, pddTempBuf + i*a_uwWidth, sizeof(__int64)*a_uwWidth);

    delete[] pddTempBuf;

    return true;
}

void CCStandardDeviationImage( unsigned char *a_pucSTDBuf, unsigned char *a_pucSrcImageBuf, unsigned short a_uwWidth, unsigned short a_uwHeight, unsigned char a_ucWindow_h, unsigned char a_ucWindow_v)
{
    unsigned char ucOffsetX = a_ucWindow_h/2;
    unsigned char ucOffsetY = a_ucWindow_v/2;
    unsigned short uwMirrorWidth = a_uwWidth+ucOffsetX*2;
    unsigned short uwMirrorHeight = a_uwHeight+ucOffsetY*2;
    unsigned char ucWindowCount = a_ucWindow_h*a_ucWindow_v;

    unsigned char *pucMirrorBuf = new unsigned char[uwMirrorWidth*uwMirrorHeight];
    CCMirror( pucMirrorBuf, a_pucSrcImageBuf, uwMirrorWidth, uwMirrorHeight, a_uwWidth, a_uwHeight, ucOffsetX, ucOffsetX, ucOffsetY, ucOffsetY);

    unsigned int *pudIntegralBuf = new unsigned int[(uwMirrorWidth+1)*(uwMirrorHeight+1)];
    CCIntegralImage( pudIntegralBuf, pucMirrorBuf, uwMirrorWidth, uwMirrorHeight);

    unsigned short *puwMirrorBuf = new unsigned short[uwMirrorWidth*uwMirrorHeight];
    int i;
    for ( i=0; i<uwMirrorWidth*uwMirrorHeight; i++)
        puwMirrorBuf[i] = pucMirrorBuf[i]*pucMirrorBuf[i];

    __int64 *pddIntegralBuf = new __int64[(uwMirrorWidth + 1)*(uwMirrorHeight + 1)];
    CCIntegralImage64( pddIntegralBuf, puwMirrorBuf, uwMirrorWidth, uwMirrorHeight);

    int x, y;
    for ( y=0; y<a_uwHeight; y++)
    {
        for ( x=0; x<a_uwWidth; x++)
        {
            unsigned int udSum = pudIntegralBuf[(y+a_ucWindow_v)*(uwMirrorWidth+1)+(x+a_ucWindow_h)]
                -pudIntegralBuf[(y+a_ucWindow_v-a_ucWindow_v)*(uwMirrorWidth+1)+(x+a_ucWindow_h)]
                -pudIntegralBuf[(y+a_ucWindow_v)*(uwMirrorWidth+1)+(x+a_ucWindow_h-a_ucWindow_h)]
                +pudIntegralBuf[(y+a_ucWindow_v-a_ucWindow_v)*(uwMirrorWidth+1)+(x+a_ucWindow_h-a_ucWindow_h)];

            __int64 ddSum2 = pddIntegralBuf[(y + a_ucWindow_v)*(uwMirrorWidth + 1) + (x + a_ucWindow_h)]
                -pddIntegralBuf[(y+a_ucWindow_v-a_ucWindow_v)*(uwMirrorWidth+1)+(x+a_ucWindow_h)]
                -pddIntegralBuf[(y+a_ucWindow_v)*(uwMirrorWidth+1)+(x+a_ucWindow_h-a_ucWindow_h)]
                +pddIntegralBuf[(y+a_ucWindow_v-a_ucWindow_v)*(uwMirrorWidth+1)+(x+a_ucWindow_h-a_ucWindow_h)];
            unsigned char ucMean = (unsigned char)(udSum/(float)(ucWindowCount)+0.5f);

            float fTemp = (float)ddSum2/ucWindowCount-((float)udSum/ucWindowCount)*((float)udSum/ucWindowCount);
            if ( fTemp>FLT_EPSILON )
                a_pucSTDBuf[y*a_uwWidth+x] = (unsigned char)min( (sqrt(fTemp)+0.5f), 255);
            else 
                a_pucSTDBuf[y*a_uwWidth+x] = 0;
        }
    }
}

void CCScalingUp( unsigned char *a_pucDstImageBuf, unsigned char *a_pucSrcImageBuf, unsigned short a_uwSrcWidth, unsigned short a_uwSrcHeight, unsigned char a_ucWidthRatio, unsigned char a_ucHeightRatio)
{
    unsigned short uwDstWidth = a_uwSrcWidth*a_ucWidthRatio;
    unsigned short uwDstHeight = a_uwSrcHeight*a_ucHeightRatio;
    float fRatio_x = (float)a_uwSrcWidth/uwDstWidth;
    float fRatio_y = (float)a_uwSrcHeight/uwDstHeight;
    
    unsigned char *pucDstImageBuf = a_pucDstImageBuf;
    
    int x, y;
    for ( y=0; y<uwDstHeight; y++)
    {
        unsigned short uwSrc_y;
        float fSrc_y = fRatio_y*(y+0.5f)-0.5f;
        if ( (fSrc_y<0) )
        {
            fSrc_y = 0;
            uwSrc_y = 0;
        }
        else if ( (fSrc_y>=a_uwSrcHeight-1) )
        {
            fSrc_y = (float)a_uwSrcHeight-1;
            uwSrc_y = a_uwSrcHeight-2;
        }
        else
            uwSrc_y = (unsigned short)fSrc_y;
        
        for ( x=0; x<uwDstWidth; x++)
        {
            unsigned short uwSrc_x;
            float fSrc_x = fRatio_x*(x+0.5f)-0.5f;
            if ( fSrc_x<0 )
            {
                fSrc_x = 0;
                uwSrc_x = 0;
            }
            else if ( (fSrc_x>=a_uwSrcWidth-1) )
            {
                fSrc_x = (float)a_uwSrcWidth-1;
                uwSrc_x = a_uwSrcWidth-2;
            }
            else
                uwSrc_x = (unsigned short)fSrc_x;
                
            unsigned char ucP0 = a_pucSrcImageBuf[uwSrc_y*a_uwSrcWidth+uwSrc_x];
            unsigned char ucP1 = a_pucSrcImageBuf[uwSrc_y*a_uwSrcWidth+uwSrc_x+1];
            unsigned char ucP2 = a_pucSrcImageBuf[(uwSrc_y+1)*a_uwSrcWidth+uwSrc_x];
            unsigned char ucP3 = a_pucSrcImageBuf[(uwSrc_y+1)*a_uwSrcWidth+uwSrc_x+1];
            float fP0 = ucP0+(fSrc_x-uwSrc_x)*((short)ucP1-ucP0);
            float fP2 = ucP2+(fSrc_x-uwSrc_x)*((short)ucP3-ucP2);
            *pucDstImageBuf = (unsigned char)(fP0+(fSrc_y-uwSrc_y)*(fP2-fP0)+0.5f);
            
            pucDstImageBuf++;
        }
    }
}

void CCScalingUp(unsigned char *a_pucDstImageBuf, unsigned char *a_pucSrcImageBuf, unsigned short a_uwDstWidth, unsigned short a_uwDstHeight, unsigned short a_uwSrcWidth, unsigned short a_uwSrcHeight)
{
    float fRatio_x = (float)a_uwSrcWidth / a_uwDstWidth;
    float fRatio_y = (float)a_uwSrcHeight / a_uwDstHeight;

    unsigned char *pucDstImageBuf = a_pucDstImageBuf;

    int x, y;
    for (y = 0; y<a_uwDstHeight; y++)
    {
        unsigned short uwSrc_y;
        float fSrc_y = fRatio_y*(y + 0.5f) - 0.5f;
        if ((fSrc_y<0))
        {
            fSrc_y = 0;
            uwSrc_y = 0;
        }
        else if ((fSrc_y >= a_uwSrcHeight - 1))
        {
            fSrc_y = (float)a_uwSrcHeight - 1;
            uwSrc_y = a_uwSrcHeight - 2;
        }
        else
            uwSrc_y = (unsigned short)fSrc_y;

        for (x = 0; x<a_uwDstWidth; x++)
        {
            unsigned short uwSrc_x;
            float fSrc_x = fRatio_x*(x + 0.5f) - 0.5f;
            if (fSrc_x<0)
            {
                fSrc_x = 0;
                uwSrc_x = 0;
            }
            else if ((fSrc_x >= a_uwSrcWidth - 1))
            {
                fSrc_x = (float)a_uwSrcWidth - 1;
                uwSrc_x = a_uwSrcWidth - 2;
            }
            else
                uwSrc_x = (unsigned short)fSrc_x;

            unsigned char ucP0 = a_pucSrcImageBuf[uwSrc_y*a_uwSrcWidth + uwSrc_x];
            unsigned char ucP1 = a_pucSrcImageBuf[uwSrc_y*a_uwSrcWidth + uwSrc_x + 1];
            unsigned char ucP2 = a_pucSrcImageBuf[(uwSrc_y + 1)*a_uwSrcWidth + uwSrc_x];
            unsigned char ucP3 = a_pucSrcImageBuf[(uwSrc_y + 1)*a_uwSrcWidth + uwSrc_x + 1];
            float fP0 = ucP0 + (fSrc_x - uwSrc_x)*((short)ucP1 - ucP0);
            float fP2 = ucP2 + (fSrc_x - uwSrc_x)*((short)ucP3 - ucP2);
            *pucDstImageBuf = (unsigned char)(fP0 + (fSrc_y - uwSrc_y)*(fP2 - fP0) + 0.5f);

            pucDstImageBuf++;
        }
    }
}

void CCScaling(unsigned char *a_pucDstImageBuf, unsigned char *a_pucSrcImageBuf, unsigned short a_uwDstWidth, unsigned short a_uwDstHeight, unsigned short a_uwSrcWidth, unsigned short a_uwSrcHeight)
{
    float fRatio_x = (float)a_uwDstWidth / a_uwSrcWidth;
    float fRatio_y = (float)a_uwDstHeight / a_uwSrcHeight;

    float fRatio_x_inv = (float)a_uwSrcWidth / a_uwDstWidth;
    float fRatio_y_inv = (float)a_uwSrcHeight / a_uwDstHeight;

    unsigned char *pucDstImageBuf = a_pucDstImageBuf;

    unsigned int uduwSrcWidthuwSrcHeight = a_uwSrcWidth*a_uwSrcHeight;
    unsigned int uduwDstWidthuwSrcHeight = a_uwDstWidth*a_uwSrcHeight;
    //horizontal
    unsigned char *pucHorizontalBuf = new unsigned char[a_uwDstWidth*a_uwSrcHeight];

    if (fRatio_x < 1)
    {//scale down
        unsigned char *pucSrcBuf = a_pucSrcImageBuf;
        unsigned char *pucDstBuf = pucHorizontalBuf;
        unsigned short x, y;
        float fHead, fTail;
        float fWeight_head, fWeight_tail;
        unsigned short uwStart, uwEnd, uwBetween;

        for (x = 0; x < a_uwDstWidth - 1; x++)
        {
            fHead = x*fRatio_x_inv;
            fTail = (x + 1)*fRatio_x_inv;
            fWeight_head = 1.0f - (fHead - (unsigned short)fHead);
            fWeight_tail = fTail - (unsigned short)fTail;
            uwStart = (unsigned short)fHead;
            uwEnd = (unsigned short)fTail;
            uwBetween = uwEnd - uwStart;

            for (y = 0; y < a_uwSrcHeight - 1; y++)
            {
                float fSum = pucSrcBuf[uwStart] * fWeight_head + pucSrcBuf[uwEnd] * fWeight_tail;
                for (unsigned short m = uwStart + 1; m < uwStart + uwBetween; m++)
                {
                    fSum += pucSrcBuf[m];
                }
                *pucDstBuf = (unsigned char)(fSum / fRatio_x_inv + 0.5f);

                pucSrcBuf += a_uwSrcWidth;
                pucDstBuf += a_uwDstWidth;
            }

            pucSrcBuf -= (uduwSrcWidthuwSrcHeight - a_uwSrcWidth);
            pucDstBuf -= (uduwDstWidthuwSrcHeight - a_uwDstWidth - 1);
        }

        pucSrcBuf = a_pucSrcImageBuf;
        pucDstBuf = pucHorizontalBuf + a_uwDstWidth - 1;

        fHead = (a_uwDstWidth - 1)*fRatio_x_inv;
        fTail = a_uwDstWidth*fRatio_x_inv;
        fWeight_head = 1.0f - (fHead - (unsigned short)fHead);
        uwStart = (unsigned short)fHead;
        uwEnd = (unsigned short)fTail;
        uwBetween = uwEnd - uwStart;

        for (y = 0; y < a_uwSrcHeight; y++)
        {
            float fSum = pucSrcBuf[uwStart] * fWeight_head;
            for (unsigned short m = uwStart + 1; m < uwStart + uwBetween; m++)
            {
                fSum += pucSrcBuf[m];
            }
            *pucDstBuf = (unsigned char)(fSum / fRatio_x_inv + 0.5f);

            pucSrcBuf += a_uwSrcWidth;
            pucDstBuf += a_uwDstWidth;
        }
    }
    else
    {//scale up
        unsigned char *pucSrcBuf = a_pucSrcImageBuf;
        unsigned char *pucDstBuf = pucHorizontalBuf;
        unsigned short x, y;
        for (x = 0; x < a_uwDstWidth; x++)
        {
            unsigned short uwSrc_x;
            float fSrc_x = fRatio_x_inv*(x + 0.5f) - 0.5f;
            if (fSrc_x<0)
            {
                fSrc_x = 0;
                uwSrc_x = 0;
            }
            else if (fSrc_x >= a_uwSrcWidth - 1)
            {
                fSrc_x = (float)a_uwSrcWidth - 1;
                uwSrc_x = a_uwSrcWidth - 2;
            }
            else
                uwSrc_x = (unsigned short)fSrc_x;

            for (y = 0; y<a_uwSrcHeight; y++)
            {
                unsigned char ucP0 = pucSrcBuf[uwSrc_x];
                unsigned char ucP1 = pucSrcBuf[uwSrc_x + 1];
                float fP0 = ucP0 + (fSrc_x - uwSrc_x)*((short)ucP1 - ucP0);
                *pucDstBuf = (unsigned char)(ucP0 + (fSrc_x - uwSrc_x)*((short)ucP1 - ucP0) + 0.5f);

                pucSrcBuf += a_uwSrcWidth;
                pucDstBuf += a_uwDstWidth;
            }

            pucSrcBuf -= uduwSrcWidthuwSrcHeight;
            pucDstBuf -= (uduwDstWidthuwSrcHeight - 1);
        }
    }
    //char str[256];
    //sprintf(str, "pucHorizontalBuf-%dx%d.raw", a_uwDstWidth, a_uwSrcHeight);
    //CCSaveYFile(str, pucHorizontalBuf, a_uwDstWidth, a_uwSrcHeight);
    
    //vertical
    if (fRatio_y < 1)
    {//scale down
        unsigned char *pucSrcBuf = pucHorizontalBuf;
        unsigned char *pucDstBuf = a_pucDstImageBuf;
        unsigned short x, y;
        float fHead, fTail;
        float fWeight_head, fWeight_tail;
        unsigned short uwStart, uwEnd, uwBetween;

        for (y = 0; y < a_uwDstHeight - 1; y++)
        {
            fHead = y*fRatio_y_inv;
            fTail = (y + 1)*fRatio_y_inv;
            fWeight_head = 1.0f - (fHead - (unsigned short)fHead);
            fWeight_tail = fTail - (unsigned short)fTail;
            uwStart = (unsigned short)fHead;
            uwEnd = (unsigned short)fTail;
            uwBetween = uwEnd - uwStart;
            
            pucSrcBuf = pucHorizontalBuf + uwStart*a_uwDstWidth;

            for (x = 0; x<a_uwDstWidth; x++)
            {
                float fSum = pucSrcBuf[0] * fWeight_head + pucSrcBuf[uwBetween*a_uwDstWidth] * fWeight_tail;
                //float fSum = pucSrcBuf[0] * fWeight_head;
                for (unsigned short m = 1; m < uwBetween; m++)
                {
                    fSum += pucSrcBuf[m*a_uwDstWidth];
                }
                *pucDstBuf = (unsigned char)(fSum / fRatio_y_inv + 0.5f);

                pucSrcBuf++;
                pucDstBuf++;
            }
        }

        pucDstBuf = a_pucDstImageBuf + (a_uwDstHeight - 1)*a_uwDstWidth;

        fHead = (a_uwDstHeight - 1)*fRatio_y_inv;
        fTail = a_uwDstHeight * fRatio_y_inv;
        fWeight_head = 1.0f - (fHead - (unsigned short)fHead);
        uwStart = (unsigned short)fHead;
        uwEnd = (unsigned short)fTail;
        uwBetween = uwEnd - uwStart;
        
        pucSrcBuf = pucHorizontalBuf + uwStart*a_uwDstWidth;
        
        for (x = 0; x < a_uwDstHeight; x++)
        {
            float fSum = pucSrcBuf[0] * fWeight_head;
            for (unsigned short m = 1; m < uwBetween; m++)
            {
                fSum += pucSrcBuf[m*a_uwDstWidth];
            }
            *pucDstBuf = (unsigned char)(fSum / fRatio_y_inv + 0.5f);

            pucSrcBuf++;
            pucDstBuf++;
        }
    }
    else
    {//scale up
        unsigned char *pucSrcBuf = pucHorizontalBuf;
        unsigned char *pucDstBuf = a_pucDstImageBuf;
        unsigned short x, y;
        for (y = 0; y < a_uwDstHeight; y++)
        {
            unsigned short uwSrc_y;
            float fSrc_y = fRatio_y_inv*(y + 0.5f) - 0.5f;
            if (fSrc_y<0)
            {
                fSrc_y = 0;
                uwSrc_y = 0;
            }
            else if (fSrc_y >= a_uwSrcHeight - 1)
            {
                fSrc_y = (float)a_uwSrcHeight - 1;
                uwSrc_y = a_uwSrcHeight - 2;
            }
            else
                uwSrc_y = (unsigned short)fSrc_y;
 
            pucSrcBuf = pucHorizontalBuf + uwSrc_y*a_uwDstWidth;

            for (x = 0; x<a_uwDstWidth; x++)
            {
                unsigned char ucP0 = pucSrcBuf[0];
                unsigned char ucP1 = pucSrcBuf[a_uwDstWidth];
                *pucDstBuf = (unsigned char)(ucP0 + (fSrc_y - uwSrc_y)*((short)ucP1 - ucP0) + 0.5f);

                pucSrcBuf++;
                pucDstBuf++;
            }
        }
    }

    delete[] pucHorizontalBuf;
}

void CCScaling16(unsigned short *a_puwDstImageBuf, unsigned short *a_puwSrcImageBuf, unsigned short a_uwDstWidth, unsigned short a_uwDstHeight, unsigned short a_uwSrcWidth, unsigned short a_uwSrcHeight)
{
    float fRatio_x = (float)a_uwDstWidth / a_uwSrcWidth;
    float fRatio_y = (float)a_uwDstHeight / a_uwSrcHeight;

    float fRatio_x_inv = (float)a_uwSrcWidth / a_uwDstWidth;
    float fRatio_y_inv = (float)a_uwSrcHeight / a_uwDstHeight;

    unsigned short *puwDstImageBuf = a_puwDstImageBuf;

    unsigned int uduwSrcWidthuwSrcHeight = a_uwSrcWidth*a_uwSrcHeight;
    unsigned int uduwDstWidthuwSrcHeight = a_uwDstWidth*a_uwSrcHeight;
    //horizontal
    unsigned short *puwHorizontalBuf = new unsigned short[a_uwDstWidth*a_uwSrcHeight];

    if (fRatio_x < 1)
    {//scale down
        unsigned short *puwSrcBuf = a_puwSrcImageBuf;
        unsigned short *puwDstBuf = puwHorizontalBuf;
        unsigned short x, y;
        for (x = 0; x < a_uwDstWidth; x++)
        {
            float fHead, fTail;
            fHead = x*fRatio_x_inv;
            fTail = (x + 1)*fRatio_x_inv;
            float fWeight_head = 1.0f - (fHead - (unsigned short)fHead);
            float fWeight_tail = fTail - (unsigned short)fTail;
            unsigned short uwStart = (unsigned short)fHead;
            unsigned short uwEnd = (unsigned short)fTail;
            unsigned short uwBetween = uwEnd - uwStart;

            for (y = 0; y < a_uwSrcHeight; y++)
            {
                float fSum = puwSrcBuf[uwStart] * fWeight_head + puwSrcBuf[uwEnd] * fWeight_tail;
                for (unsigned short m = uwStart + 1; m < uwStart + uwBetween; m++)
                {
                    fSum += puwSrcBuf[m];
                }
                *puwDstBuf = (unsigned short)(fSum / fRatio_x_inv + 0.5f);

                puwSrcBuf += a_uwSrcWidth;
                puwDstBuf += a_uwDstWidth;
            }

            puwSrcBuf -= uduwSrcWidthuwSrcHeight;
            puwDstBuf -= (uduwDstWidthuwSrcHeight - 1);
        }
    }
    else
    {//scale up
        unsigned short *puwSrcBuf = a_puwSrcImageBuf;
        unsigned short *puwDstBuf = puwHorizontalBuf;
        unsigned short x, y;
        for (x = 0; x < a_uwDstWidth; x++)
        {
            unsigned short uwSrc_x;
            float fSrc_x = fRatio_x_inv*(x + 0.5f) - 0.5f;
            if (fSrc_x<0)
            {
                fSrc_x = 0;
                uwSrc_x = 0;
            }
            else if (fSrc_x >= a_uwSrcWidth - 1)
            {
                fSrc_x = (float)a_uwSrcWidth - 1;
                uwSrc_x = a_uwSrcWidth - 2;
            }
            else
                uwSrc_x = (unsigned short)fSrc_x;

            for (y = 0; y<a_uwSrcHeight; y++)
            {
                unsigned short uwP0 = puwSrcBuf[uwSrc_x];
                unsigned short uwP1 = puwSrcBuf[uwSrc_x + 1];
                float fP0 = uwP0 + (fSrc_x - uwSrc_x)*((int)uwP1 - uwP0);
                *puwDstBuf = (unsigned short)(uwP0 + (fSrc_x - uwSrc_x)*((int)uwP1 - uwP0) + 0.5f);

                puwSrcBuf += a_uwSrcWidth;
                puwDstBuf += a_uwDstWidth;
            }

            puwSrcBuf -= uduwSrcWidthuwSrcHeight;
            puwDstBuf -= (uduwDstWidthuwSrcHeight - 1);
        }
    }
    //char str[256];
    //sprintf(str, "puwHorizontalBuf-%dx%d.raw", a_uwDstWidth, a_uwSrcHeight);
    //CCSave16YFile(str, puwHorizontalBuf, a_uwDstWidth, a_uwSrcHeight);


    //vertical
    if (fRatio_y < 1)
    {//scale down
        unsigned short *puwSrcBuf = puwHorizontalBuf;
        unsigned short *puwDstBuf = a_puwDstImageBuf;
        unsigned short x, y;
        for (y = 0; y < a_uwDstHeight; y++)
        {
            float fHead, fTail;
            fHead = y*fRatio_y_inv;
            fTail = (y + 1)*fRatio_y_inv;
            float fWeight_head = 1.0f - (fHead - (unsigned short)fHead);
            float fWeight_tail = fTail - (unsigned short)fTail;
            unsigned short uwStart = (unsigned short)fHead;
            unsigned short uwEnd = (unsigned short)fTail;
            unsigned short uwBetween = uwEnd - uwStart;

            puwSrcBuf = puwHorizontalBuf + uwStart*a_uwDstWidth;

            for (x = 0; x<a_uwDstWidth; x++)
            {
                float fSum = puwSrcBuf[0] * fWeight_head + puwSrcBuf[uwBetween*a_uwDstWidth] * fWeight_tail;
                for (unsigned short m = 1; m < uwBetween; m++)
                {
                    fSum += puwSrcBuf[m*a_uwDstWidth];
                }
                *puwDstBuf = (unsigned short)(fSum / fRatio_y_inv + 0.5f);

                puwSrcBuf++;
                puwDstBuf++;
            }
        }
    }
    else
    {//scale up
        unsigned short *puwSrcBuf = puwHorizontalBuf;
        unsigned short *puwDstBuf = a_puwDstImageBuf;
        unsigned short x, y;
        for (y = 0; y < a_uwDstHeight; y++)
        {
            unsigned short uwSrc_y;
            float fSrc_y = fRatio_y_inv*(y + 0.5f) - 0.5f;
            if (fSrc_y<0)
            {
                fSrc_y = 0;
                uwSrc_y = 0;
            }
            else if (fSrc_y >= a_uwSrcHeight - 1)
            {
                fSrc_y = (float)a_uwSrcHeight - 1;
                uwSrc_y = a_uwSrcHeight - 2;
            }
            else
                uwSrc_y = (unsigned short)fSrc_y;

            puwSrcBuf = puwHorizontalBuf + uwSrc_y*a_uwDstWidth;

            for (x = 0; x<a_uwDstWidth; x++)
            {
                unsigned short uwP0 = puwSrcBuf[0];
                unsigned short uwP1 = puwSrcBuf[a_uwDstWidth];
                *puwDstBuf = (unsigned short)(uwP0 + (fSrc_y - uwSrc_y)*((int)uwP1 - uwP0) + 0.5f);

                puwSrcBuf++;
                puwDstBuf++;
            }
        }
    }

    delete[] puwHorizontalBuf;
}

void CCScalingUpDuplicate( unsigned char *a_pucDstImageBuf, unsigned char *a_pucSrcImageBuf, unsigned short a_uwSrcWidth, unsigned short a_uwSrcHeight, unsigned char a_ucWidthRatio, unsigned char a_ucHeightRatio)
{
    unsigned short uwDstWidth = a_uwSrcWidth*a_ucWidthRatio;
    unsigned short uwDstHeight = a_uwSrcHeight*a_ucHeightRatio;
    memset( a_pucDstImageBuf, 0, uwDstWidth*uwDstHeight);
    
    int i, j, m;
    unsigned char *pucSrcImageBuf = a_pucSrcImageBuf;
    unsigned char *pucDstImageBuf = a_pucDstImageBuf;
    for ( i=0; i<uwDstHeight; i+=a_ucHeightRatio)
    {
        for ( j=0; j<uwDstWidth; j+=a_ucWidthRatio)
        {
            for ( m=0; m<a_ucHeightRatio; m++)
            {
                memset( pucDstImageBuf, *pucSrcImageBuf, a_ucWidthRatio);
                pucDstImageBuf+=uwDstWidth;
            }
            pucDstImageBuf-=(uwDstWidth*a_ucHeightRatio);
            
            pucSrcImageBuf++;
            pucDstImageBuf+=a_ucWidthRatio;
        }
        pucDstImageBuf+=(uwDstWidth*(a_ucHeightRatio-1));
    }
}

void CCScalingUp3D( unsigned char *a_pucDstBuf, unsigned char *a_pucSrcBuf, unsigned short a_uwSrcWidth, unsigned short a_uwSrcHeight, unsigned short a_uwSrcDepth,
                   unsigned char a_ucWidthRatio, unsigned char a_ucHeightRatio, unsigned char a_ucDepthRatio)
{
    unsigned short uwDstWidth = a_uwSrcWidth*a_ucWidthRatio;
    unsigned short uwDstHeight = a_uwSrcHeight*a_ucHeightRatio;
    unsigned short uwDstDepth = a_uwSrcDepth*a_ucDepthRatio;
    float fRatio_x = (float)a_uwSrcWidth/uwDstWidth;
    float fRatio_y = (float)a_uwSrcHeight/uwDstHeight;
    float fRatio_z = (float)a_uwSrcDepth/uwDstDepth;
    unsigned int udOffset_y = a_uwSrcWidth*a_uwSrcDepth;
    unsigned char *pucDstBuf = a_pucDstBuf;
    
    int x, y, z;
    for ( z=0; z<uwDstDepth; z++)
    {
        pucDstBuf = a_pucDstBuf+z;

        unsigned short uwSrc_z;
        float fSrc_z = fRatio_z*(z+0.5f)-0.5f;
        if ( fSrc_z<0 )
        {
            fSrc_z = 0;
            uwSrc_z = (unsigned short)fSrc_z;
        }
        if ( fSrc_z>=a_uwSrcDepth-1 )
        {
            fSrc_z = (float)a_uwSrcDepth-1;
            uwSrc_z = a_uwSrcDepth-2;
        }
        else
            uwSrc_z = (unsigned short)fSrc_z;

        float fWeight_z = fSrc_z-uwSrc_z;

        for ( y=0; y<uwDstHeight; y++)
        {
            unsigned short uwSrc_y;
            float fSrc_y = fRatio_y*(y+0.5f)-0.5f;
            if ( fSrc_y<0 )
            {
                fSrc_y = 0;
                uwSrc_y = (unsigned short)fSrc_y;
            }
            else if ( fSrc_y>=a_uwSrcHeight-1 )
            {
                fSrc_y = (float)a_uwSrcHeight-1;
                uwSrc_y = a_uwSrcHeight-2;
            }
            else
                uwSrc_y = (unsigned short)fSrc_y;
            
            float fWeight_y = fSrc_y-uwSrc_y;

            for ( x=0; x<uwDstWidth; x++)
            {
                unsigned short uwSrc_x;
                float fSrc_x = fRatio_x*(x+0.5f)-0.5f;
                if ( fSrc_x<0 )
                {
                    fSrc_x = 0;
                    uwSrc_x = (unsigned short)fSrc_x;
                }
                else if ( fSrc_x>=a_uwSrcWidth-1 )
                {
                    fSrc_x = (float)a_uwSrcWidth-1;
                    uwSrc_x = a_uwSrcWidth-2;
                }
                else
                    uwSrc_x = (unsigned short)fSrc_x;

                float fWeight_x = fSrc_x-uwSrc_x;

                unsigned char *pucSrcBuf = a_pucSrcBuf+uwSrc_y*udOffset_y+uwSrc_x*a_uwSrcDepth+uwSrc_z;
                unsigned char ucP0 = *pucSrcBuf;
                unsigned char ucP1 = *(pucSrcBuf+a_uwSrcDepth);
                unsigned char ucP2 = *(pucSrcBuf+udOffset_y);
                unsigned char ucP3 = *(pucSrcBuf+udOffset_y+a_uwSrcDepth);
                unsigned char ucP4 = *(pucSrcBuf+1);
                unsigned char ucP5 = *(pucSrcBuf+a_uwSrcDepth+1);
                unsigned char ucP6 = *(pucSrcBuf+udOffset_y+1);
                unsigned char ucP7 = *(pucSrcBuf+udOffset_y+a_uwSrcDepth+1);
                float fP0 = ucP0+fWeight_x*((short)ucP1-ucP0);
                float fP2 = ucP2+fWeight_x*((short)ucP3-ucP2);
                float fP02 = fP0+fWeight_y*(fP2-fP0);
                float fP4 = ucP4+fWeight_x*((short)ucP5-ucP4);
                float fP6 = ucP6+fWeight_x*((short)ucP7-ucP6);
                float fP46 = fP4+fWeight_y*(fP6-fP4);
                *pucDstBuf = (unsigned char)(fP02+fWeight_z*(fP46-fP02)+0.5f);
            
                pucDstBuf+=uwDstDepth;
            }//for ( x=0; x<uwDstWidth; x++)
        }//for ( y=0; y<uwDstHeight; y++)
    }
}

//different z scaling up method
void CCScalingUp3D_z( unsigned char *a_pucDstBuf, unsigned char *a_pucSrcBuf, unsigned short a_uwSrcWidth, unsigned short a_uwSrcHeight, unsigned short a_uwSrcDepth,
                     unsigned char a_ucWidthRatio, unsigned char a_ucHeightRatio, unsigned char a_ucDepthRatio)
{
    unsigned short uwDstWidth = a_uwSrcWidth*a_ucWidthRatio;
    unsigned short uwDstHeight = a_uwSrcHeight*a_ucHeightRatio;
    unsigned short uwDstDepth = a_uwSrcDepth*a_ucDepthRatio;
    float fRatio_x = (float)a_uwSrcWidth/uwDstWidth;
    float fRatio_y = (float)a_uwSrcHeight/uwDstHeight;
    float fRatio_z = (float)a_uwSrcDepth/uwDstDepth;
    unsigned int udOffset_y = a_uwSrcWidth*a_uwSrcDepth;
    unsigned char *pucDstBuf = a_pucDstBuf;
    
    int x, y, z;
    for ( z=0; z<uwDstDepth; z++)
    {
        pucDstBuf = a_pucDstBuf+z;
        
        unsigned short uwSrc_z;
        float fSrc_z = fRatio_z*z;
        if ( fSrc_z<0 )
        {
            fSrc_z = 0;
            uwSrc_z = (unsigned short)fSrc_z;
        }
        if ( fSrc_z>=a_uwSrcDepth-1 )
        {
            fSrc_z = (float)a_uwSrcDepth-1;
            uwSrc_z = a_uwSrcDepth-2;
        }
        else
            uwSrc_z = (unsigned short)fSrc_z;
        
        float fWeight_z = fSrc_z-uwSrc_z;

        for ( y=0; y<uwDstHeight; y++)
        {
            unsigned short uwSrc_y;
            float fSrc_y = fRatio_y*(y+0.5f)-0.5f;
            if ( fSrc_y<0 )
            {
                fSrc_y = 0;
                uwSrc_y = (unsigned short)fSrc_y;
            }
            else if ( fSrc_y>=a_uwSrcHeight-1 )
            {
                fSrc_y = (float)a_uwSrcHeight-1;
                uwSrc_y = a_uwSrcHeight-2;
            }
            else
                uwSrc_y = (unsigned short)fSrc_y;
            
            float fWeight_y = fSrc_y-uwSrc_y;

            unsigned char *pucSrcBuf_yz = a_pucSrcBuf+uwSrc_y*udOffset_y+uwSrc_z;
            for ( x=0; x<uwDstWidth; x++)
            {
                unsigned short uwSrc_x;
                float fSrc_x = fRatio_x*(x+0.5f)-0.5f;
                if ( fSrc_x<0 )
                {
                    fSrc_x = 0;
                    uwSrc_x = (unsigned short)fSrc_x;
                }
                else if ( fSrc_x>=a_uwSrcWidth-1 )
                {
                    fSrc_x = (float)a_uwSrcWidth-1;
                    uwSrc_x = a_uwSrcWidth-2;
                }
                else
                    uwSrc_x = (unsigned short)fSrc_x;
                
                float fWeight_x = fSrc_x-uwSrc_x;

                unsigned char *pucSrcBuf = pucSrcBuf_yz+uwSrc_x*a_uwSrcDepth;
                unsigned char ucP0 = *pucSrcBuf;
                unsigned char ucP1 = *(pucSrcBuf+a_uwSrcDepth);
                unsigned char ucP2 = *(pucSrcBuf+udOffset_y);
                unsigned char ucP3 = *(pucSrcBuf+udOffset_y+a_uwSrcDepth);
                unsigned char ucP4 = *(pucSrcBuf+1);
                unsigned char ucP5 = *(pucSrcBuf+a_uwSrcDepth+1);
                unsigned char ucP6 = *(pucSrcBuf+udOffset_y+1);
                unsigned char ucP7 = *(pucSrcBuf+udOffset_y+a_uwSrcDepth+1);
                float fP0 = ucP0+fWeight_x*((short)ucP1-ucP0);
                float fP2 = ucP2+fWeight_x*((short)ucP3-ucP2);
                float fP02 = fP0+fWeight_y*(fP2-fP0);
                float fP4 = ucP4+fWeight_x*((short)ucP5-ucP4);
                float fP6 = ucP6+fWeight_x*((short)ucP7-ucP6);
                float fP46 = fP4+fWeight_y*(fP6-fP4);
                *pucDstBuf = (unsigned char)(fP02+fWeight_z*(fP46-fP02)+0.5f);
                
                pucDstBuf+=uwDstDepth;
            }//for ( x=0; x<uwDstWidth; x++)
        }//for ( y=0; y<uwDstHeight; y++)
    }
}

//quadratic z scaling up method
void CCScalingUp3D_quadratic_z( unsigned char *a_pucDstBuf, unsigned char *a_pucSrcBuf, unsigned short a_uwSrcWidth, unsigned short a_uwSrcHeight, unsigned short a_uwSrcDepth,
                               unsigned char a_ucWidthRatio, unsigned char a_ucHeightRatio, unsigned char a_ucDepthRatio)
{
    unsigned short uwDstWidth = a_uwSrcWidth*a_ucWidthRatio;
    unsigned short uwDstHeight = a_uwSrcHeight*a_ucHeightRatio;
    unsigned short uwDstDepth = a_uwSrcDepth*a_ucDepthRatio;
    float fRatio_x = (float)a_uwSrcWidth/uwDstWidth;
    float fRatio_y = (float)a_uwSrcHeight/uwDstHeight;
    float fRatio_z = (float)a_uwSrcDepth/uwDstDepth;
    unsigned char *pucDstBuf = a_pucDstBuf;
    
    int x, y, z;
    unsigned int udOffset_y = a_uwSrcWidth*a_uwSrcDepth;
    unsigned char *pucXYBuf = new unsigned char[uwDstWidth*uwDstHeight*a_uwSrcDepth];
    for ( z=0; z<a_uwSrcDepth; z++)
    {
        pucDstBuf = pucXYBuf+z;
        
        for ( y=0; y<uwDstHeight; y++)
        {
            unsigned short uwSrc_y;
            float fSrc_y = fRatio_y*(y+0.5f)-0.5f;
            if ( fSrc_y<0 )
            {
                fSrc_y = 0;
                uwSrc_y = (unsigned short)fSrc_y;
            }
            else if ( fSrc_y>=a_uwSrcHeight-1 )
            {
                fSrc_y = (float)a_uwSrcHeight-1;
                uwSrc_y = a_uwSrcHeight-2;
            }
            else
                uwSrc_y = (unsigned short)fSrc_y;
            
            float fWeight_y = fSrc_y-uwSrc_y;
            
            unsigned char *pucSrcBuf_yz = a_pucSrcBuf+uwSrc_y*udOffset_y+z;
            
            for ( x=0; x<uwDstWidth; x++)
            {
                unsigned short uwSrc_x;
                float fSrc_x = fRatio_x*(x+0.5f)-0.5f;
                if ( fSrc_x<0 )
                {
                    fSrc_x = 0;
                    uwSrc_x = (unsigned short)fSrc_x;
                }
                else if ( fSrc_x>=a_uwSrcWidth-1 )
                {
                    fSrc_x = (float)a_uwSrcWidth-1;
                    uwSrc_x = a_uwSrcWidth-2;
                }
                else
                    uwSrc_x = (unsigned short)fSrc_x;
                
                float fWeight_x = fSrc_x-uwSrc_x;
                
                unsigned char *pucSrcBuf = pucSrcBuf_yz+uwSrc_x*a_uwSrcDepth;
                unsigned char ucP0 = *pucSrcBuf;
                unsigned char ucP1 = *(pucSrcBuf+a_uwSrcDepth);
                unsigned char ucP2 = *(pucSrcBuf+udOffset_y);
                unsigned char ucP3 = *(pucSrcBuf+udOffset_y+a_uwSrcDepth);
                float fP0 = ucP0+fWeight_x*((short)ucP1-ucP0);
                float fP2 = ucP2+fWeight_x*((short)ucP3-ucP2);
                float fP02 = fP0+fWeight_y*(fP2-fP0);
                *pucDstBuf = (unsigned char)(fP02+0.5f);
                
                pucDstBuf+=a_uwSrcDepth;
            }//for ( x=0; x<uwDstWidth; x++)
        }//for ( y=0; y<uwDstHeight; y++)
    }

//     unsigned char *pucImageBuf = new unsigned char[uwDstWidth*uwDstHeight];
//     unsigned char *pucSrcBuf = pucXYBuf;
//     for ( int disp=0; disp<2; disp++)
//     {
//         pucSrcBuf = pucXYBuf+disp;
//         pucDstBuf = pucImageBuf;
//         for ( int i=0; i<uwDstHeight; i++)
//         {
//             for ( int j=0; j<uwDstWidth; j++)
//             {
//                 pucDstBuf[i*uwDstWidth+j] = min( pucSrcBuf[0], 255);
//                 pucSrcBuf+=a_uwSrcDepth;
//             }
//         }
//         
//         char str[256];
//         sprintf( str, "costXY%d.bmp", disp);
//         CCSaveBMP8File( str, pucImageBuf, uwDstWidth, uwDstHeight);
//     }

    //z interpolation 
    pucDstBuf = a_pucDstBuf;
    udOffset_y = uwDstWidth*a_uwSrcDepth;
    for ( y=0; y<uwDstHeight; y++)
    {
        for ( x=0; x<uwDstWidth; x++)
        {
            unsigned char *pucSrcBuf_xy = pucXYBuf+y*udOffset_y+x*a_uwSrcDepth;
            
            for ( z=0; z<uwDstDepth; z++)
            {
                unsigned short uwSrc_z;
                float fSrc_z = fRatio_z*z;
                float fa, fb, fc;
                if ( fSrc_z<0.5f )
                {
                    uwSrc_z = 1;
                }
                else if ( fSrc_z>=(a_uwSrcDepth-1-0.5f) )
                {
                    uwSrc_z = a_uwSrcDepth-2;
                }
                else
                {
                    uwSrc_z = (unsigned short)(fSrc_z+0.5f);
                }
                fSrc_z-=uwSrc_z;
                
                fa = 0.5f*(pucSrcBuf_xy[uwSrc_z-1]-2*pucSrcBuf_xy[uwSrc_z]+pucSrcBuf_xy[uwSrc_z+1]);
                fb = 0.5f*(pucSrcBuf_xy[uwSrc_z+1]-pucSrcBuf_xy[uwSrc_z-1]);
                fc = pucSrcBuf_xy[uwSrc_z];
                
                float fP0 = fa*fSrc_z*fSrc_z+fb*fSrc_z+fc;
                if ( fP0<0 )
                    *pucDstBuf = 0;
                else if ( fP0>255 )
                    *pucDstBuf = 255;
                else
                    *pucDstBuf = (unsigned char)(fP0+0.5f);
                
                pucDstBuf++;
            }
        }
    }
    
    delete [] pucXYBuf;
}

//individual scaling up method
void CCScalingUp3D_individual_z( unsigned char *a_pucDstBuf, unsigned char *a_pucSrcBuf, unsigned short a_uwSrcWidth, unsigned short a_uwSrcHeight, unsigned short a_uwSrcDepth,
                                unsigned char a_ucWidthRatio, unsigned char a_ucHeightRatio, unsigned char a_ucDepthRatio)
{
    unsigned short uwDstWidth = a_uwSrcWidth*a_ucWidthRatio;
    unsigned short uwDstHeight = a_uwSrcHeight*a_ucHeightRatio;
    unsigned short uwDstDepth = a_uwSrcDepth*a_ucDepthRatio;
    float fRatio_x = (float)a_uwSrcWidth/uwDstWidth;
    float fRatio_y = (float)a_uwSrcHeight/uwDstHeight;
    float fRatio_z = (float)a_uwSrcDepth/uwDstDepth;
    unsigned char *pucDstBuf = a_pucDstBuf;
    
    int x, y, z;
    //x interpolation 
    unsigned int udOffset_y = a_uwSrcWidth*a_uwSrcDepth;
    unsigned char *pucXBuf = new unsigned char[uwDstWidth*a_uwSrcHeight*a_uwSrcDepth];
    for ( z=0; z<a_uwSrcDepth; z++)
    {
        pucDstBuf = pucXBuf+z;

        for ( y=0; y<a_uwSrcHeight; y++)
        {
            unsigned char *pucSrcBuf_yz = a_pucSrcBuf+y*udOffset_y+z;

            for ( x=0; x<uwDstWidth; x++)
            {
                unsigned short uwSrc_x;
                float fSrc_x = fRatio_x*(x+0.5f)-0.5f;
                if ( fSrc_x<0 )
                {
                    fSrc_x = 0;
                    uwSrc_x = (unsigned short)fSrc_x;
                }
                else if ( fSrc_x>=a_uwSrcWidth-1 )
                {
                    fSrc_x = (float)a_uwSrcWidth-1;
                    uwSrc_x = a_uwSrcWidth-2;
                }
                else
                    uwSrc_x = (unsigned short)fSrc_x;
                
                float fWeight_x = fSrc_x-uwSrc_x;

                unsigned char *pucSrcBuf = pucSrcBuf_yz+uwSrc_x*a_uwSrcDepth;
                unsigned char ucP0 = *pucSrcBuf;
                unsigned char ucP1 = *(pucSrcBuf+a_uwSrcDepth);

                float fP0 = ucP0+fWeight_x*((short)ucP1-ucP0);
                *pucDstBuf = (unsigned char)(fP0+0.5f);

                pucDstBuf+=a_uwSrcDepth;
            }
        }
    }

//     unsigned char *pucImageBuf = new unsigned char[uwDstWidth*a_uwSrcHeight];
//     unsigned char *pucSrcBuf = pucXBuf;
//     for ( int disp=0; disp<2; disp++)
//     {
//         pucSrcBuf = pucXBuf+disp;
//         pucDstBuf = pucImageBuf;
//         for ( int i=0; i<a_uwSrcHeight; i++)
//         {
//             for ( int j=0; j<uwDstWidth; j++)
//             {
//                 pucDstBuf[i*uwDstWidth+j] = min( pucSrcBuf[0], 255);
//                 pucSrcBuf+=a_uwSrcDepth;
//             }
//         }
//         
//         char str[256];
//         sprintf( str, "costX%d.bmp", disp);
//         CCSaveBMP8File( str, pucImageBuf, uwDstWidth, a_uwSrcHeight);
//     }

    //y interpolation 
    udOffset_y = uwDstWidth*a_uwSrcDepth;
    unsigned char *pucYBuf = new unsigned char[uwDstWidth*uwDstHeight*a_uwSrcDepth];
    pucDstBuf = pucYBuf;
    for ( z=0; z<a_uwSrcDepth; z++)
    {
        for ( x=0; x<uwDstWidth; x++)
        {
            unsigned char *pucSrcBuf_xz = pucXBuf+z+x*a_uwSrcDepth;
            pucDstBuf = pucYBuf+z+x*a_uwSrcDepth;

            for ( y=0; y<uwDstHeight; y++)
            {
                unsigned short uwSrc_y;
                float fSrc_y = fRatio_y*(y+0.5f)-0.5f;
                if ( fSrc_y<0 )
                {
                    fSrc_y = 0;
                    uwSrc_y = (unsigned short)fSrc_y;
                }
                else if ( fSrc_y>=a_uwSrcHeight-1 )
                {
                    fSrc_y = (float)a_uwSrcHeight-1;
                    uwSrc_y = a_uwSrcHeight-2;
                }
                else
                    uwSrc_y = (unsigned short)fSrc_y;
                
                float fWeight_y = fSrc_y-uwSrc_y;
                
                unsigned char *pucSrcBuf = pucSrcBuf_xz+uwSrc_y*udOffset_y;
                unsigned char ucP0 = *pucSrcBuf;
                unsigned char ucP1 = *(pucSrcBuf+udOffset_y);
                
                float fP0 = ucP0+fWeight_y*((short)ucP1-ucP0);
                *pucDstBuf = (unsigned char)(fP0+0.5f);
                
                pucDstBuf+=udOffset_y;
            }
        }
    }

//     unsigned char *pucImageBuf = new unsigned char[uwDstWidth*uwDstHeight];
//     unsigned char *pucSrcBuf = pucYBuf;
//     for ( int disp=0; disp<2; disp++)
//     {
//         pucSrcBuf = pucYBuf+disp;
//         pucDstBuf = pucImageBuf;
//         for ( int i=0; i<uwDstHeight; i++)
//         {
//             for ( int j=0; j<uwDstWidth; j++)
//             {
//                 pucDstBuf[i*uwDstWidth+j] = min( pucSrcBuf[0], 255);
//                 pucSrcBuf+=a_uwSrcDepth;
//             }
//         }
//         
//         char str[256];
//         sprintf( str, "costY%d.bmp", disp);
//         CCSaveBMP8File( str, pucImageBuf, uwDstWidth, uwDstHeight);
//     }

    delete [] pucXBuf;

    //z interpolation 
    pucDstBuf = a_pucDstBuf;
    for ( y=0; y<uwDstHeight; y++)
    {
        for ( x=0; x<uwDstWidth; x++)
        {
            unsigned char *pucSrcBuf_xy = pucYBuf+y*udOffset_y+x*a_uwSrcDepth;
            
            for ( z=0; z<uwDstDepth; z++)
            {
                unsigned short uwSrc_z;
                float fSrc_z = fRatio_z*z;
                if ( fSrc_z<0 )
                {
                    fSrc_z = 0;
                    uwSrc_z = (unsigned short)fSrc_z;
                }
                if ( fSrc_z>=a_uwSrcDepth-1 )
                {
                    fSrc_z = (float)a_uwSrcDepth-1;
                    uwSrc_z = a_uwSrcDepth-2;
                }
                else
                    uwSrc_z = (unsigned short)fSrc_z;
                
                float fWeight_z = fSrc_z-uwSrc_z;
                
                unsigned char *pucSrcBuf = pucSrcBuf_xy+uwSrc_z;
                unsigned char ucP0 = *pucSrcBuf;
                unsigned char ucP1 = *(pucSrcBuf+1);
                
                float fP0 = ucP0+fWeight_z*((short)ucP1-ucP0);
                *pucDstBuf = (unsigned char)(fP0+0.5f);
                
                pucDstBuf++;
            }
        }
    }

    delete [] pucYBuf;
}

void CCDilation( unsigned char *a_pucDstImageBuf, unsigned char *a_pucSrcImageBuf, unsigned short a_uwWidth, unsigned short a_uwHeight)
{
    memset( a_pucDstImageBuf, 255, a_uwWidth*a_uwHeight);
    unsigned short uwMirrorWidth = a_uwWidth+2;
    unsigned short uwMirrorHeight = a_uwHeight+2;
    unsigned char *pucMirrorBuf = new unsigned char[uwMirrorWidth*uwMirrorHeight];
    CCMirror( pucMirrorBuf, a_pucSrcImageBuf, uwMirrorWidth, uwMirrorHeight, a_uwWidth, a_uwHeight, 1, 1, 1, 1);

    int x, y;
    unsigned char *pucSrc = pucMirrorBuf+uwMirrorWidth+1;
    unsigned char *pucDst = a_pucDstImageBuf;
    for ( y=0; y<a_uwHeight; y++)
    {
        for ( x=0; x<a_uwWidth; x++)
        {
            if ( pucSrc[-uwMirrorWidth-1]==0 )
            {
                *pucDst = 0;
                pucSrc++;
                pucDst++;
                continue;
            }
            if ( pucSrc[-uwMirrorWidth]==0 )
            {
                *pucDst = 0;
                pucSrc++;
                pucDst++;
                continue;
            }
            if ( pucSrc[-uwMirrorWidth+1]==0 )
            {
                *pucDst = 0;
                pucSrc++;
                pucDst++;
                continue;
            }
            if ( pucSrc[-1]==0 )
            {
                *pucDst = 0;
                pucSrc++;
                pucDst++;
                continue;
            }
            if ( pucSrc[0]==0 )
            {
                *pucDst = 0;
                pucSrc++;
                pucDst++;
                continue;
            }
            if ( pucSrc[1]==0 )
            {
                *pucDst = 0;
                pucSrc++;
                pucDst++;
                continue;
            }
            if ( pucSrc[uwMirrorWidth-1]==0 )
            {
                *pucDst = 0;
                pucSrc++;
                pucDst++;
                continue;
            }
            if ( pucSrc[uwMirrorWidth]==0 )
            {
                *pucDst = 0;
                pucSrc++;
                pucDst++;
                continue;
            }
            if ( pucSrc[uwMirrorWidth+1]==0 )
            {
                *pucDst = 0;
                pucSrc++;
                pucDst++;
                continue;
            }
            pucSrc++;
            pucDst++;
        }
        pucSrc+=2;
    }

    delete [] pucMirrorBuf;
}

void CCErosion( unsigned char *a_pucDstImageBuf, unsigned char *a_pucSrcImageBuf, unsigned short a_uwWidth, unsigned short a_uwHeight)
{
    memset( a_pucDstImageBuf, 0, a_uwWidth*a_uwHeight);
    
    unsigned short uwMirrorWidth = a_uwWidth+2;
    unsigned short uwMirrorHeight = a_uwHeight+2;
    unsigned char *pucMirrorBuf = new unsigned char[uwMirrorWidth*uwMirrorHeight];
    CCMirror( pucMirrorBuf, a_pucSrcImageBuf, uwMirrorWidth, uwMirrorHeight, a_uwWidth, a_uwHeight, 1, 1, 1, 1);

    int x, y;
    unsigned char *pucSrc = pucMirrorBuf+uwMirrorWidth+1;
    unsigned char *pucDst = a_pucDstImageBuf;
    for ( y=0; y<a_uwHeight; y++)
    {
        for ( x=0; x<a_uwWidth; x++)
        {
            if ( pucDst[0]==0 )
            {
                if (  (pucSrc[-uwMirrorWidth]!=0) || (pucSrc[-1]!=0) || (pucSrc[0]!=0) || (pucSrc[1]!=0) 
                    || (pucSrc[uwMirrorWidth]!=0) )
                {
                    *pucDst = 255;
                    pucSrc++;
                    pucDst++;
                    continue;
                }
            }
           
            pucSrc++;
            pucDst++;
        }
        pucSrc+=2;
    }

    delete [] pucMirrorBuf;
}

float CCStandardDeviation( unsigned char *a_pucBuf, unsigned int a_udNum)
{
    __int64 ddSum = 0;
    __int64 ddSum2 = 0;
    for (unsigned int i = 0; i < a_udNum; i++)
    {
        ddSum += a_pucBuf[i];
        ddSum2 += a_pucBuf[i] * a_pucBuf[i];
    }
    
    float fTemp = (float)ddSum2 / a_udNum - ((float)ddSum / a_udNum)*((float)ddSum / a_udNum);
    if (fTemp>FLT_EPSILON)
        fTemp = sqrt(fTemp);
    else
        fTemp = 0;

    return fTemp;
}

void CCMAE( unsigned char *a_pucMAEBuf, unsigned char *a_pucSrcBuf, unsigned short a_uwWidth, unsigned short a_uwHeight, unsigned char a_ucMaskSize)
{
    unsigned char *pucMeanBuf = new unsigned char[a_uwWidth*a_uwHeight];
    CCMean( pucMeanBuf, a_pucSrcBuf, a_uwWidth, a_uwHeight, a_ucMaskSize);

    unsigned char ucMaskSize_half = a_ucMaskSize/2;
    unsigned short uwMirrorWidth = a_uwWidth+ucMaskSize_half*2;
    unsigned short uwMirrorHeight = a_uwHeight+ucMaskSize_half*2;
    unsigned char *pucMirrorBuf = new unsigned char[uwMirrorWidth*uwMirrorHeight];
    CCMirror( pucMirrorBuf, a_pucSrcBuf, uwMirrorWidth, uwMirrorHeight, a_uwWidth, a_uwHeight, ucMaskSize_half, ucMaskSize_half, ucMaskSize_half, ucMaskSize_half);

    unsigned short uwCount = a_ucMaskSize*a_ucMaskSize;

    int x, y, m, n;
    unsigned char *pucTemp1 = pucMeanBuf;
    unsigned char *pucTemp2 = pucMirrorBuf;
    unsigned char *pucDstBuf = a_pucMAEBuf;
    unsigned int udOffset = uwMirrorWidth*a_ucMaskSize-1;
    for ( y=0; y<a_uwHeight; y++)
    {
        for ( x=0; x<a_uwWidth; x++)
        {
            unsigned int udSum = 0;
            unsigned char ucMean = *pucTemp1;
            for ( m=-ucMaskSize_half; m<=ucMaskSize_half; m++)
            {
                for ( n=-ucMaskSize_half; n<=ucMaskSize_half; n++)
                {
                    udSum+=abs((*pucTemp2)-ucMean);
                    pucTemp2++;
                }
                pucTemp2+=(uwMirrorWidth-a_ucMaskSize);
            }

            *pucDstBuf = (unsigned char)((float)udSum/uwCount+0.5f);

            pucTemp1++;
            pucTemp2-=udOffset;
            pucDstBuf++;
        }

        pucTemp2+=ucMaskSize_half*2;
    }

    delete [] pucMirrorBuf;
    delete [] pucMeanBuf;
}

void CCMean( unsigned char *a_pucMeanBuf, unsigned char *a_pucSrcBuf, unsigned short a_uwWidth, unsigned short a_uwHeight, unsigned char a_ucMaskSize)
{
    unsigned char ucMaskSize_half = a_ucMaskSize/2;
    unsigned short uwMirrorWidth = a_uwWidth+ucMaskSize_half*2;
    unsigned short uwMirrorHeight = a_uwHeight+ucMaskSize_half*2;
    unsigned char *pucMirrorBuf = new unsigned char[uwMirrorWidth*uwMirrorHeight];
    CCMirror( pucMirrorBuf, a_pucSrcBuf, uwMirrorWidth, uwMirrorHeight, a_uwWidth, a_uwHeight, ucMaskSize_half, ucMaskSize_half, ucMaskSize_half, ucMaskSize_half);
    
    unsigned short uwIntgralWidth = uwMirrorWidth+1;
    unsigned short uwIntgralHeight = uwMirrorHeight+1;
    unsigned int *pudIntegralBuf = new unsigned int[uwIntgralWidth*uwIntgralHeight];
    CCIntegralImage( pudIntegralBuf, pucMirrorBuf, uwMirrorWidth, uwMirrorHeight);
    
    unsigned char ucIntegralOffset_x = ucMaskSize_half+1;
    unsigned char ucIntegralOffset_y = ucMaskSize_half+1;
    unsigned char ucAvgOffset_x = ucMaskSize_half;
    unsigned char ucAvgOffset_y = ucMaskSize_half;
    unsigned char ucWindowCount = a_ucMaskSize*a_ucMaskSize;
    
    int x, y;
    unsigned int *pudSrcBuf = pudIntegralBuf+ucIntegralOffset_y*uwIntgralWidth+ucIntegralOffset_x;
    unsigned char *pucDstBuf = a_pucMeanBuf;
    unsigned int udOffset = ucAvgOffset_y*uwIntgralWidth;
    for ( y=0; y<a_uwHeight; y++)
    {
        for ( x=0; x<a_uwWidth; x++)
        {
//             unsigned int udSum = pudIntegralBuf[(y+ucIntegralOffset_y+ucAvgOffset_y)*uwIntgralWidth+(x+ucIntegralOffset_x+ucAvgOffset_x)]
//                 -pudIntegralBuf[(y+ucIntegralOffset_y-ucAvgOffset_y-1)*uwIntgralWidth+(x+ucIntegralOffset_x+ucAvgOffset_x)]
//                 -pudIntegralBuf[(y+ucIntegralOffset_y+ucAvgOffset_y)*uwIntgralWidth+(x+ucIntegralOffset_x-ucAvgOffset_x-1)]
//                 +pudIntegralBuf[(y+ucIntegralOffset_y-ucAvgOffset_y-1)*uwIntgralWidth+(x+ucIntegralOffset_x-ucAvgOffset_x-1)];
            unsigned int udSum = pudSrcBuf[udOffset+ucAvgOffset_x]
                -pudSrcBuf[0-udOffset-uwIntgralWidth+ucAvgOffset_x]
                -pudSrcBuf[udOffset+(-ucAvgOffset_x-1)]
                +pudSrcBuf[0-udOffset-uwIntgralWidth+(-ucAvgOffset_x-1)];
            
            *pucDstBuf = (unsigned char)(udSum/(float)(ucWindowCount)+0.5f);
            
            pudSrcBuf++;
            pucDstBuf++;
        }
        pudSrcBuf+=(uwIntgralWidth-a_uwWidth);
    }
    
    delete [] pucMirrorBuf;
    delete [] pudIntegralBuf;
}

void CCImageBitConversion16To8(unsigned char *a_pucDstImgBuf, unsigned short *a_puwSrcImgBuf, unsigned short a_uwWidth, unsigned short a_uwHeight, unsigned char a_ucDstBit, unsigned char a_ucSrcBit)
{
    unsigned short *puwSrcBuf = a_puwSrcImgBuf;
    unsigned char *pucDstBuf = a_pucDstImgBuf;

    if (a_ucDstBit >= a_ucSrcBit)
    {
        unsigned short uwUp = 1<<(a_ucDstBit - a_ucSrcBit);

        for (int y = 0; y < a_uwHeight; y++)
        {
            for (int x = 0; x < a_uwWidth; x++)
            {
                *pucDstBuf = min(((*puwSrcBuf) * uwUp), 255);

                puwSrcBuf++;
                pucDstBuf++;
            }
        }
    }
    else
    {
        unsigned short uwDown = 1 << (a_ucSrcBit - a_ucDstBit);
        unsigned short uwRound = uwDown/2;

        for (int y = 0; y < a_uwHeight; y++)
        {
            for (int x = 0; x < a_uwWidth; x++)
            {
                *pucDstBuf = min((((*puwSrcBuf)+uwRound) / uwDown), 255);

                puwSrcBuf++;
                pucDstBuf++;
            }
        }
    }
}

void CCImageBitConversion8To16(unsigned short *a_puwDstImgBuf, unsigned char *a_pucSrcImgBuf, unsigned short a_uwWidth, unsigned short a_uwHeight, unsigned char a_ucDstBit, unsigned char a_ucSrcBit)
{
    unsigned char *pucSrcBuf = a_pucSrcImgBuf;
    unsigned short *puwDstBuf = a_puwDstImgBuf;

    if (a_ucDstBit >= a_ucSrcBit)
    {
        unsigned short uwUp = 1 << (a_ucDstBit - a_ucSrcBit);

        for (int y = 0; y < a_uwHeight; y++)
        {
            for (int x = 0; x < a_uwWidth; x++)
            {
                *puwDstBuf = min(((*pucSrcBuf) * uwUp), 65535);

                pucSrcBuf++;
                puwDstBuf++;
            }
        }
    }
    else
    {
        unsigned short uwDown = 1 << (a_ucSrcBit - a_ucDstBit);
        unsigned short uwRound = uwDown / 2;

        for (int y = 0; y < a_uwHeight; y++)
        {
            for (int x = 0; x < a_uwWidth; x++)
            {
                *puwDstBuf = ((*pucSrcBuf) + uwRound) / uwDown;

                pucSrcBuf++;
                puwDstBuf++;
            }
        }
    }
}

void CCGamma(unsigned short *a_puwDstImageBuf, unsigned short *a_puwSrcImageBuf, unsigned short a_uwWidth, unsigned short a_uwHeight, float a_fGamma)
{
    unsigned int i;
    unsigned short auwGamma[65536];
    for ( i = 0; i < 65536; i++)
        auwGamma[i] = min((unsigned int)(pow((float)i, a_fGamma) + 0.5f), 65535);

    unsigned int udCount = a_uwWidth*a_uwHeight;
    for ( i = 0; i < udCount; i++)
        a_puwDstImageBuf[i] = auwGamma[a_puwSrcImageBuf[i]];
}

void CCGamma(float *a_pfDstImageBuf, float *a_pfSrcImageBuf, unsigned short a_uwWidth, unsigned short a_uwHeight, float a_fGamma)
{
    unsigned int i;
    unsigned int udCount = a_uwWidth*a_uwHeight;
    for (i = 0; i < udCount; i++)
        a_pfDstImageBuf[i] = (float)pow(a_pfSrcImageBuf[i], a_fGamma);
}

void CC12To8Tone(unsigned char *a_pucDstBuf, unsigned short *a_puwSrcBuf, unsigned short a_uwWidth, unsigned short a_uwHeight, unsigned char *a_pucToneTable)
{
    unsigned char aucToneBuf[4096];
    unsigned int i;
    for (i = 0; i < 4092; i++)
    {
        unsigned uwIdx = i / 4;
        aucToneBuf[i] = (unsigned char)(a_pucToneTable[uwIdx] + ((i - uwIdx * 4) / 4.0f)*((short)a_pucToneTable[uwIdx + 1] - a_pucToneTable[uwIdx]) + 0.5f);
    }
    aucToneBuf[4092] = aucToneBuf[4093] = aucToneBuf[4094] = aucToneBuf[4095] = 255;

    unsigned int udCount = a_uwWidth*a_uwHeight;
    for (i = 0; i < udCount; i++)
    {
        a_pucDstBuf[i] = aucToneBuf[a_puwSrcBuf[i]];
    }
}

void CCAutoLevel(unsigned char *a_pucDstImageBuf, unsigned char *a_pucHighLevel, unsigned char *a_pucLowLevel, unsigned char *a_pucSrcImageBuf, unsigned short a_uwWidth, unsigned short a_uwHeight, unsigned short a_uwWOI_x, unsigned short a_uwWOI_y, unsigned short a_uwWOI_width, unsigned short a_uwWOI_height)
{
    unsigned int audHistogram[256];
    memset(audHistogram, 0, sizeof(unsigned int) * 256);
    for (int y = a_uwWOI_y; y < a_uwWOI_y + a_uwWOI_height; y++)
    {
        for (int x = 0; x < a_uwWOI_x + a_uwWOI_width; x++)
        {
            audHistogram[a_pucSrcImageBuf[y*a_uwWidth + x]]++;
        }
    }

    //detect high level
    unsigned char ucTHD = 1;//%
    unsigned int udCountTHD = (unsigned int)(a_uwWidth*a_uwHeight*ucTHD / 100.0);
    unsigned int udSum = 0;
    unsigned short uwIndex_high = 255;
    for (int i = 255; i >= 0; i--)
    {
        udSum += audHistogram[i];
        if (udSum > udCountTHD)
        {
            uwIndex_high = i;
            break;
        }
    }
    //printf("high level:%d\n", uwIndex_high);

    //detect low level
    ucTHD = 1;//%
    udCountTHD = (unsigned int)(a_uwWidth*a_uwHeight*ucTHD / 100.0);
    udSum = 0;
    unsigned short uwIndex_low = 0;
    for (int i = 0; i < 256; i++)
    {
        udSum += audHistogram[i];
        if (udSum > udCountTHD)
        {
            uwIndex_low = i;
            break;
        }
    }
    //printf("low level:%d\n", uwIndex_low);

    *a_pucHighLevel = (unsigned char)uwIndex_high;
    *a_pucLowLevel = (unsigned char)uwIndex_low;

    unsigned char aucTable[256];//8bit
    float fScale_low = 255.0f / (255 - uwIndex_low);
    unsigned char ucIndex_high2 = (unsigned char)((uwIndex_high - uwIndex_low)*fScale_low + 0.5f);
    float fScale_high = 255.0f / ucIndex_high2;
    float fScale = fScale_low*fScale_high;
    for (int i = 0; i < 256; i++)
    {
        aucTable[i] = min((unsigned short)((max(i, uwIndex_low) - uwIndex_low)*fScale + 0.5), 255);
    }

    for (int y = 0; y < a_uwHeight; y++)
    {
        for (int x = 0; x < a_uwWidth; x++)
        {
            a_pucDstImageBuf[y*a_uwWidth + x] = aucTable[a_pucSrcImageBuf[y*a_uwWidth + x]];
        }
    }
}

void CCUserLevel(unsigned char *a_pucDstImageBuf, unsigned char *a_pucSrcImageBuf, unsigned short a_uwWidth, unsigned short a_uwHeight, unsigned char a_ucHighLevel, unsigned char a_ucLowLevel, unsigned short a_uwWOI_x, unsigned short a_uwWOI_y, unsigned short a_uwWOI_width, unsigned short a_uwWOI_height)
{
    unsigned char aucTable[256];//8bit
    float fScale_low = 255.0f / (255 - a_ucLowLevel);
    unsigned char ucIndex_high2 = (unsigned char)((a_ucHighLevel - a_ucLowLevel)*fScale_low + 0.5f);
    float fScale_high = 255.0f / ucIndex_high2;
    float fScale = fScale_low*fScale_high;
    for (int i = 0; i < 256; i++)
    {
        aucTable[i] = min((unsigned short)((max(i, a_ucLowLevel) - a_ucLowLevel)*fScale + 0.5), 255);
    }

    for (int y = 0; y < a_uwHeight; y++)
    {
        for (int x = 0; x < a_uwWidth; x++)
        {
            a_pucDstImageBuf[y*a_uwWidth + x] = aucTable[a_pucSrcImageBuf[y*a_uwWidth + x]];
        }
    }
}

void CCGaussianFilter5x5(unsigned char *a_pucDstImageBuf, unsigned char *a_pucSrcImageBuf, unsigned short a_uwWidth, unsigned short a_uwHeight)
{
    //mirror
    unsigned short uwMirrorWidth = a_uwWidth + 4;
    unsigned short uwMirrorHeight = a_uwHeight + 4;
    unsigned char *pucMirrorBuf_src = new unsigned char[uwMirrorWidth*uwMirrorHeight];
    unsigned char *pucMirrorBuf_dst = new unsigned char[uwMirrorWidth*uwMirrorHeight];
    CCMirror(pucMirrorBuf_src, a_pucSrcImageBuf, uwMirrorWidth, uwMirrorHeight, a_uwWidth, a_uwHeight, 2, 2, 2, 2);
    CCMirror(pucMirrorBuf_dst, a_pucDstImageBuf, uwMirrorWidth, uwMirrorHeight, a_uwWidth, a_uwHeight, 2, 2, 2, 2);

    //[1 4 6 4 1]
    //horizontal
    unsigned char *pucSrcBuf = pucMirrorBuf_src + 2;
    unsigned char *pucDstBuf = pucMirrorBuf_dst + 2;
    for (int y = 0; y < uwMirrorHeight; y++)
    {
        for (int x = 2; x < uwMirrorWidth - 2; x++)
        {
            unsigned int udSum = pucSrcBuf[-2] + pucSrcBuf[-1] * 4 + pucSrcBuf[0] * 6 + pucSrcBuf[1] * 4 + pucSrcBuf[2];
            *pucDstBuf = (udSum + 8) / 16;
            
            pucSrcBuf++;
            pucDstBuf++;
        }
        pucSrcBuf += 4;
        pucDstBuf += 4;
    }

    //vertical
    pucSrcBuf = pucMirrorBuf_dst + 2 * uwMirrorWidth + 2;
    pucDstBuf = a_pucDstImageBuf;
    for (int y = 2; y < uwMirrorHeight - 2; y++)
    {
        for (int x = 2; x < uwMirrorWidth - 2; x++)
        {
            unsigned int udSum = pucSrcBuf[-2 * uwMirrorWidth] + pucSrcBuf[-uwMirrorWidth] * 4 + pucSrcBuf[0] * 6 + pucSrcBuf[uwMirrorWidth] * 4 + pucSrcBuf[2 * uwMirrorWidth];
            *pucDstBuf = (udSum + 8) / 16;

            pucSrcBuf++;
            pucDstBuf++;
        }

        pucSrcBuf += 4;
    }

    //CCCropWOI(a_pucDstImageBuf, pucMirrorBuf_dst, a_uwWidth, a_uwHeight, uwMirrorWidth, uwMirrorHeight, 2, 2);

    delete[] pucMirrorBuf_src;
    delete[] pucMirrorBuf_dst;
}

void CCGaussianFilter(unsigned char *a_pucDstImageBuf, unsigned char *a_pucSrcImageBuf, unsigned short a_uwWidth, unsigned short a_uwHeight, float a_fSigma)
{
    //calculate mask size
    unsigned char ucHalfMask = (unsigned char)ceil(3 * a_fSigma);
    unsigned char ucMaskSize = ucHalfMask * 2 + 1;
    float *pfMaskBuf = new float[ucMaskSize];
    for (int i = 0; i < ucMaskSize; i++)
        pfMaskBuf[i] = (float)(1 / sqrt(2 * 3.1415926*a_fSigma*a_fSigma)*exp(-(i - ucHalfMask)*(i - ucHalfMask) / (2 * a_fSigma*a_fSigma)));

    unsigned int *pudMaskBuf = new unsigned int[ucMaskSize];
    unsigned int udMaskSum = 0;
    for (int i = 0; i < ucMaskSize; i++)
    {
        pudMaskBuf[i] = (unsigned int)(pfMaskBuf[i] / pfMaskBuf[0] + 0.5f);
        udMaskSum += pudMaskBuf[i];
    }

    //mirror
    unsigned short uwMirrorWidth = a_uwWidth + ucHalfMask * 2;
    unsigned short uwMirrorHeight = a_uwHeight + ucHalfMask * 2;
    unsigned char *pucMirrorBuf_src = new unsigned char[uwMirrorWidth*uwMirrorHeight];
    unsigned char *pucMirrorBuf_dst = new unsigned char[uwMirrorWidth*uwMirrorHeight];
    CCMirror(pucMirrorBuf_src, a_pucSrcImageBuf, uwMirrorWidth, uwMirrorHeight, a_uwWidth, a_uwHeight, ucHalfMask, ucHalfMask, ucHalfMask, ucHalfMask);
    CCMirror(pucMirrorBuf_dst, a_pucDstImageBuf, uwMirrorWidth, uwMirrorHeight, a_uwWidth, a_uwHeight, ucHalfMask, ucHalfMask, ucHalfMask, ucHalfMask);
    
    //CCSaveBMP8File("_pucMirrorBuf_src.bmp", pucMirrorBuf_src, uwMirrorWidth, uwMirrorHeight);

    //horizontal
    unsigned char *pucSrcBuf = pucMirrorBuf_src;
    unsigned char *pucDstBuf = pucMirrorBuf_dst + ucHalfMask;
    unsigned int *pudTempBuf_mask = pudMaskBuf;
    for (int y = 0; y < uwMirrorHeight; y++)
    {
        for (int x = ucHalfMask; x < uwMirrorWidth - ucHalfMask; x++)
        {
            unsigned int udSum = 0;
            for (int m = 0; m < ucMaskSize; m++)
            {
                udSum += (*pucSrcBuf)*(*pudTempBuf_mask);

                pucSrcBuf++;
                pudTempBuf_mask++;
            }

            *pucDstBuf = (unsigned char)((udSum + udMaskSum / 2) / udMaskSum);

            pucSrcBuf -= (ucMaskSize - 1);
            pudTempBuf_mask -= ucMaskSize;
            pucDstBuf++;
        }

        pucSrcBuf += (ucMaskSize - 1);
        pucDstBuf += (ucMaskSize - 1);
    }

    //CCSaveBMP8File("_horizontal.bmp", pucMirrorBuf_dst, uwMirrorWidth, uwMirrorHeight);

    //vertical
    pucSrcBuf = pucMirrorBuf_dst + ucHalfMask;
    pucDstBuf = a_pucDstImageBuf;
    for (int y = ucHalfMask; y < uwMirrorHeight - ucHalfMask; y++)
    {
        for (int x = ucHalfMask; x < uwMirrorWidth - ucHalfMask; x++)
        {
            unsigned int udSum = 0;
            for (int m = 0; m < ucMaskSize; m++)
            {
                udSum += (*pucSrcBuf)*(*pudTempBuf_mask);

                pucSrcBuf+=uwMirrorWidth;
                pudTempBuf_mask++;
            }

            *pucDstBuf = (unsigned char)((udSum + udMaskSum / 2) / udMaskSum);

            pucSrcBuf -= (ucMaskSize*uwMirrorWidth - 1);
            pudTempBuf_mask -= ucMaskSize;
            pucDstBuf++;
        }

        pucSrcBuf += (ucMaskSize - 1);
    }

    //CCSaveBMP8File("_vertical.bmp", a_pucDstImageBuf, a_uwWidth, a_uwHeight);

    delete[] pfMaskBuf;
    delete[] pudMaskBuf;
    delete[] pucMirrorBuf_src;
    delete[] pucMirrorBuf_dst;
}

static void swap_(float *a_a, float *a_b);
static void plot_(unsigned char *a_pucImageBuf, unsigned short a_uwWidth, unsigned short a_uwx, unsigned short a_uwy, unsigned char a_ucColor);
static float fpart_(float x);
static float rfpart_(float x);
static float round_(float x);
void CCDrawLine(unsigned char *a_pucImageBuf, unsigned short a_uwWidth, short X0, short Y0, short X1, short Y1, unsigned char a_ucColor)
{
    unsigned short IntensityShift, ErrorAdj, ErrorAcc;
    unsigned short ErrorAccTemp, Weighting, WeightingComplementMask;
    short DeltaX, DeltaY, Temp, XDir;

    unsigned short NumLevels = 256;
    unsigned char IntensityBits = 8;
    unsigned char BaseColor = a_ucColor;

    /* Make sure the line runs top to bottom */
    if (Y0 > Y1) {
        Temp = Y0; Y0 = Y1; Y1 = Temp;
        Temp = X0; X0 = X1; X1 = Temp;
    }
    /* Draw the initial pixel, which is always exactly intersected by
    the line and so needs no weighting */
    plot_(a_pucImageBuf, a_uwWidth, X0, Y0, BaseColor);

    if ((DeltaX = X1 - X0) >= 0) {
        XDir = 1;
    }
    else {
        XDir = -1;
        DeltaX = -DeltaX; /* make DeltaX positive */
    }
    /* Special-case horizontal, vertical, and diagonal lines, which
    require no weighting because they go right through the center of
    every pixel */
    if ((DeltaY = Y1 - Y0) == 0) {
        /* Horizontal line */
        while (DeltaX-- != 0) {
            X0 += XDir;
            plot_(a_pucImageBuf, a_uwWidth, X0, Y0, BaseColor);
        }
        return;
    }
    if (DeltaX == 0) {
        /* Vertical line */
        do {
            Y0++;
            plot_(a_pucImageBuf, a_uwWidth, X0, Y0, BaseColor);
        } while (--DeltaY != 0);
        return;
    }
    if (DeltaX == DeltaY) {
        /* Diagonal line */
        do {
            X0 += XDir;
            Y0++;
            plot_(a_pucImageBuf, a_uwWidth, X0, Y0, BaseColor);
        } while (--DeltaY != 0);
        return;
    }
    /* Line is not horizontal, diagonal, or vertical */
    ErrorAcc = 0;  /* initialize the line error accumulator to 0 */
    /* # of bits by which to shift ErrorAcc to get intensity level */
    IntensityShift = 16 - IntensityBits;
    /* Mask used to flip all bits in an intensity weighting, producing the
    result (1 - intensity weighting) */
    WeightingComplementMask = NumLevels - 1;
    /* Is this an X-major or Y-major line? */
    if (DeltaY > DeltaX) {
        /* Y-major line; calculate 16-bit fixed-point fractional part of a
        pixel that X advances each time Y advances 1 pixel, truncating the
        result so that we won't overrun the endpoint along the X axis */
        ErrorAdj = ((unsigned long)DeltaX << 16) / (unsigned long)DeltaY;
        /* Draw all pixels other than the first and last */
        while (--DeltaY) {
            ErrorAccTemp = ErrorAcc;   /* remember currrent accumulated error */
            ErrorAcc += ErrorAdj;      /* calculate error for next pixel */
            if (ErrorAcc <= ErrorAccTemp) {
                /* The error accumulator turned over, so advance the X coord */
                X0 += XDir;
            }
            Y0++; /* Y-major, so always advance Y */
            /* The IntensityBits most significant bits of ErrorAcc give us the
            intensity weighting for this pixel, and the complement of the
            weighting for the paired pixel */
            Weighting = ErrorAcc >> IntensityShift;
            plot_(a_pucImageBuf, a_uwWidth, X0, Y0, BaseColor - Weighting);
            plot_(a_pucImageBuf, a_uwWidth, X0 + XDir, Y0, BaseColor - (Weighting ^ WeightingComplementMask));
        }
        /* Draw the final pixel, which is
        always exactly intersected by the line
        and so needs no weighting */
        plot_(a_pucImageBuf, a_uwWidth, X1, Y1, BaseColor);
        return;
    }
    /* It's an X-major line; calculate 16-bit fixed-point fractional part of a
    pixel that Y advances each time X advances 1 pixel, truncating the
    result to avoid overrunning the endpoint along the X axis */
    ErrorAdj = ((unsigned long)DeltaY << 16) / (unsigned long)DeltaX;
    /* Draw all pixels other than the first and last */
    while (--DeltaX) {
        ErrorAccTemp = ErrorAcc;   /* remember currrent accumulated error */
        ErrorAcc += ErrorAdj;      /* calculate error for next pixel */
        if (ErrorAcc <= ErrorAccTemp) {
            /* The error accumulator turned over, so advance the Y coord */
            Y0++;
        }
        X0 += XDir; /* X-major, so always advance X */
        /* The IntensityBits most significant bits of ErrorAcc give us the
        intensity weighting for this pixel, and the complement of the
        weighting for the paired pixel */
        Weighting = ErrorAcc >> IntensityShift;
        plot_(a_pucImageBuf, a_uwWidth, X0, Y0, BaseColor - Weighting);
        plot_(a_pucImageBuf, a_uwWidth, X0, Y0 + 1, BaseColor - (Weighting ^ WeightingComplementMask));
    }
    /* Draw the final pixel, which is always exactly intersected by the line
    and so needs no weighting */
    plot_(a_pucImageBuf, a_uwWidth, X1, Y1, BaseColor);
}

static void swap_(float *a_a, float *a_b)
{
    float fTemp = *a_a;
    *a_a = *a_b;
    *a_b = fTemp;
}

static void plot_(unsigned char *a_pucImageBuf, unsigned short a_uwWidth, unsigned short a_uwx, unsigned short a_uwy, unsigned char a_ucColor)
{
    a_pucImageBuf[a_uwy*a_uwWidth + a_uwx] = a_ucColor;
}

static float fpart_(float x)
{
    if (x < 0)
        return 1 - (x - floor(x));
    return x - floor(x);
}

static float rfpart_(float x)
{
    return 1 - fpart_(x);
}

static float round_(float x)
{
    return x + 0.5f;
}

static void BackTrack(unsigned char *a_pucSolution, unsigned char *a_pucSolution_temp, unsigned int *a_pudSolutionNum, bool *pbFilled, unsigned char a_ucDimension, unsigned char a_ucMaxDimension);

void EnumeratePermutations(unsigned char *a_pucSolution, unsigned char a_ucMaxDimension)
{
    bool *pbFilled = new bool[a_ucMaxDimension];
    for (int i = 0; i<a_ucMaxDimension; i++) // initialization
        pbFilled[i] = false;

    unsigned int udSolutionNum = 0;
    unsigned char *pucSolution_temp = new unsigned char[a_ucMaxDimension];
    BackTrack(a_pucSolution, pucSolution_temp, &udSolutionNum, pbFilled, 0, a_ucMaxDimension);   // 印出數字1到a_ucMaxDimension的所有排列。

    delete[] pucSolution_temp;
    delete[] pbFilled;

    printf("Num: %d\n", udSolutionNum);
}

static void BackTrack(unsigned char *a_pucSolution, unsigned char *a_pucSolution_temp, unsigned int *a_pudSolutionNum, bool *pbFilled, unsigned char a_ucDimension, unsigned char a_ucMaxDimension)
{
    // it's a solution
    if (a_ucDimension == a_ucMaxDimension)
    {
        //for (int i = 0; i < a_ucMaxDimension; i++)
        //{
        //    printf("%d ", a_pucSolution_temp[i]);
        //}
        //printf("\n");
        memcpy(a_pucSolution + (*a_pudSolutionNum)*a_ucMaxDimension, a_pucSolution_temp, a_ucMaxDimension);
        (*a_pudSolutionNum)++;

        return;
    }

    for (int i = 0; i < a_ucMaxDimension; i++)     // 試著將數字 n 填入各個位置
    {
        if (!pbFilled[i])
        {
            pbFilled[i] = true;   // 記錄填過的位置

            a_pucSolution_temp[i] = a_ucDimension;    // 將數字 n 填入第 i 格
            BackTrack(a_pucSolution, a_pucSolution_temp, a_pudSolutionNum, pbFilled, a_ucDimension + 1, a_ucMaxDimension);     // 繼續枚舉下一個數字

            pbFilled[i] = false;  // 回收位置
        }
    }
}