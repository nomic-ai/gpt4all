# Java Bindings Developer documents.

This document is meant to anyone looking to build the Java bindings from source, test a build locally and perform a release.

## Building locally

Maven is the build tool used by the project. Maven version of 3.8 or higher is recommended. Make sure the **mvn** 
is available on the command path.

The project builds to Java version 11 target so make sure that a JDK at version 11 or newer is installed.

### Setting up location of native shared libraries
The property **native.libs.location** in pom.xml may need to be set:
```
    <properties>
        ...
        <native.libs.location>C:\Users\felix\dev\gpt4all_java_bins\release_1_1_3_Jun22_2023</native.libs.location>
    </properties>
```
All the native shared libraries bundled with the Java binding jar will be copied from this location.
The directory structure is **native/linux**, **native/macos**, **native/windows**. These directories are copied
into the **src/main/resources** folder during the build process.

For the purposes of local testing, none of these directories have to be present or just one OS type may be present.

If none of the native libraries are present in **native.libs.location** the shared libraries will be searched for
in location path set by **LLModel.LIBRARY_SEARCH_PATH** static variable in Java source code that is using the bindings.

Alternately you can copy the shared libraries into the **src/resources/native/linux** before 
you build, but note **src/main/resources/native** is on the .gitignore, so it will not be committed to sources.

### Building

To package the bindings jar run:
```
mvn package
```
This will build two jars. One has only the Java bindings and the other is a fat jar that will have required dependencies included as well.

To package and install the Java bindings to your local maven repository run:
```
mvn install
```

### Using in a sample application

You can check out a sample project that uses the java bindings here:
https://github.com/felix-zaslavskiy/gpt4all-java-bindings-sample.git

1. First, update the dependency of java bindings to whatever you have installed in local repository such as **1.1.4-SNAPSHOT**
2. Second, update **Main.java** and set **baseModelPath** to the correct location of model weight files.

3. To make a runnable jar run:
```
mvn package
```

A fat jar is also created which is easy to run from command line:
```
java -jar target/gpt4all-java-bindings-sample-1.0-SNAPSHOT-jar-with-dependencies.jar
```

### Publish a public release.

For publishing a new version to maven central repository requires password and signing keys which F.Z. currently maintains, so
he is responsible for making a public release.

The procedure is as follows:

For a snapshot release
Run:
```
mvn deploy -P signing-profile
```

For a non-snapshot release
Run:
```
mvn clean deploy -P signing-profile,release
```