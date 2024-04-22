/// Represents embeddings (vectors) of embedded texts.
#[derive(Debug)]
pub struct Embedding {
    /// The vectors representing the text embeddings.
    pub vectors: Vec<Vec<f32>>
}

impl Embedding {
    /// Creates a new `Embedding` instance with the given vectors.
    pub fn new(vectors: Vec<Vec<f32>>) -> Self {
        Self { vectors }
    }
}


/// Options for generating embeddings using the model.
pub struct EmbeddingOptions {
    /// Texts for which embeddings are to be generated.
    pub texts: Vec<String>,

    /// The model-specific prefix representing the embedding task, without the trailing colon.
    /// Set to `None` for no prefix.
    pub prefix: Option<String>,

    /// The embedding dimension. Set to -1 for full-size.
    pub dimensionality: i32,

    /// Determines whether to average multiple embeddings if the text is longer than the model can accept.
    /// If `true`, embeddings will be averaged. If `false`, text will be truncated.
    pub do_mean: bool,

    /// Try to be fully compatible with the Atlas API.
    /// Currently, this means texts longer than 8192 tokens with do_mean="true" will raise an error.
    /// Disabled by default.
    pub atlas: bool
}


/// Builder for constructing `EmbeddingOptions`.
pub struct EmbeddingOptionsBuilder {
    /// Texts for which embeddings are to be generated.
    texts: Vec<String>,

    /// The model-specific prefix representing the embedding task, without the trailing colon.
    prefix: Option<String>,

    /// The embedding dimension. Set to -1 for full-size.
    dimensionality: i32,

    /// Determines whether to average multiple embeddings if the text is longer than the model can accept.
    /// If `true`, embeddings will be averaged. If `false`, text will be truncated.
    do_mean: bool,

    /// Try to be fully compatible with the Atlas API.
    /// Currently, this means texts longer than 8192 tokens with long_text_mode="mean" will raise an error.
    /// Disabled by default.
    atlas: bool
}

impl EmbeddingOptionsBuilder {
    /// Creates a new `EmbeddingOptionsBuilder`.
    pub fn new() -> Self {
        let default_options = EmbeddingOptions::default();

        Self {
            texts: default_options.texts,
            prefix: default_options.prefix,
            dimensionality: default_options.dimensionality,
            do_mean: default_options.do_mean,
            atlas: default_options.atlas,
        }
    }

    /// Sets the texts for which embeddings will be generated.
    pub fn texts(mut self, texts: &Vec<String>) -> Self {
        self.texts = texts.clone();
        self
    }

    /// Adds a text to the texts for which embeddings will be generated.
    pub fn add_text(mut self, text: String) -> Self {
        self.texts.push(text);
        self
    }

    /// Sets the model-specific prefix representing the embedding task.
    pub fn prefix(mut self, prefix: &str) -> Self  {
        self.prefix = Some(prefix.to_string());
        self
    }

    /// Sets the embedding dimension.
    pub fn dimensionality(mut self, dimensionality: i32) -> Self {
        self.dimensionality = dimensionality;
        self
    }

    /// Sets whether to average multiple embeddings if the text is longer than the model can accept.
    pub fn do_mean(mut self, do_mean: bool) -> Self {
        self.do_mean = do_mean;
        self
    }

    /// Sets whether to be fully compatible with the Atlas API.
    pub fn atlas(mut self, atlas: bool) -> Self {
        self.atlas = atlas;
        self
    }

    /// Builds `EmbeddingOptions` based on the set values.
    pub fn build(&self) -> EmbeddingOptions {
        EmbeddingOptions {
            texts: self.texts.clone(),
            prefix: self.prefix.clone(),
            dimensionality: self.dimensionality,
            do_mean: self.do_mean,
            atlas: self.atlas,
        }
    }
}
