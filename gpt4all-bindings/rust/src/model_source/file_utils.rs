use std::error::Error;
use std::fmt::{Display, Formatter};
use std::fs;
use std::path::PathBuf;

// Import the StreamExt extension trait (files are large (GBs) => preloading to RAM is not a good idea)
use futures::StreamExt;
use md5::{Digest, Md5};
use reqwest::Url;
use tokio::fs::File;
use tokio::io::AsyncWriteExt;
use crate::model_source::file_utils::FileDownloadError::{HashMismatchError, NetworkError};


#[derive(Debug)]
pub(crate) enum FileDownloadError {
    /// Network Error.
    NetworkError(String),
    /// Integrity check failed.
    HashMismatchError
}

impl Error for FileDownloadError { }

impl Display for FileDownloadError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            NetworkError(message) => f.write_str(message),
            HashMismatchError => f.write_str("Integrity check failed.")
        }
    }
}


/// Downloads a file from a URL with integrity check based on MD5 hash.
///
/// # Arguments
///
/// * `url` - The URL of the file to download.
/// * `file_path` - The local path where the file will be saved.
/// * `md5_expected` - The expected MD5 hash value of the downloaded file.
///
/// # Returns
///
/// A `Result` containing `Ok(())` if the download is successful and the file integrity is verified,
/// or an error of type `Box<dyn Error>` if any operation fails.
pub(crate) async fn download_file_with_integrity(url: &str, file_path: &str, md5_expected: &str) -> Result<(), Box<dyn Error>> {
    let url = Url::parse(url)?;

    let response = reqwest::get(url).await?;

    // Check if the request was successful (status code 2xx)
    if ! response.status().is_success() {
        return Err(NetworkError(response.status().to_string()).into());
    }

    // Create a new file at the specified path
    let mut file = File::create(&file_path).await?;

    // Initialize MD5 hasher for integrity checks
    let mut hasher = Md5::new();

    // Stream response to file
    let mut download_stream = response.bytes_stream();
    while let Some(chunk_result) = download_stream.next().await {
        // save chunk + update hasher
        let chunk = chunk_result?;

        file.write_all(&chunk).await?;
        hasher.update(&chunk);
    }

    // Calculate hash result
    let result = hasher.finalize();
    let md5_hex_value = format!("{:x}", result);

    // Check if hash matches with the expected value
    if md5_hex_value != md5_expected {
        // Remove file if MD5 hash doesn't match
        if let Err(e) = fs::remove_file(&file_path) {
            eprintln!("Failed to remove file: {}", e);
        }
        return Err(HashMismatchError.into());
    }

    Ok(())
}


/// Function to get the model folder path as 'PathBuf' or create it if it does not exist
pub(crate) fn get_or_create_home_dir_sub_folder(sub_folder: &str) -> PathBuf {
    let model_folder = if let Some(mut home_dir) = dirs::home_dir() {
        home_dir.push(sub_folder);
        home_dir
    } else {
        PathBuf::from("./")
    };

    // Create the directory if it doesn't exist
    if ! model_folder.exists() {
        if let Err(err) = fs::create_dir_all(&model_folder) {
            eprintln!("Failed to create model folder: {}", err);
        }
    }

    model_folder
}
