//
//  promo.mm
//  Game7_MacOS
//
//  Created by Dingwings on 5/14/14.
//  Copyright (c) 2014 Dingwings. All rights reserved.
//

#include "promo.h"

/*
**
*/
void promoGotoAppStore( const char* szURL )
{
    //[[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"itms://itunes.com/apps/tableflipstudios"]];
    
    NSString* pLink = [NSString stringWithUTF8String:szURL];
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:pLink]];
}