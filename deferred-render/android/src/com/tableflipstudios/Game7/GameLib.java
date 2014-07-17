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

package com.tableflipstudios.Game7;

import java.io.File;
import android.content.res.Resources;
import android.content.res.AssetManager;

// Wrapper for native library

public class GameLib {

     static {
         System.loadLibrary("game7");
     }

     public static void setFileDir(File fileDir)
     {
         String fileDirString = fileDir.toString();
         System.out.println( "setFileDIR : " + fileDirString );
         setSaveFileDir(fileDirString);
     }
     
     public static void setAssetManagerFromActivity( AssetManager assetManager )
     {
    	 setAssetManager( assetManager );
     }
    
    /**
     * @param width the current view width
     * @param height the current view height
     */
     public static native void init(int width, int height);
     public static native void step();
     public static native void setSaveFileDir(String fileDir);
     public static native void setAssetManager( AssetManager assetmanager );
     
     public static native void touchBegan( int iX, int iY, int iID );
     public static native void touchMoved( int iX, int iY, int iID );
     public static native void touchEnded( int iX, int iY, int iID );
}
