use std::{env, fs, io};
use std::path::{Path,PathBuf};
use std::process::Command;
use walkdir::WalkDir;

/// Builds the CMake backend project.
/// # Arguments
///
/// * `backend_source_path` - The path to the backend source directory.
/// * `build_dir_path` - The path to the directory where the project will be built.
///
fn build_cmake_backend_project(backend_source_path: &PathBuf, build_dir_path: &PathBuf) -> Result<(), Box<dyn std::error::Error>> {
    // Temporary build folder
    let out_dir = env::var("OUT_DIR").expect("OUT_DIR environment variable not set");
    let build_path_temp = Path::new(&out_dir).join("./build_temp");

    // Run cmake to generate build files
    // Executes: cmake -S 'backend_source_path' -B 'build_path_temp' -DCMAKE_BUILD_TYPE=RelWithDebInfo
    let cmake_build = Command::new("cmake")
        .arg("-S")
        .arg(&backend_source_path)
        .arg("-B")
        .arg(&build_path_temp)
        .arg("-DCMAKE_BUILD_TYPE=RelWithDebInfo")
        .output()?;

    if !cmake_build.status.success() {
        if let Ok(str) = std::str::from_utf8(&cmake_build.stdout) {
            println!("{}", str);
        }
        if let Ok(str) = std::str::from_utf8(&cmake_build.stderr) {
            eprintln!("{}", str);
        }
        return Err(format!("Failed to generate build files: {}", cmake_build.status).into());
    }

    // Run cmake to build the project
    // Executes: cmake --build 'build_path_temp' --parallel --config Release
    let cmake_output = Command::new("cmake")
        .arg("--build")
        .arg(build_path_temp.clone())
        .arg("--parallel")
        .arg("--config")
        .arg("Release")
        .output()?;

    if !cmake_output.status.success() {
        return Err(format!("Failed to build project: {}", cmake_output.status).into());
    }

    // Copy final build files
    copy_prebuilt_c_lib(&build_path_temp, build_dir_path).expect("Failed to copy prebuilt C library files");

    // Remove Temporary build folder
    fs::remove_dir_all(&build_path_temp).map_err(|err| format!("Error removing temporary build directory: {}", err))?;

    // Build Success
    Ok(())
}


/// Copies prebuilt C library files from the temporary build directory to the final build directory.
///
/// # Arguments
///
/// * `temp_build_path` - The path to the temporary build directory containing the prebuilt library files.
/// * `final_build_path` - The path to the final build directory where the library files will be copied.
///
fn copy_prebuilt_c_lib(temp_build_path: &PathBuf, final_build_path: &PathBuf) -> io::Result<()> {
    // Create the final build directory if it doesn't exist
    if ! final_build_path.exists() {
        fs::create_dir_all(&final_build_path)?;
    }

    // Iterate over files in the temporary build directory
    let walker = WalkDir::new(temp_build_path).follow_links(true);
    for entry in walker {
        let entry = entry?;
        let file_type = entry.file_type();

        // Check if the entry is a file
        if file_type.is_file() {
            let src_path = entry.path();
            let file_name = src_path.file_name().unwrap().to_str().unwrap();

            // Check if the file has a supported extension (we need only library extensions)
            if file_name.ends_with(".dll") || file_name.ends_with(".so") || file_name.ends_with(".dylib")  || file_name.ends_with(".metal") {
                // Need Metal info only for macOS (linux/windows use Vulkan SDK)
                #[cfg(not(target_os = "macos"))]
                {
                    if file_name.ends_with(".metal") { continue; }
                }

                // Construct the destination path in the final build directory
                let dest_path = final_build_path.join(file_name);

                // Copy the file to the final build directory
                fs::copy(&src_path, &dest_path)?;
            }
        }
    }

    Ok(())
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Look for backend source folder in current directory or parent directory
    let backend_source_path = if Path::new("./gpt4all-backend").exists() {
        PathBuf::from("./gpt4all-backend")
    } else {
        PathBuf::from("./../../gpt4all-backend")
    };

    // Build directory path
    let out_dir_path = PathBuf::from(env::var("OUT_DIR")?);
    let build_dir_path = out_dir_path.join("build");

    // Build C Lib
    if let Err(e) = build_cmake_backend_project(&backend_source_path, &build_dir_path) {
        eprintln!("Failed to build backend project with CMake");
        return Err(e);
    }

    let canonized_lib_folder = build_dir_path
        // Canonicalize the path as `rustc-link-search` requires an absolute
        // path.
        .canonicalize()?;


    // On Linux, Cargo expects the shared library to have a versioned
    // file name (e.g., libllmodel.so.0), but CMake may generate the shared library
    // without a version number (e.g., libllmodel.so). This code block creates a
    // symbolic link from libllmodel.so to libllmodel.so.0 to satisfy Cargo's
    // expectations.
    #[cfg(target_os = "linux")]
    {
        let symlink_path = canonized_lib_folder.join("libllmodel.so.0");
        let target_path = canonized_lib_folder.join("libllmodel.so");
        std::os::unix::fs::symlink(target_path, symlink_path).ok();
    }

    // Tell cargo to look for shared libraries in the specified directory
    println!("cargo:rustc-link-search=native={}", canonized_lib_folder.display());

    // Tell cargo to tell rustc to link against the `llmodel` library.
    println!("cargo:rustc-link-lib={}", "llmodel");

    // Success :)
    Ok(())
}
