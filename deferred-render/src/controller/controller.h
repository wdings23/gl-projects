//
//  controller.h
//  CityBenchmark
//
//  Created by Dingwings on 7/25/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//
#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
    
enum
{
    TOUCHCONTROL_BEGAN = 0,
    TOUCHCONTROL_MOVED,
    TOUCHCONTROL_ENDED,
    
    NUM_TOUCHCONTROL_TYPES,
};
    
void controllerTouchBegan( double fX, double fY, int iNumTouches );
void controllerTouchMoved( double fX, double fY, int iNumTouches );
void controllerTouchEnded( double fX, double fY, int iNumTouches );
    
void controllerUpdate( void );
    
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __CONTROLLER_H__