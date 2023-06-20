/// Container for model prompt parameters
///
/// **Attention**: _logits_ and _tokens_ has currently no effect!
class LLModelPromptConfig {

  /// logits of current context
  List<double> logits = [];

  /// current tokens in the context window
  List<int> tokens = [];

  /// number of tokens in past conversation
  int nPast = 0;

  /// number of tokens possible in context window
  int nCtx = 1024;

  /// number of tokens to predict
  int nPredict = 128;

  /// top k logits to sample from
  int topK = 40;

  /// nucleus sampling probability threshold
  double topP = 0.95;

  /// temperature to adjust model's output distribution
  double temp = 0.28;

  /// number of predictions to generate in parallel
  int nBatch = 8;

  /// penalty factor for repeated tokens
  double repeatPenalty = 1.1;

  /// last n tokens to penalize
  int repeatLastN = 10;

  /// percent of context to erase if we exceed the context window
  double contextErase = 0.55;
}
