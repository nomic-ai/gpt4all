# Java bindings

Java bindings let you load a gpt4all library into your Java application and execute text 
generation using an intuitive and easy to use API. No GPU is required because gpt4all system executes on the cpu.
The gpt4all models are quantized to easily fit into system RAM and use about 4 to 7GB of system RAM.

## Getting Started
You can add Java bindings into your Java project by adding dependency to your project:

**Maven**
```
<dependency>
    <groupId>com.hexadevlabs</groupId>
    <artifactId>gpt4all-java-binding</artifactId>
    <version>1.1.2</version>
</dependency>
```
**Gradle**
```
implementation 'com.hexadevlabs:gpt4all-java-binding:1.1.2'
```

To add the library dependency for another build system see [Maven Central Java bindings](https://central.sonatype.com/artifact/com.hexadevlabs/gpt4all-java-binding/).

To download a model binary weights file use an url such as https://gpt4all.io/models/ggml-gpt4all-j-v1.3-groovy.bin. 

For information about other models available see [Model file list](https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-chat#manual-download-of-models).

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

            // Will also stream to Standard out
            String fullGeneration = model.generate(prompt, config, true);

        } catch (Exception e) {
            // Exception generally may happen if model file fails to load 
            // for a number of reasons such as file not found. 
            // It is possible that Java may not be able to dynamically load the native shared library or 
            // the llmodel shared library may not be able to dynamically load the backend 
            // implementation for the model file you provided.
            // 
            // Once the LLModel class is successfully loaded into memory the text generation calls 
            // generally should not throw exceptions.
            e.printStackTrace(); // Printing here but in production system you may want to take some action.
        }
    }

}
```

For a maven based sample project that uses this library see [Sample project](https://github.com/felix-zaslavskiy/gpt4all-java-bindings-sample)

### Additional considerations
#### Logger warnings
The Java bindings library may produce a warning:
```
SLF4J: Failed to load class "org.slf4j.impl.StaticLoggerBinder".
SLF4J: Defaulting to no-operation (NOP) logger implementation
SLF4J: See http://www.slf4j.org/codes.html#StaticLoggerBinder for further details.
```
If you don't have a SLF4J binding included in your project. Java bindings only use logging for informational 
purposes, so logger is not essential to correctly use the library. You can ignore this warning if you don't have SLF4J bindings
in your project.

To add a simple logger using maven dependency you may use:
```
<dependency>
    <groupId>org.slf4j</groupId>
    <artifactId>slf4j-simple</artifactId>
    <version>1.7.36</version>
</dependency>
```

#### Loading your native libraries
1. Java bindings package jar comes bundled with native library files for Windows, macOS and Linux. These library files are 
copied to a temporary directory and loaded at runtime. For advanced users who may want to package shared libraries into Docker containers 
or want to use a custom build of the shared libraries and ignore the once bundled with the java package they have option 
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
2. Not every avx only shared library is bundled with the jar right now to reduce size. Only the libgptj-avx is included.
If you are running into issues please let us know using the gpt4all project issue tracker https://github.com/nomic-ai/gpt4all/issues.
