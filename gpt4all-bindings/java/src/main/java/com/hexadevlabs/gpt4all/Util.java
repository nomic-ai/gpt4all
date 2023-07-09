package com.hexadevlabs.gpt4all;

import jnr.ffi.LibraryLoader;
import jnr.ffi.LibraryOption;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.CharsetDecoder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class Util {

    private static final Logger logger = LoggerFactory.getLogger(Util.class);
    private static final CharsetDecoder cs = StandardCharsets.UTF_8.newDecoder();

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

    /**
     * Copy over shared library files from resource package to
     * target Temp directory.
     *
     * @return Path path to the temp directory holding the shared libraries
     */
    public static Path copySharedLibraries() {
        try {
            // Identify the OS and architecture
            String osName = System.getProperty("os.name").toLowerCase();
            boolean isWindows = osName.startsWith("windows");
            boolean isMac = osName.startsWith("mac os x");
            boolean isLinux = osName.startsWith("linux");
            if(isWindows) osName = "windows";
            if(isMac) osName = "macos";
            if(isLinux) osName = "linux";

            //String osArch = System.getProperty("os.arch");

            // Create a temporary directory
            Path tempDirectory = Files.createTempDirectory("nativeLibraries");
            tempDirectory.toFile().deleteOnExit();

            String[] libraryNames = {
                    "gptj-default",
                    "gptj-avxonly",
                    "llmodel",
                    "mpt-default",
                    "llamamodel-230511-default",
                    "llamamodel-230519-default",
                    "llamamodel-mainline-default",
                    "llamamodel-mainline-metal",
                    "replit-mainline-default",
                    "replit-mainline-metal",
                    "ggml-metal.metal",
                    "falcon-default"
            };

            for (String libraryName : libraryNames) {

                if(!isMac && (
                        libraryName.equals("replit-mainline-metal")
                                || libraryName.equals("llamamodel-mainline-metal")
                                || libraryName.equals("ggml-metal.metal"))
                ) continue;

                if(isWindows){
                    libraryName = libraryName + ".dll";
                } else if(isMac){
                    if(!libraryName.equals("ggml-metal.metal"))
                        libraryName = "lib" + libraryName + ".dylib";
                } else if(isLinux) {
                    libraryName = "lib"+ libraryName + ".so";
                }

                // Construct the resource path based on the OS and architecture
                String nativeLibraryPath = "/native/" + osName + "/" + libraryName;

                // Get the library resource as a stream
                InputStream in = Util.class.getResourceAsStream(nativeLibraryPath);
                if (in == null) {
                    throw new RuntimeException("Unable to find native library: " + nativeLibraryPath);
                }

                // Create a file in the temporary directory with the original library name
                Path tempLibraryPath = tempDirectory.resolve(libraryName);

                // Use Files.copy to copy the library to the temporary file
                Files.copy(in, tempLibraryPath, StandardCopyOption.REPLACE_EXISTING);

                // Close the input stream
                in.close();
            }

            // Add shutdown hook to delete tempDir on JVM exit
            // On Windows deleting dll files that are loaded into memory is not possible.
            if(!isWindows) {
                Runtime.getRuntime().addShutdownHook(new Thread(() -> {
                    try {
                        Files.walk(tempDirectory)
                                .sorted(Comparator.reverseOrder())
                                .map(Path::toFile)
                                .forEach(file -> {
                                    try {
                                        Files.delete(file.toPath());
                                    } catch (IOException e) {
                                        logger.error("Deleting temp library file", e);
                                    }
                                });
                    } catch (IOException e) {
                        logger.error("Deleting temp directory for libraries", e);
                    }
                }));
            }

            return tempDirectory;
        } catch (IOException e) {
            throw new RuntimeException("Failed to load native libraries", e);
        }
    }

    public static String getValidUtf8(byte[] bytes) {
        try {
            return cs.decode(ByteBuffer.wrap(bytes)).toString();
        } catch (CharacterCodingException e) {
            return null;
        }
    }
}
