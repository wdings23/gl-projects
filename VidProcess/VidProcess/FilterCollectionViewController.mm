//
//  FilterCollectionViewController.m
//  VidProcess
//
//  Created by Dingwings on 6/8/14.
//  Copyright (c) 2014 Dingwings. All rights reserved.
//

#import "FilterCollectionViewController.h"
#import "FilterCollectionViewCell.h"

#include "shadermanager.h"
#include "filepathutil.h"

@interface FilterCollectionViewController () <UICollectionViewDataSource, UICollectionViewDelegateFlowLayout, UICollectionViewDelegate>

@end

@implementation FilterCollectionViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view.
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section
{
    std::vector<std::string> aFileNames;
    getAllFilesInDirectory( "", aFileNames, "nfo" );
    return aFileNames.size();
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    NSUInteger iCellIndex = [indexPath indexAtPosition:1];
    
    std::vector<std::string> aFileNames;
    getAllFilesInDirectory( "", aFileNames, "nfo" );
    
    // fill out cell info
    FilterCollectionViewCell* pCell = [collectionView dequeueReusableCellWithReuseIdentifier:@"Cell" forIndexPath:indexPath];
    pCell.backgroundColor = [UIColor clearColor];
    
    // file name
    char szFileName[256];
    int iLength = (int)strlen( aFileNames[iCellIndex].c_str() );
    
    memset( szFileName, 0, sizeof( szFileName ) );
    memcpy( szFileName, aFileNames[iCellIndex].c_str(), iLength - 4 );
    
    NSString* fileName = [NSString stringWithUTF8String:szFileName];
    pCell.filterName.text = fileName;
    
    aFileNames.clear();
    getAllFilesInDirectory( "", aFileNames, "png" );
    std::string pngName = aFileNames[iCellIndex];
    
    char szFullPath[256];
    getWritePath( szFullPath, pngName.c_str() );
    
    pCell.frameImage.image = [UIImage imageWithContentsOfFile:[NSString stringWithUTF8String:szFullPath]];
    
    return pCell;
}

- (void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath
{
    FilterCollectionViewCell* pSelected = [collectionView cellForItemAtIndexPath:indexPath];
    pSelected.backgroundColor = [UIColor redColor];
    
    // get movie filename
    NSUInteger iCellIndex = [indexPath indexAtPosition:1];

    // broadcast to start movie
    NSDictionary* pCellInfo = @{@"CellIndex": @(iCellIndex)};
    [[NSNotificationCenter defaultCenter] postNotificationName:@"StartMovie" object:pSelected userInfo:pCellInfo];

    [self.navigationController popToRootViewControllerAnimated:YES];
}

@end
