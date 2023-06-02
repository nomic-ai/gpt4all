# Java bindings for GPT4ALL

## Getting Started
You can add GPT4ALL Large language models into your Java project quick and easy.
* Install the shared library on your system. On Windows the two files are **llmodel.dll** and **llama.dll**, 
on Linux these libraries end in extension **.so** and on Mac they end in extension **.dylib**.
1. On Windows shared libraries have to be on your JVM's library search path or you can set the *LLModel.LIBRARY_SEARCH_PATH* property.
For example to add it to your JVM search path:
```
java -Djava.library.path=C:\Users\felix\gpt4all\lib\ myapp.jar
```
```
// Or form inside Java
LLModel.LIBRARY_SEARCH_PATH = "C:\\Users\\felix\\gpt4all\\lib\\";
```
2. On macOS set the *DYLD_LIBRARY_PATH* environment variable to locate shared libraries.
3. On Linux set the *LD_LIBRARY_PATH*.
* Add dependency to you project.
```
<dependency>
    <groupId>com.hexadevlabs</groupId>
    <artifactId>gpt4all-java-binding</artifactId>
    <version>1.1.0</version>
</dependency>
```
* Sample Code:
```java
public class Example {
    public static void main(String[] args) {

        String prompt = "### Human:\nWhat is the meaning of life\n### Assistant:";

        // Optionally in case extra search path are necessary.
        //LLModel.LIBRARY_SEARCH_PATH = "C:\\Users\\felix\\gpt4all\\lib\\";

        try (LLModel model = new LLModel(Path.of("C:\\Users\\felix\\AppData\\Local\\nomic.ai\\GPT4All\\ggml-mpt-7b-instruct.bin"))) {

            LLModel.GenerationConfig config = LLModel.config()
                    .withNPredict(4096).build();

            model.generate(prompt, config, true);

        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

}
```

For a complete sample project that you can try out see https://github.com/felix-zaslavskiy/gpt4all-java-bindings-sample
