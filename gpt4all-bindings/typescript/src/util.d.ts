/// <reference types="node" />
declare module "gpt4all";

/**
 * Initiates the download of a model file of a specific model type.
 * By default this downloads without waiting. use the controller returned to alter this behavior.
 * @param {ModelFile} model - The model file to be downloaded.
 * @param {DownloadOptions} options - to pass into the downloader. Default is { location: (cwd), debug: false }.
 * @returns {DownloadController} object that allows controlling the download process.
 *
 * @throws {Error} If the model already exists in the specified location.
 * @throws {Error} If the model cannot be found at the specified url.
 *
 * @example
 * const controller = download('ggml-gpt4all-j-v1.3-groovy.bin')
 * controller.promise().then(() => console.log('Downloaded!'))
 */
declare function downloadModel(
    modelName: string,
    options?: DownloadModelOptions
): DownloadController;

/**
 * Options for the model download process.
 */
export interface DownloadModelOptions {
    /**
     * location to download the model.
     * Default is process.cwd(), or the current working directory
     */
    modelPath?: string;

    /**
     * Debug mode -- check how long it took to download in seconds
     * @default false
     */
    debug?: boolean;

    /**
     * Remote download url. Defaults to `https://gpt4all.io/models`
     * @default https://gpt4all.io/models
     */
    url?: string;
}

declare function listModels(): Promise<Record<string, string>[]>;

interface RetrieveModelOptions {
    allowDownload?: boolean;
    verbose?: boolean;
    modelPath?: string;
}

declare async function retrieveModel(
    model: string,
    options?: RetrieveModelOptions
): Promise<string>;

/**
 * Model download controller.
 */
interface DownloadController {
    /** Cancel the request to download from gpt4all website if this is called. */
    cancel: () => void;
    /** Convert the downloader into a promise, allowing people to await and manage its lifetime */
    promise: () => Promise<void>;
}

export { downloadModel, DownloadModelOptions, DownloadController, listModels, retrieveModel, RetrieveModelOptions };
