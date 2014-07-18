#import <GameKit/GameKit.h>
#import <GameKit/GKScore.h>

#include "gamecenterwrapper.h"

NSString* sLeaderboardID = nil;
NSArray* _scores = nil;
NSArray* _playerArray = nil;

/*
**
*/
void setLeaderboardID( const char* szID )
{
    sLeaderboardID = [[NSString alloc] initWithUTF8String:szID];
}

/*
**
*/
void retrieveLeaderBoardData( void )
{
    GKLeaderboard* leaderboardRequest = [[GKLeaderboard alloc] init];
    if( leaderboardRequest != nil )
    {
        leaderboardRequest.playerScope = GKLeaderboardPlayerScopeGlobal;
        leaderboardRequest.timeScope = GKLeaderboardTimeScopeAllTime;
        leaderboardRequest.identifier = sLeaderboardID;
        leaderboardRequest.range = NSMakeRange( 1, 100 );
        [ leaderboardRequest loadScoresWithCompletionHandler: ^(NSArray* scores, NSError* error )
         {
             _scores = scores;
             
             NSArray* playerIDs = [scores valueForKeyPath:@"playerID"];
             [ GKPlayer loadPlayersForIdentifiers:playerIDs withCompletionHandler:^(NSArray* playerArray, NSError* error )
               {
                   if( error == nil && playerArray != nil )
                   {
                       _playerArray = [[NSArray alloc] initWithArray:playerArray];
                   }
                }
              
             ];
         }
        ];
    }
}

/*
**
*/
const char* getLeaderBoardName( int iIndex )
{
    if( _playerArray == nil )
    {
        return "";
    }
    
    GKPlayer* player = [_playerArray objectAtIndex:(NSUInteger)iIndex];
    
    NSString* playerName = player.alias;
    return [playerName cStringUsingEncoding:NSUTF8StringEncoding];
}

/*
**
*/
int getLeaderBoardScore( int iIndex )
{
    if( _scores == nil )
    {
        return 0;
    }
    
    GKScore* score = [_scores objectAtIndex:(NSUInteger)iIndex];
    return (int)score.value;
}

/*
**
*/
int getNumLeaderBoardEntries( void )
{
    if( _scores == nil )
    {
        return 0;
    }
    
    return (int)[_scores count];
}

/*
**
*/
void submitScore( int iScore )
{
    GKScore* scoreReporter = [[GKScore alloc] initWithLeaderboardIdentifier: sLeaderboardID];
    
    if( sLeaderboardID != nil )
    {
        if( scoreReporter != nil )
        {
            scoreReporter.value = (int64_t)iScore;
            scoreReporter.context = 0;
            
            NSArray* scores = @[scoreReporter];
            [ GKScore reportScores:scores withCompletionHandler:^(NSError* error )
              {
                  OUTPUT( "score submitted\n" );
              }
            ];
            
            retrieveLeaderBoardData();
        }
    }
}