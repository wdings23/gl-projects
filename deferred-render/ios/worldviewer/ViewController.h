//
//  ViewController.h
//  worldviewer
//
//  Created by Dingwings on 5/7/14.
//  Copyright (c) 2014 Dingwings. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

@interface ViewController : GLKViewController

@property (readwrite) float retinaScale;
@property (readwrite) int screenWidth;
@property (readwrite) int screenHeight;

@end
