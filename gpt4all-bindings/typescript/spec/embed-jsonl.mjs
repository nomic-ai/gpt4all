import { loadModel, createEmbedding } from '../src/gpt4all.js'
import { createGunzip, createGzip, createUnzip } from 'node:zlib';
import { Readable } from 'stream'
import readline  from 'readline'
const embedder = await loadModel("nomic-embed-text-v1.5.f16.gguf", { verbose: true, type: 'embedding', device: 'gpu' })
console.log("Running with", embedder.llm.threadCount(), "threads");


const unzip = createGunzip();
const url = "https://huggingface.co/datasets/sentence-transformers/embedding-training-data/resolve/main/squad_pairs.jsonl.gz"
const stream = await fetch(url)
        .then(res => Readable.fromWeb(res.body));

const lineReader = readline.createInterface({
    input: stream.pipe(unzip),
    crlfDelay: Infinity
})

lineReader.on('line', line => {
    //pairs of questions and answers
    const question_answer = JSON.parse(line)
    console.log(createEmbedding(embedder, question_answer))
})

lineReader.on('close', () => embedder.dispose())

