/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.tableflipstudios.worldviewer;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;
import android.content.Context;
import android.content.res.AssetManager;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.IOException;
import java.io.FileOutputStream;
import android.view.MotionEvent;
import android.content.pm.ActivityInfo;

import java.io.File;


public class GameActivity extends Activity {

    GameView mView;
    Context mContext;
    File mInternalStorage;
    AssetManager mAssetManager;
    
    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        
        mContext = getApplicationContext();
        mInternalStorage = mContext.getFilesDir();
        
        System.out.println( "!!! INTERNAL STORAGE = " + mInternalStorage + " !!!" );
        
        GameLib.setFileDir( mInternalStorage );
        
        mView = new GameView(getApplication(), true, 16, 8 );
        setContentView(mView);
        
        String files[] = null;
        try
        {
            mAssetManager = mContext.getResources().getAssets();
            traverseDir( "" );
        }
        catch( Exception e ) {}
        
        GameLib.setAssetManagerFromActivity( mAssetManager );
        
        // only portrait orientation
        setRequestedOrientation( ActivityInfo.SCREEN_ORIENTATION_PORTRAIT );
        
    }

    @Override protected void onPause() {
        super.onPause();
        mView.onPause();
    }

    @Override protected void onResume() {
        super.onResume();
        mView.onResume();
    }
    
    @Override
	public boolean onTouchEvent(MotionEvent event)
	{        
        int iNumPointers = event.getPointerCount();
        int iAction = event.getAction();
        
        if(iAction == MotionEvent.ACTION_DOWN ||
		   iAction == MotionEvent.ACTION_POINTER_1_DOWN ||
		   iAction == MotionEvent.ACTION_POINTER_2_DOWN ||
		   iAction == MotionEvent.ACTION_POINTER_3_DOWN)
		{
            for( int i = 0; i < iNumPointers; i++)
            {
                int iX = (int)event.getX(i);
                int iY = (int)event.getY(i);
                int iID = event.getPointerId(i);
                
                GameLib.touchBegan( iX, iY, iID );
                
                System.out.println( "touchBegan: num pointers = " + iNumPointers + " action = " + iAction +
                                    "( " + iX + ", " + iY + " )" );
            }
        }
        else if( iAction == MotionEvent.ACTION_MOVE )
        {
            for( int i = 0; i < iNumPointers; i++)
            {
                int iX = (int)event.getX(i);
                int iY = (int)event.getY(i);
                int iID = event.getPointerId(i);
                
                GameLib.touchMoved( iX, iY, iID );
                System.out.println( "touchMoved: num pointers = " + iNumPointers + " action = " + iAction +
                                   "( " + iX + ", " + iY + " )" );
            }
        }
        else if( iAction == MotionEvent.ACTION_UP )
        {
            for( int i = 0; i < iNumPointers; i++)
            {
                int iX = (int)event.getX(i);
                int iY = (int)event.getY(i);
                int iID = event.getPointerId(i);
                
                GameLib.touchEnded( iX, iY, iID );
                System.out.println( "touchEnded: num pointers = " + iNumPointers + " action = " + iAction +
                                   "( " + iX + ", " + iY + " )" );
                
            }
        }
        
        return true;
    }
    
    protected void traverseDir( String directory )
    {
    	String fileObjs[] = null;
    	try
    	{
    		fileObjs = mAssetManager.list( directory );
    		if( fileObjs.length > 0 )
    		{
    			for( int i = 0; i < fileObjs.length; i++ )
    			{
    				String childDir = directory + "/" + fileObjs[i];
                    System.out.println( "childDir = " + childDir );
    				traverseDir( childDir );
    			}
    		}
    		else
    		{
                System.out.println( "file = " + directory );
                
                // just want the filename
                String[] splits = directory.split( "\\/" );
                String fileName = splits[splits.length-1];
                
                System.out.println( "fileName = " + fileName );
                
    			final InputStream in = mAssetManager.open( fileName );
                System.out.println( "opened file " + directory );
                
                System.out.println( "internal storage = " + mInternalStorage.toString() );
                
                File outFile = new File( mInternalStorage.toString(), fileName );
                
                // save out the file if not there
                {
                    System.out.println( "created file" );
                    
                    FileOutputStream out = new FileOutputStream( outFile );
                    copyFile( in, out );
                    in.close();
                    out.flush();
                    out.close();
                }
    		}
    	}
    	catch( IOException e ) { System.out.println( "Exception" ); }
    }
    
    protected void copyFile(InputStream in, OutputStream out ) throws IOException
    {
        byte[] buffer = new byte[1024];
        int read;
        while( ( read = in.read( buffer ) ) != -1 )
        {
            out.write( buffer, 0, read );
        }
    }
}
