# Java bindings

Java bindings let you load a gpt4all library into your Java application and execute text 
generation using an intuitive and easy to use API. No GPU is required because gpt4all executes on the CPU.
The gpt4all models are quantized to easily fit into system RAM and use about 4 to 7GB of system RAM.

## Getting Started
You can add Java bindings into your Java project by adding the following dependency to your project:

**Maven**
```
<dependency>
    <groupId>com.hexadevlabs</groupId>
    <artifactId>gpt4all-java-binding</artifactId>
    <version>1.1.5</version>
</dependency>
```
**Gradle**
```
implementation 'com.hexadevlabs:gpt4all-java-binding:1.1.5'
```

To add the library dependency for another build system see [Maven Central Java bindings](https://central.sonatype.com/artifact/com.hexadevlabs/gpt4all-java-binding/).

To download model binary weights file use a URL such as [`https://gpt4all.io/models/gguf/gpt4all-13b-snoozy-q4_0.gguf`](https://gpt4all.io/models/gguf/gpt4all-13b-snoozy-q4_0.gguf).

For information about other models available see the [model file list](https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-chat#manual-download-of-models).

### Sample code
```java
public class Example {
    public static void main(String[] args) {

        String prompt = "### Human:\nWhat is the meaning of life\n### Assistant:";

        // Replace the hardcoded path with the actual path where your model file resides
        String modelFilePath = "C:\\Users\\felix\\AppData\\Local\\nomic.ai\\GPT4All\\ggml-gpt4all-j-v1.3-groovy.bin";
        
        try (LLModel model = new LLModel(Path.of(modelFilePath))) {

            // May generate up to 4096 tokens but generally stops early
            LLModel.GenerationConfig config = LLModel.config()
                    .withNPredict(4096).build();

            // Will also stream to standard output
            String fullGeneration = model.generate(prompt, config, true);

        } catch (Exception e) {
            // Exceptions generally may happen if the model file fails to load 
            // for a number of reasons such as a file not found. 
            // It is possible that Java may not be able to dynamically load the native shared library or 
            // the llmodel shared library may not be able to dynamically load the backend 
            // implementation for the model file you provided.
            // 
            // Once the LLModel class is successfully loaded into memory the text generation calls 
            // generally should not throw exceptions.
            e.printStackTrace(); // Printing here but in a production system you may want to take some action.
        }
    }

}
```

For a Maven-based sample project that uses this library see this [sample project](https://github.com/felix-zaslavskiy/gpt4all-java-bindings-sample)

### Additional considerations
#### Logger warnings
The Java bindings library may produce a warning if you don't have a SLF4J binding included in your project:
```
SLF4J: Failed to load class "org.slf4j.impl.StaticLoggerBinder".
SLF4J: Defaulting to no-operation (NOP) logger implementation
SLF4J: See http://www.slf4j.org/codes.html#StaticLoggerBinder for further details.
```
The Java bindings only use logging for informational 
purposes, so a logger is not essential to correctly use the library. You can ignore this warning if you don't have SLF4J bindings
in your project.

To add a simple logger using a Maven dependency you may use:
```
<dependency>
    <groupId>org.slf4j</groupId>
    <artifactId>slf4j-simple</artifactId>
    <version>1.7.36</version>
</dependency>
```

#### Loading your native libraries
1. the Java bindings package JAR comes bundled with a native library files for Windows, macOS and Linux. These library files are 
copied to a temporary directory and loaded at runtime. For advanced users who may want to package shared libraries into Docker containers 
or want to use a custom build of the shared libraries and ignore the once bundled with the Java package they have option 
to load libraries from your local directory by setting a static property to the location of library files.
There are no guarantees of compatibility if used in such a way so be careful if you really want to do it.

For example:
```java
class Example {
    public static void main(String[] args) {
        // gpt4all native shared libraries location
        LLModel.LIBRARY_SEARCH_PATH = "C:\\Users\\felix\\gpt4all\\lib\\"; 
        // ... use the library normally
    }
}
```
2. Not every AVX-only shared library is bundled with the JAR right now to reduce size. Only libgptj-avx is included.
If you are running into issues please let us know using the [gpt4all project issue tracker](https://github.com/nomic-ai/gpt4all/issues).

3. For Windows the native library included in jar depends on specific Microsoft C and C++ (MSVC) runtime libraries which may not be installed on your system.
If this is the case you can easily download and install the latest x64 Microsoft Visual C++ Redistributable package from https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170

4. When running Java in a Docker container it is advised to use eclipse-temurin:17-jre parent image. Alpine based parent images don't work due to the native library dependencies.

## Version history
1. Version **1.1.2**:
   - Java bindings is compatible with gpt4ll version 2.4.6
   - Initial stable release with the initial feature set
2. Version **1.1.3**:
   - Java bindings is compatible with gpt4all version 2.4.8
   - Add static GPT4ALL_VERSION to signify gpt4all version of the bindings
   - Add PromptIsTooLongException for prompts that are longer than context size.
   - Replit model support to include Metal Mac hardware support.
3. Version **1.1.4**:
   - Java bindings is compatible with gpt4all version 2.4.11
   - Falcon model support included.
4. Version **1.1.5**:
   - Add a check for model file readability before loading model.
   
