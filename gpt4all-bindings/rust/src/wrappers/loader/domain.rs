use std::fmt;

#[derive(Debug, Clone)]
pub struct CompletionModelConfig {
    pub default_prompt_template: Option<String>
}

#[derive(Debug, Clone)]
pub enum ModelBuildVariant {
    /// Automatically select the appropriate build variant.
    AUTO,

    /// Use the default build variant.
    DEFAULT,

    /// Use the AVX-only build variant.
    AVX_ONLY,

    /// Use build variant specified by a String.
    CUSTOM(String)
}

impl fmt::Display for ModelBuildVariant {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            ModelBuildVariant::AUTO => write!(f, "auto"),
            ModelBuildVariant::DEFAULT => write!(f, "default"),
            ModelBuildVariant::AVX_ONLY => write!(f, "avxonly"),
            ModelBuildVariant::CUSTOM(build_variant) => write!(f, "{}", build_variant),
        }
    }
}

#[derive(Debug, Clone)]
pub struct ModelLoadOptions {
    /// The Maximum context window size to be used
    pub nCtx: i32,

    /// Number of gpu layers to be used
    pub ngl: i32,

    /// The processing unit on which the model will run
    pub device: Device,

    /// The number of threads to be used by the model.
    pub threads: Option<i32>,

    /// The implementation to be used
    pub build_variant: ModelBuildVariant
}

#[derive(Debug, Clone, PartialEq)]
pub enum Device {
    /// Central Processing Unit
    CPU,
    /// Graphics Processing Unit
    GPU,
    /// Graphics Processing Unit specified by vendor name
    ///
    /// - "amd": AMD Graphics Processing Unit
    /// - "nvidia": NVIDIA Graphics Processing Unit
    /// - "intel": Intel Graphics Processing Unit
    GPU_BY_VENDOR(String),
    /// Graphics Processing Unit specified by name
    GPU_BY_NAME(String),
}

impl fmt::Display for Device {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Device::CPU => write!(f, "cpu"),
            Device::GPU => write!(f, "gpu"),
            Device::GPU_BY_VENDOR(vendor) => write!(f, "{}", vendor),
            Device::GPU_BY_NAME(name) => write!(f, "{}", name)
        }
    }
}

/// Struct representing a GPU device.
#[derive(Debug, Clone)]
pub struct GpuDevice {
    /// Index (identifier) of the GPU device
    pub index: i32,
    /// Type of the GPU device.
    pub device_type: i32,
    /// Size of the device's heap.
    pub heap_size: usize,
    /// Name of the GPU device.
    pub name: String,
    /// Vendor of the GPU device.
    pub vendor: String
}


/// Builder for `ModelLoadOptions`
#[derive(Debug, Clone)]
pub struct ModelLoadOptionsBuilder {
    nCtx: i32,
    ngl: i32,
    device: Device,
    threads: Option<i32>,
    build_variant: ModelBuildVariant,
}

impl ModelLoadOptionsBuilder {
    /// Create a new `ModelLoadOptionsBuilder` with required parameters.
    pub fn new() -> Self {
        let default_options = ModelLoadOptions::default();

        ModelLoadOptionsBuilder {
            nCtx: default_options.nCtx,
            ngl: default_options.ngl,
            device: default_options.device,
            threads: default_options.threads,
            build_variant: default_options.build_variant,
        }
    }

    /// Set the Maximum context window size.
    pub fn nCtx(mut self, nCtx: i32) -> Self {
        self.nCtx = nCtx;
        self
    }

    /// Set the number of gpu layers.
    pub fn ngl(mut self, ngl: i32) -> Self {
        self.ngl = ngl;
        self
    }

    /// Set the processing unit on which the model will run.
    pub fn device(mut self, device: Device) -> Self {
        self.device = device;
        self
    }

    /// Set the number of threads.
    pub fn threads(mut self, threads: i32) -> Self {
        self.threads = Some(threads);
        self
    }

    /// Set the implementation to be used.
    pub fn build_variant(mut self, build_variant: ModelBuildVariant) -> Self {
        self.build_variant = build_variant;
        self
    }

    /// Build the `ModelLoadOptions` struct.
    pub fn build(self) -> ModelLoadOptions {
        ModelLoadOptions {
            nCtx: self.nCtx,
            ngl: self.ngl,
            device: self.device,
            threads: self.threads,
            build_variant: self.build_variant,
        }
    }
}
