class GenerationConfig {

  List<double> logits = [];
  List<int> tokens = [];
  int nPast = 0;
  int nCtx = 1024;
  int nPredict = 128;
  int topK = 40;
  double topP = 0.95;
  double temp = 0.28;
  int nBatch = 8;
  double repeatPenalty = 1.1;
  int repeatLastN = 10;
  double contextErase = 0.55;
}
