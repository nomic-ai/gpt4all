package com.hexadevlabs.gpt4all;

import jnr.ffi.LibraryLoader;
import jnr.ffi.LibraryOption;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class Util {

    public static LLModelLibrary loadSharedLibrary(String librarySearchPath){
        String libraryName = "llmodel";
        Map<LibraryOption, Object> libraryOptions = new HashMap<>();
        libraryOptions.put(LibraryOption.LoadNow, true); // load immediately instead of lazily (ie on first use)
        libraryOptions.put(LibraryOption.IgnoreError, false); // calls shouldn't save last errno after call

        if(librarySearchPath!=null) {
            Map<String, List<String>> searchPaths = new HashMap<>();
            searchPaths.put(libraryName, List.of(librarySearchPath));

            return LibraryLoader.loadLibrary(LLModelLibrary.class,
                    libraryOptions,
                    searchPaths,
                    libraryName
            );
        }else {

            return LibraryLoader.loadLibrary(LLModelLibrary.class,
                    libraryOptions,
                    libraryName
            );
        }

    }
}
